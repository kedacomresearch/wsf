

#ifndef _KEDACOM_ASRT_ACTOR_TIMER_TIMERPOOL_H_
#define _KEDACOM_ASRT_ACTOR_TIMER_TIMERPOOL_H_


namespace wsf
{
    namespace actor
    {
        /**	\brief	��ʱ�������
 *
 */
        template <typename T = int>
        class TimerPool
        {
        public:
            typedef T parameter_t;

            /**	\brief	��ʱ������
			 */
            struct timer_t : public timer_list
            {
                parameter_t parameter;                     //!< ��ʱ����ʱ�ص��������
                uint16_t id;                               //!< ��ʱ���ڻ�����е�ID
                void (*timer_function)(parameter_t &param);//!< ��ʱ����ʱ�ص�������ַ
                TimerPool<parameter_t> *pool;              //!< ��ʱ�������Ļ���ص�ַ
				bool        allocated;                     //!< �Ƿ񱻷�����
            };

            static int const MAXSIZE_COLUME = 8;
            static int const ROW_SIZE = 1024;
            //static int const MAXSIZE = 8; // uinit K(1024)

            /**	\brief	����ع��캯��
			 *	\details	{	����һ������أ�����������timers_�С�ÿ������Ԫ��ָ��һ����ʱ�����顣
			 ÿ����ʱ���������ӵ���ʱ������ͷpool_�С�����Ҫ���䶨ʱ��ʱ������Alloc
			 �ӻ����������ȡ��ͷ��ʱ������ʹ���궨ʱ��ʱ������Free����ʱ���������ӵ�
			 ��������β��
			 }
			 */
            TimerPool(int size)
            {
                used_ = 0;
                freed_ = 0;
                INIT_LIST_HEAD(&pool_);
                memset(timers_, 0, sizeof(timers_));

                int col = (size / ROW_SIZE);
                col = col ? col : 1;
                size_ = col * ROW_SIZE;

                for (int i = 0; i < col; i++)
                {
                    timers_[i] = (timer_t *)malloc(sizeof(timer_t) * ROW_SIZE);
                    for (int j = 0; j < ROW_SIZE; j++)
                    {
                        timer_t *timer = timers_[i] + j;
                        list_add_tail(&timer->entry, &pool_);
                        timer->base = NULL;
                        timer->id = ROW_SIZE * i + j;
                        timer->function = NULL;
                        timer->pool = this;
						timer->allocated = false;
                    }
                }
            }

            /**	\brief	�ͷŻ����ռ�õ��ڴ���Դ
			 */
            ~TimerPool()
            {
                for (int i = 0; i < MAXSIZE_COLUME ; i++)
                {
                    if (timers_[i])
                    {
                        free(timers_[i]);
                        timers_[i] = NULL;
                    }
                }
            }
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable : 4311)
#pragma warning (disable : 4302)
#endif
            /**	\brief	��ʱ������
				\details	�Ӷ�ʱ������������ȡ��ͷ��ʱ����
				*/
            timer_list *Alloc()
            {
                if (list_empty(&pool_))
                    return NULL;
                ++used_;
				timer_t *timer = list_first_entry(&pool_, timer_t, entry);
                list_del(&timer->entry);
				timer->allocated = true;;
				new ( & timer->parameter) parameter_t();
                return timer;
            }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
            /**	\brief	���ݶ�ʱ�������ID���ض�ʱ���ĵ�ַ��
			 *	\param	id	��ʱ���ڻ�����е�ID
			 */
            timer_list *Query(int id)
            {
                int index = id / ROW_SIZE;
                int offset = id % ROW_SIZE;
                if (index >= size_)
                    return NULL;
                return &timers_[index][offset];
            }


            /**	\brief	��ʱ���ͷ�
				\details	����ʱ���������ӵ���������β��
				*/
            void Free(struct timer_list *timer)
            {
				timer_t* t = static_cast<timer_t*>(timer);
				t->allocated = false;
				t->parameter.~parameter_t();
                ++freed_;
                list_add_tail(&timer->entry, &pool_);
				
            }

            timer_t *timers_[MAXSIZE_COLUME];//!<	��ʱ������أ�ÿ������Ԫ��ָ���СΪ1024��������
            int size_;                       //!<	��ʱ����������õĿռ䣨��λ1024����Ĭ��Ϊ2
            list_head pool_;                 //!<	��ʱ������ͷ
            int used_;
            int freed_;
        };
    }
}

#endif
