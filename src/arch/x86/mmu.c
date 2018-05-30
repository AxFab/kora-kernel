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
#include <kernel/memory.h>
#include <kernel/core.h>
#include <kernel/mmu.h>
#include <kora/mcrs.h>
#include <kora/allocator.h>
#include <kora/bbtree.h>
#include <kora/splock.h>
#include <assert.h>
#include <errno.h>
#include <string.h>


extern uint32_t *grub_table;
void x86_enable_MMU();

/* - */
void mmu_enable()
{
    // List available memory
    uint32_t *ram_table = (uint32_t *)grub_table[12];
    for (; *ram_table == 0x14; ram_table += 6) {
        int64_t base = (int64_t)ram_table[1] | ((int64_t)ram_table[2] << 32);
        int64_t length = (int64_t)ram_table[3] | ((int64_t)ram_table[4] << 32);

        if (base < 2 * _Mib_) { // First 2 Mb are reserved for kernel code
            length -= 2 * _Mib_ - base;
            base = 2 * _Mib_;
        }

        if (length > 0 && ram_table[5] == 1) {
            page_range(base, length);
        }
    }

    // Use 1.5 Mb to 2 Mb as initial heap arena
    setup_allocator((void *)(1536 * _Kib_), 512 * _Kib_);


    int i;
    // Prepare Kernel pages
    page_t *dir = (page_t *)KRN_PG_DIR;
    page_t *tbl = (page_t *)KRN_PG_TBL;
    memset(dir, 0, PAGE_SIZE);
    memset(tbl, 0, PAGE_SIZE);
    dir[1022] = KRN_PG_DIR | MMU_K_RW;
    dir[1023] = KRN_PG_DIR | MMU_K_RW;
    dir[0] = KRN_PG_TBL | MMU_K_RW;
    for (i = 0; i < 512; ++i) {
        tbl[i] = (i * PAGE_SIZE) | MMU_K_RW;
    }

    kMMU.kspace->lower_bound = MMU_KSPACE_LOWER;
    kMMU.kspace->upper_bound = MMU_KSPACE_UPPER;

    // Active change
    x86_enable_MMU();
}

void mmu_leave() {}

/* - */
int mmu_resolve(mspace_t *mspace, size_t vaddress, page_t paddress, int access, bool clean)
{
    assert((vaddress & (PAGE_SIZE - 1)) == 0);

    int dir = (vaddress >> 22) & 0x3ff;
    // int tbl = (vaddress >> 12) & 0x3ff;
    int flags = 0;

    page_t *table = NULL;
    if (vaddress >= MMU_KSPACE_LOWER && vaddress < MMU_KSPACE_UPPER) {
        table = MMU_K_DIR;
        flags = access & VMA_WRITE ? MMU_K_RW : MMU_K_RO;
        if (access & VMA_UNCACHABLE)
            flags |= 0x10;
    } else if (vaddress >= MMU_USPACE_LOWER &&
               vaddress < MMU_USPACE_UPPER) {
        table = MMU_U_DIR;
        flags = access & VMA_WRITE ? MMU_U_RW : MMU_U_RO;
    } else {
        kpanic("Wrong memory mapping request");
    }

    if (paddress == 0) {
        paddress = page_new();
    }

    if (table[dir] == 0) {
        table[dir] = page_new() | (flags & ~0x10) | MMU_WRITE;
        if (table == MMU_K_DIR) {
            MMU_U_DIR[dir] = MMU_K_DIR[dir];
        }
    }

    if (MMU_PAGES(vaddress) == 0) {
        MMU_PAGES(vaddress) = paddress | flags;
    }

    if (clean) {
        memset((void *)vaddress, 0, PAGE_SIZE);
    }

    return 0;
}

/* - */
static page_t mmu_read_(size_t vaddress, bool drop, bool clean)
{
    vaddress = ALIGN_DW(vaddress, PAGE_SIZE);
    page_t *dir = (page_t *)(0xFFFFF000 | ((vaddress >> 20) & ~3));
    page_t *tbl = (page_t *)(0xFFC00000 | ((vaddress >> 10) & ~3));
    if (*dir == 0 || *tbl == 0) {
        return 0;
    }
    if (clean) {
        memset((void *)vaddress, 0, PAGE_SIZE);
    }
    if (drop) {
        asm volatile (
        "movl %0,%%eax\n"
        "invlpg (%%eax)\n"
        :: "r"(vaddress) : "%eax");
        *tbl = 0;
    }
    return *tbl & (~(PAGE_SIZE - 1));
}

page_t mmu_read(mspace_t *mspace, size_t vaddress)
{
    return mmu_read_(vaddress, false, false);
}

page_t mmu_drop(mspace_t *mspace, size_t vaddress, bool clean)
{
    return mmu_read_(vaddress, true, clean);
}

void mmu_protect(mspace_t *mspace, size_t address, size_t length, int access)
{
}

/* - */
static page_t mmu_directory()
{
    int i;
    page_t dir_page = page_new();
    page_t *dir = kmap(PAGE_SIZE, NULL, dir_page, VMA_PHYS);
    for (i = 0; i < KRN_SP_LOWER / 4; ++i) {
        dir[i] = 0;
    }
    for (i = KRN_SP_LOWER / 4; i <= KRN_SP_UPPER / 4; ++i) {
        dir[i] = ((page_t *)KRN_PG_DIR)[i]; // | MMU_K_RW;
    }
    dir[1023] = dir_page | MMU_K_RW;
    dir[1022] = KRN_PG_DIR | MMU_K_RW;
    dir[0] = ((page_t *)KRN_PG_DIR)[0] | MMU_K_RW;
    kunmap(dir, PAGE_SIZE);
    return dir_page;
}


void mmu_create_uspace(mspace_t *mspace)
{
    mspace->lower_bound = MMU_USPACE_LOWER;
    mspace->upper_bound = MMU_USPACE_UPPER;
    mspace->directory = mmu_directory();
}

void mmu_destroy_uspace(mspace_t *mspace)
{
    const size_t T = 4 * _Mib_;
    unsigned i;
    /* Free all pages used as table */
    page_t *dir = kmap(PAGE_SIZE, NULL, mspace->directory, VMA_PHYS);
    for (i = mspace->lower_bound / T; i < mspace->upper_bound / T; ++i) {
        page_t tbl = dir[i] & ~(PAGE_SIZE - 1);
        if (tbl != 0)
            page_release(tbl);
    }
    kunmap(dir, PAGE_SIZE);
    /* Free the directory page */
    page_release(mspace->directory);
}

void mmu_context(mspace_t *mspace)
{

}

void mmu_dump_x86()
{
    int i, j;
    uint32_t *dir = (uint32_t *)0xFFFFF000;
    kprintf(KLOG_DBG, "-- Pages dump\n");
    for (i = 1; i < 1024; ++i) {
        if (dir[i] != 0) {
            kprintf(KLOG_DBG, "  %3x - %08x\n", i << 2, dir[i]);
            uint32_t *tbl = (uint32_t *)(0xFFC00000 | (i << 12));
            for (j = 0; j < 1024; ++j) {
                if (tbl[j] != 0) {
                    kprintf(KLOG_DBG, "   |- %3x - %08x\n", (i << 12) | (j << 2), tbl[j]);
                }
            }
        }
    }
}

void mmu_explain_x86(size_t a)
{
    size_t d = 0xFFFFF000 | ((a >> 20) & ~3);
    size_t t = 0xFFC00000 | ((a >> 10) & ~3);
    kprintf(KLOG_DBG, "Address <%08x> - [DIR=%08x:%08x]", a, d, *(int *)d);
    if (*(int *)d != 0) {
        kprintf(KLOG_DBG, " - [TBL=%08x:%08x]\n", t, *(int *)t);
    } else {
        kprintf(KLOG_DBG, "\n");
    }
}