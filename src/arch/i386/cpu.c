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

_Noreturn void cpu_halt_x86(size_t stack, void *tss);

int cpu_no();
int cpu_save(cpu_state_t*);
void cpu_restore();


_Noreturn void cpu_halt()
{
    assert(kCPU.irq_semaphore == 0);
    cpu_halt_x86();
}

void cpu_stack(task_t, void*, void*);
void cpu_shutdown(); // REBOOT, POWER_OFF, SLEEP, DEEP_SLEEP, HALT
