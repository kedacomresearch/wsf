/** \brief ���ɾ����ʱ������ʱ����ʱ���ִ�лص�������
 *	\details	{ ��Ҫ���ܣ�
 1.	����ʱ���ֶ�ʱϵͳ
 2.	��ʱ���ֶ�ʱϵͳ�����ö�ʱ��
 3.	ɾ���Ѿ����õĶ�ʱ��
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

        /**	\brief	��ʱ�����Ͷ���
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
            /**	\brief	��ʱ���ص������������
			 */
            struct parameter_t
            {
                parameter_t()
                    : /*user_info(NULL),*/ id(0)
					, is_canceled(false)

                {
                }
                ::Theron::Address actor;//!< ���ö�ʱ����Actor��ַ
				std::function< void(bool)> fn;

                uint32_t id;//!< ��ʱ����ID
				bool is_canceled;
            };

            /**	\typedef	��ʱ������
			 */
            typedef TimerPool<parameter_t>::timer_t timer_t;

            /**	\brief	��ʱ�����캯��
			 *	\details	��ʼ����ʱ������ء�����ʱ���֡���ʼ��ʱ���־���
			 */
			Timer(int maxsize = 8 * 1024);

            /**	\brief	��ʱ����ʼ��
			 *	\details	��ʼ��ʱ���ֶ�ʱϵͳ��ʼ��ʱ�̡�ʱ����ת��һ����Ҫ�������Ӱں��Ӱ���ֵ��
			 ��ʼ��ʱ���ֺ�ʱ������Ϣͷ
			 */
			virtual bool Initialize();

            /**	\brief	��ʱ���ͷ�
			 *	\details	����ʱ������Ϣͷ��ʱ����
			 *	\bug	�˳���ѭ�� stop_ = false;
			 */
			virtual void Terminate();



            /**	\brief	���һ����ʱ��
			 *	\details	�Ӷ�ʱ������������ȡ��һ����ʱ�������г�ʼ���󣬷��ض�ʱ���������е�ID
			 *	\param	timeout ��ʱʱ����ms��
			 *	\param	fn	��ʱ�ص�����
			 *	\param	param	��ʱ�ص��������
			 *	\return	��ʱ���������е�ID
			 */
			virtual uint32_t Add(uint32_t timeout, /*void (*fn)(unsigned long)*/ void(*fn)(parameter_t &param), parameter_t &param);

            /**	\brief ���ݶ�ʱ�������IDɾ����ʱ��
			 *	\details	���ȸ��ݻ����ID�õ���ʱ����Ȼ���ʱ������ɾ����ʱ������󽫶�ʱ���黹����ء�
			 */
			virtual bool Del(uint32_t id);

            /**	\brief	ʱ���ֶ�ʱϵͳ����ѭ��
			 *	\details	ÿ����ʱ���ֵ�һ�񣬲鿴�Ƿ��ж�ʱ����ʱ��
			 */
			virtual void MainLoop();

			static void EntryPoint(void *const context);

            /**	\brief	����ʱ���ֶ�ʱϵͳ
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
            Theron::uint32_t accuracy_;     //!<	ʱ���־���
            Theron::uint64_t tick_per_unit_;//!<	ʱ����ת��һ�񾭹��ļ����ʱ�Ӱ�
            Theron::uint64_t threshhold_;   //!<	ʱ����ת��һ�񾭹�����ֵ�����ʱ�Ӱڣ�������
            Theron::uint64_t begin_;        //!<	ʱ���ֶ�ʱϵͳ��ʼ��ʱ��
            Theron::Detail::Thread thread_; //!<	�̶߳���
            void *hTimer_;                  //!<	ʱ������Ϣͷ
            inteface_time_wheel *time_wheel_; //!<	ʱ����
            TimerPool<parameter_t> tpool_;  //!<	��ʱ�������
            ::Theron::Detail::Mutex lock_;  //!<	������
        };
    }
}

#endif//!
