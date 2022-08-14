// Timer Interrupt handler


#include "include/types.h"
#include "include/param.h"
#include "include/riscv.h"
#include "include/sbi.h"
#include "include/spinlock.h"
#include "include/timer.h"
#include "include/printf.h"
#include "include/proc.h"

struct spinlock tickslock;
uint ticks;

void timerinit() {
    initlock(&tickslock, "time");
    ticks = 0;
    #ifdef DEBUG
    printf("timerinit\n");
    #endif
}

void
set_next_timeout() {
    // There is a very strange bug,
    // if comment the `printf` line below
    // the timer will not work.

    // this bug seems to disappear automatically
    // printf("");
    set_timer(r_time() + INTERVAL);
}

void timer_tick() {
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
    set_next_timeout();
}

uint64 get_ticks()
{
    acquire(&tickslock);
    uint64 ret = ticks;
    release(&tickslock);
    return ret;
}

uint64 get_time_ms() {
    uint64 time = r_time();
    return time / (TICK_FREQ / MSEC_PER_SEC);
}

uint64 get_time_us() {
    return r_time() * USEC_PER_SEC / TICK_FREQ;
}

struct timeval get_timeval(){
   uint64 time = r_time();
   return (struct timeval){
     .sec = time / (TICK_FREQ),
     .usec = time / (TICK_FREQ / MSEC_PER_SEC),
   };
}
