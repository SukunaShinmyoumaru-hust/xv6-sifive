#ifndef __TIMER_H
#define __TIMER_H

#include "types.h"
#include "spinlock.h"
#define TIME_SLICE_PER_SEC 100    // 10 ms
#define MSEC_PER_SEC 1000    // 1s = 1000 ms
#define USEC_PER_SEC 1000000 // 1s = 1000000 us

#define TICK_FREQ 1000000    // 1 MHz   FUF740-C000 for csr time
#define TICK_TO_MS(tick) ((tick) / (TICK_FREQ / MSEC_PER_SEC))
#define TICK_TO_US(tick) ((tick) / (TICK_FREQ / USEC_PER_SEC)) // not accurate 
#define MS_TO_TICK(ms) ((ms) * (TICK_FREQ / MSEC_PER_SEC))
#define SECOND_TO_TICK(sec) ((sec)*TICK_FREQ)

#define CYCLE_FREQ 3000000000    // 3 GHz I guess  for csr cycle
#define CYCLE_TO_MS(cycle) ((cycle) / (CYCLE_FREQ / MSEC_PER_SEC))
#define CYCLE_TO_US(cycle) ((cycle) / (CYCLE_FREQ / USEC_PER_SEC)) 
#define MS_TO_CYCLE(ms) ((ms) * (CYCLE_FREQ / MSEC_PER_SEC))
#define SECOND_TO_CYCLE(sec) ((sec)*CYCLE_FREQ)


#define CLOCK_REALTIME 0

extern struct spinlock tickslock;
extern uint ticks;

struct timespec {
    long    tv_sec;         /* seconds */
    long    tv_nsec;        /* nanoseconds */
};

struct tms {
	uint64 utime;		// user time 
	uint64 stime;		// system time 
	uint64 cutime;		// user time of children 
	uint64 cstime;		// system time of children 
};

#define NSEC_PER_SEC 1000000000LL
#define KTIME_MAX            ((int64)~((uint64)1 << 63)) 
#if (BITS_PER_LONG == 64) 
# define KTIME_SEC_MAX            (KTIME_MAX / NSEC_PER_SEC) 
#else 
# define KTIME_SEC_MAX            LONG_MAX 
#endif

typedef int64 ktime_t;

static inline ktime_t ktime_set(const int64 secs, const unsigned long nsecs)
{
    if (secs >= KTIME_SEC_MAX)
        return KTIME_MAX;

    return secs * NSEC_PER_SEC + (int64)nsecs;
}

static inline int ktime_compare(const ktime_t cmp1, const ktime_t cmp2)
{
    if (cmp1 < cmp2)
        return -1;
    if (cmp1 > cmp2)
        return 1;
    return 0;
}

static inline int ktime_after(const ktime_t cmp1, const ktime_t cmp2)
{
    return ktime_compare(cmp1, cmp2) > 0;
}

static inline int ktime_before(const ktime_t cmp1, const ktime_t cmp2)
{
    return ktime_compare(cmp1, cmp2) < 0;
}


uint64 get_time_ms();
uint64 get_time_us();

void timerinit();
void set_next_timeout();
void timer_tick();

#endif
