#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kernel/task.h>


#define HZ 100
#define TICKS_PER_SEC (1000000/HZ)
// #define TICKS_PER_SEC 10000 /* 100 µs */



int timer_cpu = 0;
splock_t xtime_lock;
uint64_t jiffies = 0; // Number of CPU ticks

uint64_t ticks_last = 0;
uint64_t ticks_elapsed = 0;

time64_t time_us;

time64_t time64()
{
    return time_us;
}

void clock_init()
{
    time_us = cpu_time() * 1000000LL;
    timer_cpu = cpu_no();
    splock_init(&xtime_lock);
    cpu_elapsed(&ticks_last);
}


void advent_timeout();

void sys_ticks()
{
    irq_disable();
    if (timer_cpu == cpu_no()) {
        splock_lock(&xtime_lock);
        time_us += TICKS_PER_SEC;
        ticks_elapsed += cpu_elapsed(&ticks_last);
        jiffies++;
        // Update Wall time
        // Compute global load
        splock_unlock(&xtime_lock);
    }
    advent_timeout();
    task_switch(TS_READY, 0);
}



