/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#include <kernel/memory.h>
#include <kora/mcrs.h>
#include <stdlib.h>
#include <time.h>


time_t cpu_time()
{
    return time(NULL);
}

/* Request to start other CPUs */
void cpu_awake()
{
}

int cpu_no()
{
    return 0;
}

uint64_t cpu_elapsed(uint64_t *last)
{
    return 1;
}


_Noreturn void cpu_run()
{
    abort();
}

_Noreturn void cpu_halt()
{
    abort();
}

void cpu_setup_task()
{
    abort();
}
void cpu_setup_signal()
{
    abort();
}

bool cpu_task_return_uspace()
{
    abort();
    return false;
}

struct kSys kSYS;
struct kCpu kCPU0;
