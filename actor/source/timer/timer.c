/*
 *  linux/kernel/timer.c
 *
 *  Kernel internal timers
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  1997-01-28  Modified by Finn Arne Gangstad to make timers scale better.
 *
 *  1997-09-10  Updated NTP code according to technical memorandum Jan '96
 *              "A Kernel Model for Precision Timekeeping" by Dave Mills
 *  1998-12-24  Fixed a xtime SMP race (we need the xtime_lock rw spinlock to
 *              serialize accesses to xtime/lost_ticks).
 *                              Copyright (C) 1998  Andrea Arcangeli
 *  1999-03-10  Improved NTP compatibility by Ulrich Windl
 *  2002-05-31	Move sys_sysinfo here and make its locking sane, Robert Love
 *  2000-10-05  Implemented scalable SMP per-CPU timer handling.
 *                              Copyright (C) 2000, 2001, 2002  Ingo Molnar
 *              Designed by David S. Miller, Alexey Kuznetsov and Ingo Molnar
 */

#include <stdio.h>
#include <malloc.h>

#include "timer.h"

#define BUG_ON(x)
#define likely(x) x
#define unlikely(x) x

#define TIMER_NOT_PINNED 0
#define TIMER_PINNED 1

#define bool int
#define true 1
#define false 0
/*
 * timer vector definitions:
 */
#define CONFIG_BASE_SMALL 0
#define TVN_BITS (CONFIG_BASE_SMALL ? 4 : 6)
#define TVR_BITS (CONFIG_BASE_SMALL ? 6 : 8)
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)
#define MAX_TVAL ((unsigned long)((1ULL << (TVR_BITS + 4 * TVN_BITS)) - 1))

struct tvec
{
    struct list_head vec[TVN_SIZE];
};

struct tvec_root
{
    struct list_head vec[TVR_SIZE];
};

struct tvec_base
{
    //spinlock_t lock;
    struct timer_list *running_timer;
    unsigned long timer_jiffies;
    unsigned long next_timer;
    unsigned long active_timers;
    unsigned long all_timers;

    unsigned long jiffies;//system jiffies, set by user

    struct tvec_root tv1;
    struct tvec tv2;
    struct tvec tv3;
    struct tvec tv4;
    struct tvec tv5;
};




//#define jiffies 0


/**
 * timer_pending - is a timer pending?
 * @timer: the timer in question
 *
 * timer_pending will tell whether a given timer is currently pending,
 * or not. Callers must ensure serialization wrt. other operations done
 * to this timer, eg. interrupt contexts, or other CPUs on SMP.
 *
 * return value: 1 if the timer is pending, 0 if not.
 */
static inline int timer_pending(const struct timer_list *timer)
{
    return timer->entry.next != NULL;
}

/*
 * If the list is empty, catch up ->timer_jiffies to the current time.
 * The caller must hold the tvec_base lock.  Returns true if the list
 * was empty and therefore ->timer_jiffies was updated.
 */
static bool catchup_timer_jiffies(struct tvec_base *base)
{
    if (!base->all_timers)
    {
        base->timer_jiffies = base->jiffies;
        return true;
    }
    return false;
}

static void
__internal_add_timer(struct tvec_base *base, struct timer_list *timer)
{
    unsigned long expires = timer->expires;
    unsigned long idx = expires - base->timer_jiffies;
    struct list_head *vec;

    if (idx < TVR_SIZE)
    {
        int i = expires & TVR_MASK;
        vec = base->tv1.vec + i;
    }
    else if (idx < 1 << (TVR_BITS + TVN_BITS))
    {
        int i = (expires >> TVR_BITS) & TVN_MASK;
        vec = base->tv2.vec + i;
    }
    else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS))
    {
        int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
        vec = base->tv3.vec + i;
    }
    else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS))
    {
        int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
        vec = base->tv4.vec + i;
    }
    else if ((signed long)idx < 0)
    {
        /*
		 * Can happen if you add a timer with expires == jiffies,
		 * or you set a timer to go off in the past
		 */
        vec = base->tv1.vec + (base->timer_jiffies & TVR_MASK);
    }
    else
    {
        int i;
        /* If the timeout is larger than MAX_TVAL (on 64-bit
		 * architectures or with CONFIG_BASE_SMALL=1) then we
		 * use the maximum timeout.
		 */
        if (idx > MAX_TVAL)
        {
            idx = MAX_TVAL;
            expires = idx + base->timer_jiffies;
        }
        i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
        vec = base->tv5.vec + i;
    }
    /*
	 * Timers are FIFO:
	 */
    list_add_tail(&timer->entry, vec);
}

static void internal_add_timer(struct tvec_base *base, struct timer_list *timer)
{
    (void)catchup_timer_jiffies(base);
    __internal_add_timer(base, timer);
    /*
	 * Update base->active_timers and base->next_timer
	 */
    if (!base->active_timers++ ||
        time_before(timer->expires, base->next_timer))
        base->next_timer = timer->expires;
    base->all_timers++;
}


static inline void detach_timer(struct timer_list *timer, bool clear_pending)
{
    struct list_head *entry = &timer->entry;

    __list_del(entry->prev, entry->next);
    if (clear_pending)
        entry->next = NULL;
    entry->prev = NULL;
}

static inline void
detach_expired_timer(struct timer_list *timer, struct tvec_base *base)
{
    detach_timer(timer, true);
    /*	if (!tbase_get_deferrable(timer->base))*/
    base->active_timers--;
    base->all_timers--;
    (void)catchup_timer_jiffies(base);
}

static int detach_if_pending(struct timer_list *timer, struct tvec_base *base,
                             bool clear_pending)
{
    if (!timer_pending(timer))
        return 0;

    detach_timer(timer, clear_pending);
    /*if (!tbase_get_deferrable(timer->base))*/ {
        base->active_timers--;
        if (timer->expires == base->next_timer)
            base->next_timer = base->timer_jiffies;
    }
    base->all_timers--;
    //(void)catchup_timer_jiffies(base);
    return 1;
}


static inline int
__mod_timer(struct timer_list *timer, unsigned long expires,
            bool pending_only, int pinned)
{
    //	struct tvec_base *base/*, *new_base*/;
    //	unsigned long flags;
    int ret = 0 /*, cpu*/;

    /*timer_stats_timer_set_start_info(timer);*/
    BUG_ON(!timer->function);

    //	base = lock_timer_base(timer, &flags);

    ret = detach_if_pending(timer, timer->base, false);
    if (!ret && pending_only)
        goto out_unlock;


    // 	cpu = get_nohz_timer_target(pinned);
    // 	new_base = per_cpu(tvec_bases, cpu);
    //
    // 	if (base != new_base) {
    // 		/*
    // 		 * We are trying to schedule the timer on the local CPU.
    // 		 * However we can't change timer's base while it is running,
    // 		 * otherwise del_timer_sync() can't detect that the timer's
    // 		 * handler yet has not finished. This also guarantees that
    // 		 * the timer is serialized wrt itself.
    // 		 */
    // 		if (likely(base->running_timer != timer)) {
    // 			/* See the comment in lock_timer_base() */
    // 			timer_set_base(timer, NULL);
    // 			spin_unlock(&base->lock);
    // 			base = new_base;
    // 			spin_lock(&base->lock);
    // 			timer_set_base(timer, base);
    // 		}
    // 	}

    timer->expires = expires;
    internal_add_timer(timer->base, timer);

out_unlock:
    //	spin_unlock_irqrestore(&base->lock, flags);

    return ret;
}

/**
 * mod_timer_pending - modify a pending timer's timeout
 * @timer: the pending timer to be modified
 * @expires: new timeout in jiffies
 *
 * mod_timer_pending() is the same for pending timers as mod_timer(),
 * but will not re-activate and modify already deleted timers.
 *
 * It is useful for unserialized use of timers.
 */
int mod_timer_pending(struct timer_list *timer, unsigned long expires)
{
    return __mod_timer(timer, expires, true, TIMER_NOT_PINNED);
}
/*EXPORT_SYMBOL(mod_timer_pending);*/



/**
 * mod_timer - modify a timer's timeout
 * @timer: the timer to be modified
 * @expires: new timeout in jiffies
 *
 * mod_timer() is a more efficient way to update the expire field of an
 * active timer (if the timer is inactive it will be activated)
 *
 * mod_timer(timer, expires) is equivalent to:
 *
 *     del_timer(timer); timer->expires = expires; add_timer(timer);
 *
 * Note that if there are multiple unserialized concurrent users of the
 * same timer, then mod_timer() is the only safe way to modify the timeout,
 * since add_timer() cannot modify an already running timer.
 *
 * The function returns whether it has modified a pending timer or not.
 * (ie. mod_timer() of an inactive timer returns 0, mod_timer() of an
 * active timer returns 1.)
 */
int mod_timer(struct timer_list *timer, unsigned long expires)
{
    /*expires = apply_slack(timer, expires);*/

    /*
	 * This is a common optimization triggered by the
	 * networking code - if the timer is re-modified
	 * to be the same thing then just return:
	 */
    if (timer_pending(timer) && timer->expires == expires)
        return 1;

    return __mod_timer(timer, expires, false, TIMER_NOT_PINNED);
}


/**
 * mod_timer_pinned - modify a timer's timeout
 * @timer: the timer to be modified
 * @expires: new timeout in jiffies
 *
 * mod_timer_pinned() is a way to update the expire field of an
 * active timer (if the timer is inactive it will be activated)
 * and to ensure that the timer is scheduled on the current CPU.
 *
 * Note that this does not prevent the timer from being migrated
 * when the current CPU goes offline.  If this is a problem for
 * you, use CPU-hotplug notifiers to handle it correctly, for
 * example, cancelling the timer when the corresponding CPU goes
 * offline.
 *
 * mod_timer_pinned(timer, expires) is equivalent to:
 *
 *     del_timer(timer); timer->expires = expires; add_timer(timer);
 */
int mod_timer_pinned(struct timer_list *timer, unsigned long expires)
{
    if (timer->expires == expires && timer_pending(timer))
        return 1;

    return __mod_timer(timer, expires, false, TIMER_PINNED);
}


/**
 * add_timer - start a timer
 * @timer: the timer to be added
 *
 * The kernel will do a ->function(->data) callback from the
 * timer interrupt at the ->expires point in the future. The
 * current time is 'jiffies'.
 *
 * The timer's ->expires, ->function (and if the handler uses it, ->data)
 * fields must be set prior calling this function.
 *
 * Timers with an ->expires field in the past will be executed in the next
 * timer tick.
 */
void add_timer(struct timer_list *timer)
{
    BUG_ON(timer_pending(timer));
    mod_timer(timer, timer->expires);
}


/**
 * add_timer_on - start a timer on a particular CPU
 * @timer: the timer to be added
 * @cpu: the CPU to start it on
 *
 * This is not very scalable on SMP. Double adds are not possible.
 */
// void add_timer_on(struct timer_list *timer, int cpu)
// {
// // 	struct tvec_base *base = per_cpu(tvec_bases, cpu);
// // 	unsigned long flags;
// //
// // 	timer_stats_timer_set_start_info(timer);
//  	BUG_ON(timer_pending(timer) || !timer->function);
// // 	spin_lock_irqsave(&base->lock, flags);
// // 	timer_set_base(timer, base);
// // 	debug_activate(timer, timer->expires);
//  	internal_add_timer(timer->base, timer);
// // 	spin_unlock_irqrestore(&base->lock, flags);
// }


/**
 * del_timer - deactive a timer.
 * @timer: the timer to be deactivated
 *
 * del_timer() deactivates a timer - this works on both active and inactive
 * timers.
 *
 * The function returns whether it has deactivated a pending timer or not.
 * (ie. del_timer() of an inactive timer returns 0, del_timer() of an
 * active timer returns 1.)
 */
int del_timer(struct timer_list *timer)
{
    //	struct tvec_base *base;
    //	unsigned long flags;
    int ret = 0;

    //timer_stats_timer_clear_start_info(timer);
    if (timer_pending(timer))
    {
        //base = lock_timer_base(timer, &flags);
        ret = detach_if_pending(timer, timer->base, true);
        //		spin_unlock_irqrestore(&base->lock, flags);
    }

    return ret;
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning (disable : 4311)
#endif
static int cascade(struct tvec_base *base, struct tvec *tv, int index)
{
    /* cascade all the timers from tv up one level */
    struct timer_list *timer, *tmp;
    struct list_head tv_list;

    list_replace_init(tv->vec + index, &tv_list);

    /*
	 * We are removing _all_ timers from the list, so we
	 * don't have to detach them individually.
	 */
    list_for_each_entry_safe(struct timer_list, timer, tmp, &tv_list, entry)
    {
        BUG_ON(tbase_get_base(timer->base) != base);
        /* No accounting, while moving them */
        __internal_add_timer(base, timer);
    }

    return index;
}

static void call_timer_fn(struct timer_list *timer, void (*fn)(void *),
                          void *data)
{
    fn(data);
}

#define INDEX(N) ((base->timer_jiffies >> (TVR_BITS + (N)*TVN_BITS)) & TVN_MASK)

/**
 * run_timers - run all expired timers (if any) on this CPU.
 * @base: the timer vector to be processed.
 *
 * This function cascades all vectors and executes all expired timer
 * vectors.
 */
static inline void run_timers(struct tvec_base *base)
{
    struct timer_list *timer;

    //	spin_lock_irq(&base->lock);
    if (catchup_timer_jiffies(base))
    {
        //	spin_unlock_irq(&base->lock);
        return;
    }
    while (time_after_eq(base->jiffies, base->timer_jiffies))
    {
        struct list_head work_list;
        struct list_head *head = &work_list;
        int index = base->timer_jiffies & TVR_MASK;

        /*
		 * Cascade timers:
		 */
        if (!index &&
            (!cascade(base, &base->tv2, INDEX(0))) &&
            (!cascade(base, &base->tv3, INDEX(1))) &&
            !cascade(base, &base->tv4, INDEX(2)))
        {
            cascade(base, &base->tv5, INDEX(3));
        }

        ++base->timer_jiffies;
        list_replace_init(base->tv1.vec + index, head);

        while (!list_empty(head))
        {

            void (*fn)(void *);
            void *data;
            //			bool irqsafe;

            timer = list_first_entry(head, struct timer_list, entry);
            fn = timer->function;
            data = timer->data;
            //irqsafe = tbase_get_irqsafe(timer->base);

            //			timer_stats_account_timer(timer);

            base->running_timer = timer;
            detach_expired_timer(timer, base);

            // 			if (irqsafe) {
            // 				//spin_unlock(&base->lock);
            // 				call_timer_fn(timer, fn, data);
            // 				//spin_lock(&base->lock);
            // 			} else {
            //spin_unlock_irq(&base->lock);
            call_timer_fn(timer, fn, data);
            //spin_lock_irq(&base->lock);
            //			}
        }
    }
    base->running_timer = NULL;
    //spin_unlock_irq(&base->lock);
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif


static int init_timers(struct tvec_base *base, unsigned long jiffies)
{

    int j;
    for (j = 0; j < TVN_SIZE; j++)
    {
        INIT_LIST_HEAD(base->tv5.vec + j);
        INIT_LIST_HEAD(base->tv4.vec + j);
        INIT_LIST_HEAD(base->tv3.vec + j);
        INIT_LIST_HEAD(base->tv2.vec + j);
    }
    for (j = 0; j < TVR_SIZE; j++)
        INIT_LIST_HEAD(base->tv1.vec + j);

    base->timer_jiffies = jiffies;
    base->next_timer = base->timer_jiffies;
    base->active_timers = 0;
    base->all_timers = 0;
    return 0;
}

static void *__init(unsigned long jiffies)
{
    struct tvec_base *base = (struct tvec_base *)malloc(sizeof(struct tvec_base));
    if (base)
    {
        init_timers(base, jiffies);
    }
    return base;
}

static void __destroy(void *h)
{
    struct tvec_base *base = (struct tvec_base *)h;
    if (base)
    {
        free(base);
    }
}

static int __add_timer(void *h, struct timer_list *timer)
{
    struct tvec_base *base = (struct tvec_base *)h;
    internal_add_timer(base, timer);
    return 0;
}
static int __del_timer(void *h, struct timer_list *timer)
{
    del_timer(timer);
    return 0;
}

static inline void __set_jiffies(void *h, unsigned long jiffies)
{
    struct tvec_base *base = (struct tvec_base *)h;
    base->jiffies = jiffies;
}

static int __run(void *h, unsigned long jiffies)
{
    struct tvec_base *base = (struct tvec_base *)h;
    base->jiffies = jiffies;
    run_timers(base);
    return 0;
}


struct inteface_time_wheel _time_wheel = {
    __init,
    __destroy,
    __add_timer,
    __del_timer,
    __run,
    __set_jiffies};

extern struct inteface_time_wheel *inteface_time_wheel_load_symbol()
{
    return &_time_wheel;
}