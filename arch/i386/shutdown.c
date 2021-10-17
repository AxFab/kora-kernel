/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#include <kernel/stdc.h>
#include <kernel/arch.h>
#include <kernel/tasks.h>
#include <errno.h>


// void cpu_jmpbuf(cpu_state_t jbuf, void *stack, void* entry, void *arg)
// {
//     size_t *sptr = (size_t*)stack + (4096 / sizeof(size_t));
//     memset(jbuf, 0, sizeof(jbuf));
//     jbuf[5] = entry;
//     jbuf[3] = (size_t)sptr;
//     sptr--;
//     *sptr = arg;
//     jbuf[4] = (size_t)stack;
// }

void cpu_setjmp(cpu_state_t *jmpbuf, void *stack, void *entry, void *param)
{
    size_t len = (KSTACK_PAGES * PAGE_SIZE) - sizeof(size_t);
    size_t *sptr = (void *)((size_t)stack + len);
    (*jmpbuf)[5] = (size_t)entry;
    (*jmpbuf)[3] = (size_t)sptr;
    (*jmpbuf)[6] = (size_t)sptr;

    sptr--;
    *sptr = (size_t)param;
    sptr--;
    (*jmpbuf)[4] = (size_t)sptr;
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
