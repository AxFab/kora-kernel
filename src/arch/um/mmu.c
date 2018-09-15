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
#include <kernel/memory.h>
#include <kora/mcrs.h>
#include <assert.h>
#include <errno.h>
#include <string.h>



// UserSpace have 512 first Mb (-1) (131072 Pages - Table of 512 Kb)
// KernelSpace have 16 Mb after 1 Gb (4096 Pages - Table of 16 Kb)

uint8_t mmu_bmp[MMU_LG] = { 0 };

uint32_t ktable[(16 * _Kib_) / 4] = { 0 };

mspace_t *user_space;

/* - */
void mmu_enable()
{
    page_range(12 * _Kib_, 64 * _Kib_);
    page_range(128 * _Kib_, 96 * _Kib_);

    kMMU.kspace->lower_bound = (size_t)valloc(16 * _Mib_);
    kMMU.kspace->upper_bound = kMMU.kspace->lower_bound + 16 * _Mib_;
}

void mmu_disable()
{
    free(kMMU.kspace->lower_bound);
}

/* - */
page_t mmu_resolve(size_t vaddress, page_t paddress, int access, bool clean)
{
    uint32_t *dir;
    int page_no;
    if (vaddress >= kMMU.kspace->lower_bound &&
        vaddress < kMMU.kspace->upper_bound) {
        dir = ktable;
        page_no = (vaddress - kMMU.kspace->lower_bound) / PAGE_SIZE;
    } else if (vaddress >= user_space->lower_bound &&
               vaddress < user_space->upper_bound) {
        dir = (uint32_t *)user_space->directory;
        page_no = (vaddress - user_space->lower_bound) / PAGE_SIZE;
    }

    // assert(dir[page_no] == 0);
    if (paddress == 0)
        paddress = page_new();
    dir[page_no] = paddress | (access & 7);
    return paddress;
}

page_t mmu_read_(size_t vaddress, bool drop, bool clean)
{
    uint32_t *dir;
    int page_no;
    if (vaddress >= kMMU.kspace->lower_bound &&
        vaddress < kMMU.kspace->upper_bound) {
        dir = ktable;
        page_no = (vaddress - kMMU.kspace->lower_bound) / PAGE_SIZE;
    } else if (vaddress >= user_space->lower_bound &&
               vaddress < user_space->upper_bound) {
        dir = (uint32_t *)user_space->directory;
        page_no = (vaddress - user_space->lower_bound) / PAGE_SIZE;
    }

    page_t pg = dir[page_no] & ~(PAGE_SIZE - 1);
    if (drop)
        dir[page_no] = 0;
    return pg;
}

page_t mmu_read(size_t vaddress)
{
    return mmu_read_(vaddress, false, false);
}

page_t mmu_drop(size_t vaddress, bool clean)
{
    return mmu_read_(vaddress, true, clean);
}


int mmu_uspace_size = 24;

void mmu_create_uspace(mspace_t *mspace)
{
    mspace->lower_bound = (size_t)valloc(mmu_uspace_size * PAGE_SIZE);
    mspace->upper_bound = mspace->lower_bound + mmu_uspace_size * PAGE_SIZE;
    mspace->directory = (page_t)kalloc(mmu_uspace_size * sizeof(page_t));
    user_space = mspace;
}

void mmu_destroy_uspace(mspace_t *mspace)
{
    free((void *)mspace->lower_bound);
    kfree((void *)mspace->directory);
}


// page_t mmu_directory()
// {
//     usertable = kalloc(512 * _Kib_);
//     page_t dir_page = (page_t)usertable;
//     return dir_page;
// }

// void mmu_release_dir(page_t dir)
// {
//     uint32_t *table = (uint32_t *)dir;
//     kfree(table);
// }
