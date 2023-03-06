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
#include <kernel/memory.h>
// #include <kernel/cpu.h>
// #include <kernel/vfs.h>
#include <kora/mcrs.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <kernel/dlib.h>


vmsp_t kernel_space;
struct kMmu __mmu;

void memory_initialize()
{
    __mmu.max_vma_size = _Gib_;
    __mmu.upper_physical_page = 0;
    __mmu.pages_amount = 0;
    __mmu.free_pages = 0;
    __mmu.page_size = PAGE_SIZE;

    /* Init Kernel memory space structure */
    memset(&kernel_space, 0, sizeof(kernel_space));
    bbtree_init(&kernel_space.tree);
    splock_init(&kernel_space.lock);
    kernel_space.max_size = VMSP_MAX_SIZE;
    __mmu.kspace = &kernel_space;

    /* Enable MMU */
    mmu_enable();

    char tmp[20];
    kprintf(KL_MSG, "Memory available %s\n", sztoa_r(__mmu.pages_amount * PAGE_SIZE, tmp));
}

void memory_sweep()
{
    vmsp_sweep(__mmu.kspace);
    mmu_leave();
    page_teardown();

}

vmsp_t *memory_space_at(size_t address)
{
    if (address >= __mmu.kspace->lower_bound && address < __mmu.kspace->upper_bound)
        return __mmu.kspace;
    if (__mmu.uspace == NULL)
        return NULL;
    if (address >= __mmu.uspace->lower_bound && address < __mmu.uspace->upper_bound)
        return __mmu.uspace;
    return NULL;
}

void memory_info()
{
    kprintf(KL_DBG, "MemTotal:      %9s (%dK)\n", sztoa(__mmu.upper_physical_page * PAGE_SIZE), __mmu.upper_physical_page * 4);
    kprintf(KL_DBG, "MemFree:       %9s (%dK)\n", sztoa(__mmu.free_pages * PAGE_SIZE), __mmu.free_pages * 4);
    kprintf(KL_DBG, "MemAvailable:  %9s (%dK)\n", sztoa(__mmu.pages_amount * PAGE_SIZE), __mmu.pages_amount * 4);
    kprintf(KL_DBG, "MemDetected:   %9s (%dK)\n", sztoa(__mmu.upper_physical_page * PAGE_SIZE), __mmu.upper_physical_page * 4);
    kprintf(KL_DBG, "MemUsed:       %9s (%dK)\n", sztoa((__mmu.pages_amount - __mmu.free_pages) * PAGE_SIZE), (__mmu.pages_amount - __mmu.free_pages) * 4);
}

// Buffers:           53664 kB
// Cached:           862688 kB
// SwapCached:            0 kB
// Active:           879112 kB
// Inactive:         768040 kB
// Active(anon):     693192 kB
// Inactive(anon):   237184 kB
// Active(file):     185920 kB
// Inactive(file):   530856 kB
// Unevictable:          32 kB
// Mlocked:              32 kB
// SwapTotal:       4192252 kB
// SwapFree:        4192252 kB
// Dirty:                 0 kB
// Writeback:             0 kB
// AnonPages:        730876 kB
// Mapped:           224552 kB
// Shmem:            199580 kB
// Slab:              80548 kB
// SReclaimable:      51596 kB
// SUnreclaim:        28952 kB
// KernelStack:        5680 kB
// PageTables:        19408 kB
// NFS_Unstable:          0 kB
// Bounce:                0 kB
// WritebackTmp:          0 kB
// CommitLimit:     8241420 kB
// Committed_AS:    2872492 kB
// VmallocTotal:   34359738367 kB
// VmallocUsed:           0 kB
// VmallocChunk:          0 kB
// HardwareCorrupted:     0 kB
// AnonHugePages:         0 kB
// ShmemHugePages:        0 kB
// ShmemPmdMapped:        0 kB
// HugePages_Total:       0
// HugePages_Free:        0
// HugePages_Rsvd:        0
// HugePages_Surp:        0
// Hugepagesize:       2048 kB
// DirectMap4k:       78604 kB
// DirectMap2M:     8228864 kB
