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

void mmu_enable() {}
void mmu_leave() {}
page_t mmu_read(size_t addr)
{
    return 0;
}
page_t mmu_drop(size_t addr)
{
    return 0;
}
page_t mmu_protect(size_t addr, int flags)
{
    return 0;
}
page_t mmu_resolve(size_t addr, page_t phys, int flags)
{
    return 0;
}
void mmu_context(mspace_t *mspace) {}
void mmu_create_uspace(mspace_t *mspace) {}
void mmu_destroy_uspace(mspace_t *mspace) {}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void cpu_halt() {}
time64_t cpu_clock()
{
    return 0;
}
time_t cpu_time()
{
    return 0;
}
void cpu_setup()
{
    kSYS.cpus = kalloc(sizeof(struct kCpu) * 32);
    kSYS.cpus[0].irq_semaphore = 1;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void platform_setup()
{

}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#include <windows.h>
int main()
{
    kernel_start();
    assert(kCPU.irq_semaphore == 0);
    for (;;) {
        sys_ticks();
        async_timesup();
        Sleep(10);
    }
    return 0;
}

