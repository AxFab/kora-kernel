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
#include <kernel/memory.h>
#include <kernel/mmu.h>
#include <kora/mcrs.h>
#include "../check.h"

unsigned char mmu_bmp[MMU_LG] = { 0 };

size_t __um_mspace_pages_count = 64;
size_t __um_pages_available = 64;


struct mmu_dir {
    page_t dir;
    page_t *tbl;
    int cnt;
};

struct mmu_dir *kdir;
struct mmu_dir *udir;

// #define MMU_KRN_START  (0x10 * PAGE_SIZE)
// #define MMU_USR_START  (0x10000000)

size_t MMU_USR_START;

/* Request from kernel early code to enable paging */
void mmu_enable()
{
    // List available memory
    page_range(8 * PAGE_SIZE, __um_pages_available * PAGE_SIZE);

    // // Setup allocator initial area
    // setup_allocator((void *)(1536 * _Kib_), 512 * _Kib_);

    kdir = kalloc(sizeof(struct mmu_dir));
    kdir->dir = page_new();
    kdir->cnt = __um_mspace_pages_count;
    kdir->tbl = kalloc(sizeof(page_t) * kdir->cnt);

    kMMU.kspace->lower_bound = (size_t)_valloc(__um_mspace_pages_count * PAGE_SIZE);
    kMMU.kspace->upper_bound = kMMU.kspace->lower_bound + __um_mspace_pages_count *
                               PAGE_SIZE;
    kMMU.kspace->directory = (page_t)kdir;
    kMMU.kspace->a_size += PAGE_SIZE;
    // Active change
    MMU_USR_START = (size_t)_valloc(__um_mspace_pages_count * PAGE_SIZE);
}

void mmu_leave()
{
    _vfree((void *)kMMU.kspace->lower_bound);
    _vfree((void *)MMU_USR_START);
    kfree(kdir->tbl);
    page_release(kdir->dir);
    kfree(kdir);
}

void mmu_context(mspace_t *mspace)
{
    udir = (struct mmu_dir *)mspace->directory;
}

void mmu_create_uspace(mspace_t *mspace)
{
    struct mmu_dir *udir = kalloc(sizeof(struct mmu_dir));
    udir->dir = page_new();
    udir->cnt = __um_mspace_pages_count;
    udir->tbl = kalloc(sizeof(page_t) * udir->cnt);

    mspace->lower_bound = MMU_USR_START;
    mspace->upper_bound = MMU_USR_START + __um_mspace_pages_count * PAGE_SIZE;
    mspace->directory = (page_t)udir;
    mspace->a_size += PAGE_SIZE;
}

void mmu_destroy_uspace(mspace_t *mspace)
{
    struct mmu_dir *udir = (struct mmu_dir *)mspace->directory;
    kfree(udir->tbl);
    page_release(udir->dir);
    kfree(udir);
}


page_t mmu_read(size_t address)
{
    if (address >= kMMU.kspace->lower_bound && address < kMMU.kspace->upper_bound) {
        int pno = (address - kMMU.kspace->lower_bound) / PAGE_SIZE;
        return kdir->tbl[pno] & (~(PAGE_SIZE - 1));
    } else if (address >= MMU_USR_START) {
        int pno = (address - MMU_USR_START) / PAGE_SIZE;
        return udir->tbl[pno] & (~(PAGE_SIZE - 1));
    }
    return 0;
}

page_t mmu_drop(size_t address)
{
    if (address >= kMMU.kspace->lower_bound && address < kMMU.kspace->upper_bound) {
        int pno = (address - kMMU.kspace->lower_bound) / PAGE_SIZE;
        page_t pg = kdir->tbl[pno] & (~(PAGE_SIZE - 1));
        if (pg != 0) {
            kdir->tbl[pno] = 0;
            kMMU.kspace->p_size -= PAGE_SIZE;
            kMMU.kspace->a_size -= PAGE_SIZE;
        }
        return pg;
    } else if (address >= MMU_USR_START) {
        int pno = (address - MMU_USR_START) / PAGE_SIZE;
        page_t pg = udir->tbl[pno] & (~(PAGE_SIZE - 1));
        if (pg != 0) {
            udir->tbl[pno] = 0;
            // uspace->p_size -= PAGE_SIZE;
            // uspace->a_size -= PAGE_SIZE;
        }
        return pg;
    }
    return 0;
}

page_t mmu_protect(size_t address, int access)
{
    int pno = (address - kMMU.kspace->lower_bound) / PAGE_SIZE;
    kdir->tbl[pno] = (kdir->tbl[pno] & (~(PAGE_SIZE - 1))) | access;
    return kdir->tbl[pno] & ~(PAGE_SIZE - 1);
}

int mmu_resolve(size_t address, page_t phys, int access)
{
    int pages = 0;
    if (phys == 0) {
        pages++;
        phys = page_new();
    }
    if (address >= kMMU.kspace->lower_bound && address < kMMU.kspace->upper_bound) {
        kMMU.kspace->p_size += PAGE_SIZE;
        kMMU.kspace->a_size += PAGE_SIZE;
        int pno = (address - kMMU.kspace->lower_bound) / PAGE_SIZE;
        kdir->tbl[pno] = phys | access;
        return pages;
    }
    kpanic("Bad resolve\n");
}

