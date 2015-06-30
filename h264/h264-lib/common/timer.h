#ifndef _TIMER_H_
#define _TIMER_H_

#include "portab.h"

typedef struct  
{
    int64_t all;
    int64_t start;
    int64_t overhead;
} timer_st;

#ifdef ENABLE_PROFILE

static __inline void
start_timer(timer_st* t)
{
    t->start = read_counter();
}

static __inline void
stop_timer_all(timer_st* t)
{
    t->all += read_counter() - t->start - t->overhead;
}

static __inline void 
init_timer(timer_st* t)
{
    memset(t, 0, sizeof(*t));
    start_timer(t);
    t->overhead = read_counter() - t->start;
}

#else // ENABLE_PROFILE

static __inline void
start_timer(timer_st* t)
{
}

static __inline void
stop_timer_all(timer_st* t)
{
}

static __inline void 
init_timer(timer_st* t)
{
}

#endif
#endif 