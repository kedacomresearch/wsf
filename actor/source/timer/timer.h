#ifndef _LINUX_TIMER_H
#define _LINUX_TIMER_H

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	These inlines deal with timer wrapping correctly. You are 
 *	strongly encouraged to use them
 *	1. Because people otherwise forget
 *	2. Because if the timer wrap changes in future you won't have to
 *	   alter your driver code.
 *
 * time_after(a,b) returns true if the time a is after time b.
 *
 * Do this with "<0" and ">=0" to only 
 the sign of the result. A
 * good compiler would generate better code (and a really good compiler
 * wouldn't care). Gcc is currently neither.
 */
#define time_after(a, b) ((long)((b) - (a)) < 0)
#define time_before(a, b) time_after(b, a)

#define time_after_eq(a, b) ((long)((a) - (b)) >= 0)
#define time_before_eq(a, b) time_after_eq(b, a)

/*
 * Calculate whether a is in the range of [b, c].
 */
#define time_in_range(a, b, c) \
    (time_after_eq(a, b) &&    \
     time_before_eq(a, c))

/*
 * Calculate whether a is in the range of [b, c).
 */
#define time_in_range_open(a, b, c) \
    (time_after_eq(a, b) &&         \
     time_before(a, c))

struct tvec_base;

struct timer_list
{
    /*
	 * All fields that change during normal runtime grouped to the
	 * same cacheline
	 */
    struct list_head entry;
    unsigned long expires;
    struct tvec_base *base;

    //void (*function)(unsigned long);
    //unsigned long data;
    void (*function)(void *);
    void *data;
    struct marker_t
    {
        int pending : 1;
        int id : 14;

    } marker;
};



struct inteface_time_wheel
{
    /**
	 * init - Initialize time wheel instance
	 * @timer: timer do del
	 * @jiffies: init jiffies
	 * This function will alloc timer resource and return the handle
	 * user should call destroy with above handle
	 */
    void *(*init)(unsigned long jiffies);
    void (*destroy)(void *h);

    int (*add_timer)(void *h, struct timer_list *timer);
    int (*del_timer)(void *h, struct timer_list *timer);


    int (*run)(void *h, unsigned long jiffies);
    void (*set_jiffies)(void *h, unsigned long jiffies);
};
extern struct inteface_time_wheel *inteface_time_wheel_load_symbol();

#ifdef __cplusplus
};
#endif

#endif
