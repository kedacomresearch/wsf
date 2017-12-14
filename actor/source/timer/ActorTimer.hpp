/** \brief 添加删除定时器。定时器超时后会执行回调函数。
 *	\details	{ 主要功能：
 1.	启动时间轮定时系统
 2.	在时间轮定时系统中设置定时器
 3.	删除已经设置的定时器
 *				}
 *
 */

#ifndef _KEDACOM_ASRT_ACTOR_TIMER_TIMER_H_
#define _KEDACOM_ASRT_ACTOR_TIMER_TIMER_H_
#include <Theron/Detail/Threading/Thread.h>
#include <Theron/Detail/Threading/Mutex.h>
#include <Theron/Detail/Threading/Clock.h>
#include <wsf/actor.hpp>



#include "timer.h"


#include "TimerPool.hpp"
namespace wsf
{
    namespace actor
    {

        /**	\brief	定时器类型定义
		 */
        class Timer
        {

        public:

			static inline Timer&  instance() {
				assert(instance_);
				return *instance_;
            }
            virtual ~Timer(){}

            enum
            {
                INVALID_TIMERID = 0xFFFFFFFF
            };
            /**	\brief	定时器回调函数入参类型
			 */
            struct parameter_t
            {
                parameter_t()
                    : /*user_info(NULL),*/ id(0)
					, is_canceled(false)

                {
                }
                ::Theron::Address actor;//!< 设置定时器的Actor地址
				std::function< void(bool)> fn;

                uint32_t id;//!< 定时器虚ID
				bool is_canceled;
            };

            /**	\typedef	定时器类型
			 */
            typedef TimerPool<parameter_t>::timer_t timer_t;

            /**	\brief	定时器构造函数
			 *	\details	初始化定时器缓存池、加载时间轮、初始化时间轮精度
			 */
			Timer(int maxsize = 8 * 1024);

            /**	\brief	定时器初始化
			 *	\details	初始化时间轮定时系统初始化时刻、时间轮转动一格需要经过的钟摆和钟摆阈值、
			 初始化时间轮和时间轮信息头
			 */
			virtual bool Initialize();

            /**	\brief	定时器释放
			 *	\details	销毁时间轮信息头和时间轮
			 *	\bug	退出主循环 stop_ = false;
			 */
			virtual void Terminate();



            /**	\brief	添加一个定时器
			 *	\details	从定时器缓存链表中取出一个定时器，进行初始化后，返回定时器在链表中的ID
			 *	\param	timeout 超时时长（ms）
			 *	\param	fn	超时回调函数
			 *	\param	param	超时回调函数入参
			 *	\return	定时器在链表中的ID
			 */
			virtual uint32_t Add(uint32_t timeout, /*void (*fn)(unsigned long)*/ void(*fn)(parameter_t &param), parameter_t &param);

            /**	\brief 根据定时器缓存池ID删除定时器
			 *	\details	首先根据缓存池ID得到定时器，然后从时间轮中删除定时器，最后将定时器归还缓存池。
			 */
			virtual bool Del(uint32_t id);

            /**	\brief	时间轮定时系统的主循环
			 *	\details	每经过时间轮的一格，查看是否有定时器超时。
			 */
			virtual void MainLoop();

			static void EntryPoint(void *const context);

            /**	\brief	启动时间轮定时系统
			 */
			virtual bool Run(bool withSeparateThread = true);
            
			inline void Lock()
            {
                lock_.Lock();
            }

            inline void Unlock()
            {
                lock_.Unlock();
            }
			static Timer* instance_;

        protected:
            bool stop_;
            Theron::uint32_t accuracy_;     //!<	时间轮精度
            Theron::uint64_t tick_per_unit_;//!<	时间轮转动一格经过的计算机时钟摆
            Theron::uint64_t threshhold_;   //!<	时间轮转动一格经过的阈值计算机时钟摆（考虑误差）
            Theron::uint64_t begin_;        //!<	时间轮定时系统初始化时刻
            Theron::Detail::Thread thread_; //!<	线程对象
            void *hTimer_;                  //!<	时间轮信息头
            inteface_time_wheel *time_wheel_; //!<	时间轮
            TimerPool<parameter_t> tpool_;  //!<	定时器缓存池
            ::Theron::Detail::Mutex lock_;  //!<	互斥锁
        };
    }
}

#endif//!
