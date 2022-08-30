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
#include <kernel/tasks.h>
#include <kernel/memory.h>
#include <kernel/vfs.h>
#include <kernel/net.h>
#include <kernel/core.h>
#include <bits/atomic.h>

void module_init(fs_anchor_t *fsanchor, mspace_t *vm);


sys_info_t sysinfo;
#ifndef _VTAG_
#define _VTAG_  "v0.0"
#endif

/* Kernel entry point, must be reach by a single CPU */
void kstart()
{
    irq_reset(false /*, KMODE_SYSTEM */);
    memset(&sysinfo, 0, sizeof(sys_info_t));
    kprintf(-1, "\033[97mKoraOS\033[0m - " __ARCH " - " _VTAG_ "\n");
    kprintf(-1, "Build the " __DATE__ ".\n");
    cpu_setup(&sysinfo);

    kprintf(-1, "\n\033[94m  Greetings on KoraOS...\033[0m\n");

    // Kernel initialization
    clock_init(sysinfo.uptime);
    fs_anchor_t *fsanchor = vfs_init();
    scheduler_init(/*fsanchor, NULL*/);
    net_setup();

    // Read Symbols
    module_init(fsanchor, kMMU.kspace);

    arch_init();

    for (int i = 0, n = 2; i < n; ++i) {
        char tmp[32];
        snprintf(tmp, 32, "Kernel loader #%d", i + 1);
        task_start(tmp, module_loader, NULL);
    }

    sysinfo.is_ready = 1;
    irq_zero();
}

/* Kernel secondary entry point, must be reach by additional CPUs */
void kready()
{
    irq_reset(false);
    cpu_setup(&sysinfo);
    kprintf(-1, "\033[32mCpu %d ready and waiting...\033[0m\n", cpu_no());

    while (sysinfo.is_ready == 0)
        atomic_break();
    irq_zero();
}

sys_info_t *ksys()
{
    return &sysinfo;
}

cpu_info_t *kcpu()
{
    if (sysinfo.is_ready == 0)
        return NULL;
    int no = cpu_no();
    return &sysinfo.cpu_table[no];
}

