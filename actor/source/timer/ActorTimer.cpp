#include "ActorTimer.hpp"




namespace wsf
{
    namespace actor
    {

		Timer* Timer::instance_ = NULL;
		/**	\brief	定时器构造函数
		*	\details	初始化定时器缓存池、加载时间轮、初始化时间轮精度
		*/
		Timer::Timer(int maxsize/* = 8 * 1024*/)
			: tpool_(maxsize)
		{
			time_wheel_ = inteface_time_wheel_load_symbol();

			accuracy_ = 40;//40ms
		}

		/**	\brief	定时器初始化
		*	\details	初始化时间轮定时系统初始化时刻、时间轮转动一格需要经过的钟摆和钟摆阈值、
		初始化时间轮和时间轮信息头
		*/
		bool Timer::Initialize()
		{
			begin_ = ::Theron::Detail::Clock::GetTicks();
			Theron::uint64_t frequency = ::Theron::Detail::Clock::GetFrequency();
			tick_per_unit_ = frequency / (1000 / accuracy_);   //ticks per jiffy
			threshhold_ = frequency / (1000 / (accuracy_ - 3));//ticks per jiffy
			hTimer_ = time_wheel_->init(0);
			this->Run();
			return true;
		}

		/**	\brief	定时器释放
		*	\details	销毁时间轮信息头和时间轮
		*	\bug	退出主循环 stop_ = false;
		*/
		void Timer::Terminate()
		{
			stop_ = true;
			thread_.Join();
			time_wheel_->destroy(hTimer_);
		}


		/**	\brief	外定时器超时回调函数
		*	\details	调用用户设置的内定时器超时回调函数后，将定时器释放回缓存链表。
		*	\param	data 定时器地址
		*/
		static void timer_callback(void *data)
		{
			Timer::timer_t *timer = (Timer::timer_t *)data;
			timer->timer_function(timer->parameter);
			timer->pool->Free(timer);
		}

		/**	\brief	添加一个定时器
		*	\details	从定时器缓存链表中取出一个定时器，进行初始化后，返回定时器在链表中的ID
		*	\param	timeout 超时时长（ms）
		*	\param	fn	超时回调函数
		*	\param	param	超时回调函数入参
		*	\return	定时器在链表中的ID
		*/
		uint32_t Timer::Add(uint32_t timeout, /*void (*fn)(unsigned long)*/ void(*fn)(parameter_t &param), parameter_t &param)
		{
			assert(!stop_);
			Theron::uint64_t now = ::Theron::Detail::Clock::GetTicks() - begin_;        //ticks
			Theron::uint64_t expires_ticks = now + timeout * tick_per_unit_ / accuracy_;//ticks
			Theron::uint64_t jiffies = (now / tick_per_unit_);

			time_wheel_->set_jiffies(hTimer_, (unsigned long)jiffies);
			unsigned long expires = (unsigned long)(expires_ticks / tick_per_unit_);

			Lock();

			timer_t *timer = (timer_t *)tpool_.Alloc();
			if (timer == NULL)
			{
				return INVALID_TIMERID;
			}

			if (timer)
			{
				timer->expires = expires;        //!<	设置从时间轮定时系统初始化开计时的超时经历时长
				timer->function = timer_callback;//!<	设置外超时回调函数，超时被时间轮调用
				timer->timer_function = fn;      //!<	设置内超时回调函数，用户设置后，被外超时回调函数调用
				timer->data = timer;             //!<	设置外超时回调函数入参，等于定时器的地址
				param.id = (uint32_t)timer->id;
				param.is_canceled = false;
				timer->parameter = param;        //!<	设置内超时回调函数入参，等于用户地址

				timer->base = (tvec_base *)hTimer_;
				time_wheel_->add_timer(hTimer_, timer);
			}
			Unlock();
			return (uint32_t)timer->id;
		}

		/**	\brief 根据定时器缓存池ID删除定时器
		*	\details	首先根据缓存池ID得到定时器，然后从时间轮中删除定时器，最后将定时器归还缓存池。
		*/
		bool Timer::Del(uint32_t id)
		{

			bool found = false;
			assert(id != INVALID_TIMERID);
			Lock();
			struct timer_list *timer = tpool_.Query(id);
			if (timer)
			{
				timer_t *t = (timer_t*)timer;
				if (t->allocated)
				{
					t->parameter.is_canceled = true;
					t->timer_function(t->parameter);

					time_wheel_->del_timer(hTimer_, timer);
					found = true;
					tpool_.Free(timer);
				}
			}
			Unlock();
			return found;
		}

		/**	\brief	时间轮定时系统的主循环
		*	\details	每经过时间轮的一格，查看是否有定时器超时。
		*/
		void Timer::MainLoop()
		{
			Theron::uint64_t last = (::Theron::Detail::Clock::GetTicks() - begin_);


			while (!stop_)
			{
				Theron::uint64_t now = (::Theron::Detail::Clock::GetTicks() - begin_);
				Theron::uint64_t ticks = now - last;

				if (ticks >= threshhold_)
				{
					last = now;
					Theron::uint64_t jiffies = now / tick_per_unit_;

					Lock();
					time_wheel_->run(hTimer_, (unsigned long)jiffies);
					Unlock();
				}
				else
				{

					Theron::Detail::Utils::SleepThread(accuracy_ / 2);
				}
			};
			//now we should release all not timeout one
			for (uint32_t id = 0; id < tpool_.MAXSIZE_COLUME* tpool_.ROW_SIZE; id++)
			{
				Del(id);			
			}

		}
		
		void Timer::EntryPoint(void *const context)
		{
			Timer *This = (Timer *)context;
			This->MainLoop();
		}

		/**	\brief	启动时间轮定时系统
		*/
		bool Timer::Run(bool withSeparateThread/* = true*/)
		{
			stop_ = false;
			if (withSeparateThread)
			{
				thread_.Start(EntryPoint, this);
			}
			else
			{
				MainLoop();
			}
			return true;
		}

    }
}
