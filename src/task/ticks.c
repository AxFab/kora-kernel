/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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

uint64_t ticks_last = 0;
uint64_t ticks_elapsed = 0;

clock64_t kclock()
{
    return kSYS.clock_us + kSYS.clock_adj;
}

uint64_t clock_elapsed(uint64_t *last)
{
    uint64_t ticks = cpu_clock();
    uint64_t elapsed = ticks - *last;
    *last =  ticks;
    return elapsed;
}

#define  SEC_PER_DAY (24*60*60)
#define  SEC_PER_HOUR (60*60)
#define  SEC_PER_MIN (60)

int ls = 0;

void clock_ticks()
{
    if (ls++ > HZ) {
        ls = 0;
        clock64_t now = KTIME_TO_SEC(kclock());
        int secs = now % SEC_PER_DAY;
        int sec = (secs % SEC_PER_MIN);
        int min = secs % SEC_PER_HOUR / 60;
        int hour = secs / 3600;
        kprintf(-1, "Hour: %02d:%02d:%02d\n", hour, min, sec);
    }

    // kprintf(-1, "CPU.%d Ticks\n", cpu_no());
    if (kSYS.timer_cpu == cpu_no()) {
        splock_lock(&kSYS.time_lock);
        kSYS.clock_us += KTICKS_PER_SEC;
        ticks_elapsed += clock_elapsed(&ticks_last);
        kSYS.jiffies++;
        splock_unlock(&kSYS.time_lock);
    }
    if (kCPU.flags & CPU_NO_TASK)
        return;
    async_timesup();
    scheduler_switch(TS_READY, 0);
}

void clock_init()
{
    irq_register(0, (irq_handler_t)clock_ticks, NULL);
    kSYS.clock_us = 0;
    kSYS.clock_adj = SEC_TO_KTIME(cpu_time());
    kSYS.timer_cpu = cpu_no();
    splock_init(&kSYS.time_lock);
    clock_elapsed(&ticks_last);
}
