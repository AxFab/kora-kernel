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
#include <time.h>

void cpu_awake()
{
    kprintf(KLOG_MSG, "CPU: %s, Single CPU\n", "HostSimulatedCPU");
}

uint64_t cpu_clock()
{
    static uint64_t ticks = 0;
    return ++ticks;
}

int cpu_no()
{
    return 0;
}

void cpu_run() {}
void cpu_setup_signal() {}
void cpu_setup_task() {}
void cpu_task_return_uspace() {}

time_t cpu_time()
{
    return time(NULL);
}

void cpu_reboot()
{
}



void cpu_save_task(task_t *task)
{

}

