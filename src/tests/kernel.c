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


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
KMODULE(imgdk);
KMODULE(isofs);
KMODULE(fatfs);
KMODULE(lnet);

void platform_setup()
{
    // Load fake disks drivers
    // task_create(kernel_module, &kmod_info_imgdk, kmod_info_imgdk.name);
    // Load fake network driver
    // task_create(kernel_module, &kmod_info_lnet, kmod_info_lnet.name);
    // Load fake screen

    // Load file systems
    // task_create(kernel_module, &kmod_info_isofs, kmod_info_isofs.name);
    // task_create(kernel_module, &kmod_info_fatfs, kmod_info_fatfs.name);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#include <setjmp.h>
#include <windows.h>

jmp_buf __tcase_jump;

int main()
{
    kernel_start();
    assert(kCPU.irq_semaphore == 0);
    kCPU.flags |= CPU_NO_TASK;
    if (setjmp(__tcase_jump) != 0)
        return -1;
    for (;;) {
        clock_ticks();
        async_timesup();
        Sleep(10);
    }
    return 0;
}

