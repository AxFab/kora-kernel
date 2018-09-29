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
#include <errno.h>

void cpu_stack(task_t *task, size_t entry, size_t param)
{
    size_t *stack = task->kstack + (task->kstack_len / sizeof(size_t));
    task->state[5] = entry;
    task->state[3] = (size_t)stack;

    stack--;
    *stack = param;
    stack--;
    *stack = (size_t)kexit;
    task->state[4] = (size_t)stack;
}

void cpu_shutdown(int cmd) // REBOOT, POWER_OFF, SLEEP, DEEP_SLEEP, HALT
{
    switch (cmd) {
    // case REBOOT:
    //     while ((inb(0x64) & 2));
    //     outb(0x64, 0xF4);
    //     for(;;);
    default:
        errno = ENOSYS;
        return;
    }
}

