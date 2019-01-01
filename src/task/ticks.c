/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
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


uint64_t time_elapsed(uint64_t *last)
{
    uint64_t ticks = cpu_clock();
    uint64_t elapsed = ticks - *last;
    *last =  ticks;
    return elapsed;
}

void sys_ticks()
{
    // kprintf(-1, "CPU.%d Ticks\n", cpu_no());

    // irq_disable();
    if (timer_cpu == cpu_no()) {
        splock_lock(&xtime_lock);
        time_us += TICKS_PER_SEC;
        ticks_elapsed += time_elapsed(&ticks_last);
        jiffies++;
        // Update Wall time
        // Compute global load
        splock_unlock(&xtime_lock);
    }
    if (kCPU.flags & CPU_NO_TASK)
        return;
    async_timesup();
    scheduler_switch(TS_READY, 0);
}

void clock_init()
{
    irq_register(0, (irq_handler_t)sys_ticks, NULL);

    time_us = cpu_time() * 1000000LL;
    timer_cpu = cpu_no();
    splock_init(&xtime_lock);
    time_elapsed(&ticks_last);
}

