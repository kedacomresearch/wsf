

#ifndef _KEDACOM_ASRT_ACTOR_TIMER_TIMERPOOL_H_
#define _KEDACOM_ASRT_ACTOR_TIMER_TIMERPOOL_H_


namespace wsf
{
    namespace actor
    {
        /**	\brief	定时器缓存池
 *
 */
        template <typename T = int>
        class TimerPool
        {
        public:
            typedef T parameter_t;

            /**	\brief	定时器类型
			 */
            struct timer_t : public timer_list
            {
                parameter_t parameter;                     //!< 定时器超时回调函数入参
                uint16_t id;                               //!< 定时器在缓存池中的ID
                void (*timer_function)(parameter_t &param);//!< 定时器超时回调函数地址
                TimerPool<parameter_t> *pool;              //!< 定时器所属的缓存池地址
				bool        allocated;                     //!< 是否被分配了
            };

            static int const MAXSIZE_COLUME = 8;
            static int const ROW_SIZE = 1024;
            //static int const MAXSIZE = 8; // uinit K(1024)

            /**	\brief	缓存池构造函数
			 *	\details	{	构造一个缓存池，保存在数组timers_中。每个数组元素指向一个定时器数组。
			 每个定时器都被链接到定时器链表头pool_中。当需要分配定时器时，调用Alloc
			 从缓存池链表中取出头定时器。当使用完定时器时，调用Free将定时器重新链接到
			 缓存链表尾。
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

            /**	\brief	释放缓存池占用的内存资源
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
            /**	\brief	定时器分配
				\details	从定时器缓存链表中取出头定时器。
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
            /**	\brief	根据定时器缓存池ID返回定时器的地址。
			 *	\param	id	定时器在缓存池中的ID
			 */
            timer_list *Query(int id)
            {
                int index = id / ROW_SIZE;
                int offset = id % ROW_SIZE;
                if (index >= size_)
                    return NULL;
                return &timers_[index][offset];
            }


            /**	\brief	定时器释放
				\details	将定时器重新链接到缓存链表尾。
				*/
            void Free(struct timer_list *timer)
            {
				timer_t* t = static_cast<timer_t*>(timer);
				t->allocated = false;
				t->parameter.~parameter_t();
                ++freed_;
                list_add_tail(&timer->entry, &pool_);
				
            }

            timer_t *timers_[MAXSIZE_COLUME];//!<	定时器缓存池，每个数组元素指向大小为1024的子数组
            int size_;                       //!<	定时器缓存池启用的空间（单位1024），默认为2
            list_head pool_;                 //!<	定时器链表头
            int used_;
            int freed_;
        };
    }
}

#endif
