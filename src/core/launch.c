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
#include <kernel/stdc.h>
#include <kernel/tasks.h>
#include <kernel/memory.h>
#include <kernel/vfs.h>
#include <kernel/core.h>


/* Kernel entry point, must be reach by a single CPU */
void kernel_start()
{
    irq_reset(false);
    kprintf(KL_MSG, "\033[97mKoraOS\033[0m - " __ARCH " - v" _VTAG_ "\nBuild the " __DATE__ ".\n");

    memory_initialize();
    xtime_t now;
    cpu_setup(&now);
    kprintf(KL_MSG, "\n");

    kprintf(KL_MSG, "\033[94m  Greetings on KoraOS...\033[0m\n");
    clock_init(0, now);
    vfs_t *vfs = vfs_init();
    // Network
    module_init(vfs, kMMU.kspace);

    platform_start();

    scheduler_init(vfs, NULL);
    task_start("Kernel loader #1", module_loader, NULL);
    // task_start("Kernel loader #2", module_loader, NULL);
    // Prepare exec
    // task_start("Kernel init", task_firstinit, NULL);

    irq_reset(true);
}

/* Kernel secondary entry point, must be reach by additional CPUs */
void kernel_ready()
{
    // Setup cpu clock and per-cpu memory
    kprintf(KL_MSG, "\033[32mCpu %d is waiting...\033[0m\n", cpu_no());
    for (;;);
}



