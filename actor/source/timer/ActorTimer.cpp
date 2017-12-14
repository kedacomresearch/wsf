#include "ActorTimer.hpp"




namespace wsf
{
    namespace actor
    {

		Timer* Timer::instance_ = NULL;
		/**	\brief	��ʱ�����캯��
		*	\details	��ʼ����ʱ������ء�����ʱ���֡���ʼ��ʱ���־���
		*/
		Timer::Timer(int maxsize/* = 8 * 1024*/)
			: tpool_(maxsize)
		{
			time_wheel_ = inteface_time_wheel_load_symbol();

			accuracy_ = 40;//40ms
		}

		/**	\brief	��ʱ����ʼ��
		*	\details	��ʼ��ʱ���ֶ�ʱϵͳ��ʼ��ʱ�̡�ʱ����ת��һ����Ҫ�������Ӱں��Ӱ���ֵ��
		��ʼ��ʱ���ֺ�ʱ������Ϣͷ
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

		/**	\brief	��ʱ���ͷ�
		*	\details	����ʱ������Ϣͷ��ʱ����
		*	\bug	�˳���ѭ�� stop_ = false;
		*/
		void Timer::Terminate()
		{
			stop_ = true;
			thread_.Join();
			time_wheel_->destroy(hTimer_);
		}


		/**	\brief	�ⶨʱ����ʱ�ص�����
		*	\details	�����û����õ��ڶ�ʱ����ʱ�ص������󣬽���ʱ���ͷŻػ�������
		*	\param	data ��ʱ����ַ
		*/
		static void timer_callback(void *data)
		{
			Timer::timer_t *timer = (Timer::timer_t *)data;
			timer->timer_function(timer->parameter);
			timer->pool->Free(timer);
		}

		/**	\brief	���һ����ʱ��
		*	\details	�Ӷ�ʱ������������ȡ��һ����ʱ�������г�ʼ���󣬷��ض�ʱ���������е�ID
		*	\param	timeout ��ʱʱ����ms��
		*	\param	fn	��ʱ�ص�����
		*	\param	param	��ʱ�ص��������
		*	\return	��ʱ���������е�ID
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
				timer->expires = expires;        //!<	���ô�ʱ���ֶ�ʱϵͳ��ʼ������ʱ�ĳ�ʱ����ʱ��
				timer->function = timer_callback;//!<	�����ⳬʱ�ص���������ʱ��ʱ���ֵ���
				timer->timer_function = fn;      //!<	�����ڳ�ʱ�ص��������û����ú󣬱��ⳬʱ�ص���������
				timer->data = timer;             //!<	�����ⳬʱ�ص�������Σ����ڶ�ʱ���ĵ�ַ
				param.id = (uint32_t)timer->id;
				param.is_canceled = false;
				timer->parameter = param;        //!<	�����ڳ�ʱ�ص�������Σ������û���ַ

				timer->base = (tvec_base *)hTimer_;
				time_wheel_->add_timer(hTimer_, timer);
			}
			Unlock();
			return (uint32_t)timer->id;
		}

		/**	\brief ���ݶ�ʱ�������IDɾ����ʱ��
		*	\details	���ȸ��ݻ����ID�õ���ʱ����Ȼ���ʱ������ɾ����ʱ������󽫶�ʱ���黹����ء�
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

		/**	\brief	ʱ���ֶ�ʱϵͳ����ѭ��
		*	\details	ÿ����ʱ���ֵ�һ�񣬲鿴�Ƿ��ж�ʱ����ʱ��
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

		/**	\brief	����ʱ���ֶ�ʱϵͳ
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
