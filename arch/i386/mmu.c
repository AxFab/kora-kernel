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
#include <kernel/cpu.h>
#include <kernel/memory.h>
#include <string.h>

void setup_allocator(void *, size_t);

#define MMU_KRN(vaddr)  (page_t*)(0xFFBFF000 | (((vaddr) >> 20) & ~3))
#define MMU_DIR(vaddr)  (page_t*)(0xFFFFF000 | (((vaddr) >> 20) & ~3))
#define MMU_TBL(vaddr)  (page_t*)(0xFFC00000 | (((vaddr) >> 10) & ~3))

#define MMU_KRN_DIR_PG  ((page_t*)0x2000)
#define MMU_KRN_TBL_PG  ((page_t*)0x3000)


static int mmu_flags(size_t vaddr, int flags)
{
    int pgf = 1;
    if (flags & VMA_WRITE)
        pgf |= 0x2;
    if (vaddr < MMU_KSPACE_LOWER)
        pgf |= 0x4;
    else
        pgf |= 0x8;
    if (flags & VMA_UNCACHABLE)
        pgf |= 0x10;
    return pgf;
}

/* Kernel prepare and initiate virtual memory */
void mmu_enable()
{
    unsigned i;
    // Setup first heap arena [3Mib - 4Mib]
    setup_allocator((void *)(3 * _Mib_), 1 * _Mib_);

    // Record physical available memory
    mboot_memory();

    // Prepare kernel PGD
    page_t *dir = MMU_KRN_DIR_PG;
    page_t *tbl = MMU_KRN_TBL_PG;
    memset(dir, 0, PAGE_SIZE);
    memset(tbl, 0, PAGE_SIZE);
    dir[1023] = dir[1022] = (page_t)dir | MMU_K_RW;
    dir[0] = (page_t)tbl | MMU_K_RW;
    for (i = 0; i < 1024; ++i)
        tbl[i] = (i * PAGE_SIZE) | MMU_K_RW;

    kMMU.kspace->lower_bound = MMU_KSPACE_LOWER;
    kMMU.kspace->upper_bound = MMU_KSPACE_UPPER;
    kMMU.kspace->directory = (page_t)MMU_KRN_DIR_PG;
    x86_enable_mmu();
}

/* Disallocate properly kernel ressources, mostly for checks */
void mmu_leave()
{
    unsigned i;
    page_t *dir = MMU_KRN_DIR_PG;
    for (i = MMU_KTBL_LOW ; i < MMU_KTBL_HIGH; ++i) {
        if (dir[i])
            page_release(dir[i] & (PAGE_SIZE - 1));
    }
}

/* Resolve a single missing virtual page */
int mmu_resolve(size_t vaddr, page_t phys, int flags)
{
    int pages = 0;
    page_t *dir = MMU_DIR(vaddr);
    page_t *tbl = MMU_TBL(vaddr);
    if (*dir == 0) {
        if (vaddr >= MMU_KSPACE_LOWER) {
            page_t *krn = MMU_KRN(vaddr);
            if (*krn == 0) {
                pages++;
                *krn = page_new() | MMU_K_RW;
                *dir = *krn;
                memset((void *)ALIGN_DW((size_t)tbl, PAGE_SIZE), 0, PAGE_SIZE);
            }
            *dir = *krn;
        } else {
            pages++;
            page_t pgd = page_new();
            *dir = pgd | MMU_U_RW;
            if (vaddr < 0x500000)
                kprintf(-1, "[MMU] Missing table %p using %p\n", vaddr, pgd);
            memset((void *)ALIGN_DW((size_t)tbl, PAGE_SIZE), 0, PAGE_SIZE);
        }
    }

    if (*tbl == 0) {
        if (phys == 0) {
            pages++;
            phys = page_new();
        }
        if (vaddr < 0x500000)
            kprintf(-1, "[MMU] Resolve at %p using %p\n", vaddr, phys);
        *tbl = phys | mmu_flags(vaddr, flags);
    } else
        assert(vaddr >= MMU_KSPACE_LOWER);
    return pages; //(*tbl) & ~(PAGE_SIZE - 1);
}

/* Get physical address for virtual provided one */
page_t mmu_read(size_t vaddr)
{
    page_t *dir = MMU_DIR(vaddr);
    page_t *tbl = MMU_TBL(vaddr);
    if ((*dir & 1) == 0 || ((*tbl & 1) == 0))
        return 0;
    return *tbl & ~(PAGE_SIZE - 1);
}


/* Release a virtual page, returns physical one in case release is required */
page_t mmu_drop(size_t vaddr)
{
    // kprintf(-1, " - %08x\n", vaddr);
    page_t *dir = MMU_DIR(vaddr);
    page_t *tbl = MMU_TBL(vaddr);
    if ((*dir & 1) == 0 || ((*tbl & 1) == 0))
        return 0;
    page_t pg = *tbl & ~(PAGE_SIZE - 1);
    if (vaddr < 0x500000)
        kprintf(-1, "[MMU] Drop page at %p using %p\n", vaddr, pg);
    *tbl = 0;
    asm volatile(
        "movl %0,%%eax\n"
        "invlpg (%%eax)\n"
        :: "r"(vaddr) : "%eax");
    return pg;
}

/* Change access settimgs for a virtual page */
page_t mmu_protect(size_t vaddr, int flags)
{
    page_t *dir = MMU_DIR(vaddr);
    page_t *tbl = MMU_TBL(vaddr);
    if ((*dir & 1) == 0 || ((*tbl & 1) == 0))
        return 0;
    page_t pg = *tbl & ~(PAGE_SIZE - 1);
    *tbl = pg | mmu_flags(vaddr, flags);
    asm volatile(
        "movl %0,%%eax\n"
        "invlpg (%%eax)\n"
        :: "r"(vaddr) : "%eax");
    return pg;
}

void mmu_create_uspace(mspace_t *mspace)
{
    unsigned i;
    page_t dir_pg = page_new();
    page_t *dir = (page_t *)kmap(PAGE_SIZE, NULL, dir_pg, VMA_PHYSIQ);
    memset(dir, 0,  PAGE_SIZE);
    dir[1023] = dir_pg | MMU_K_RW;
    dir[1022] = (page_t)MMU_KRN_DIR_PG | MMU_K_RW;
    dir[0] = (page_t)MMU_KRN_TBL_PG | MMU_K_RW;
    // TODO - COPY KERNEL HEAP TABLE PAGES
    for (i = MMU_KSPACE_LOWER >> 22; i < MMU_KSPACE_UPPER >> 22; ++i)
        dir[i] = ((page_t *)0xFFBFF000)[i];
    kunmap(dir, PAGE_SIZE);

    mspace->p_size++; // TODO g_size
    mspace->lower_bound = MMU_USPACE_LOWER;
    mspace->upper_bound = MMU_USPACE_UPPER;
    mspace->directory = dir_pg;
}

void mmu_destroy_uspace(mspace_t *mspace)
{
    unsigned i;
    page_t dir_pg = mspace->directory;
    page_t *dir = (page_t *)kmap(PAGE_SIZE, NULL, dir_pg, VMA_PHYSIQ);

    for (i = MMU_UTBL_LOW ; i < MMU_UTBL_HIGH; ++i) {
        if (dir[i]) {
            mspace->p_size--;
            page_release(dir[i] & (PAGE_SIZE - 1));
        }
    }
    kunmap(dir, PAGE_SIZE);
    // TODO mspace->p_size--;
    page_release(dir_pg);
}

void mmu_context(mspace_t *mspace)
{
    int i;
    page_t dir_pg = mspace->directory;
    page_t *dir = (page_t *)kmap(PAGE_SIZE, NULL, dir_pg, VMA_PHYSIQ);
    // unsigned table = ((unsigned)&mspace) >> 22;
    /* Check the current stack page is present  */
    page_t *krn = (page_t *)0xFFBFF000;
    for (i = 0; i < 4; ++i)
        dir[i] = krn[i];
    for (i = 768; i < 1020; ++i)
        dir[i] = krn[i];
    // dir[table] = ((page_t *)0xFFBFF000)[table];
    kunmap(dir, PAGE_SIZE);
    x86_set_cr3(dir_pg);
}

void mmu_explain(size_t vaddr)
{
    size_t cr3 = x86_get_cr3();
    page_t *dir = MMU_DIR(vaddr);
    page_t *tbl = MMU_TBL(vaddr);
    if (*dir)
        kprintf(KLOG_DBG, " @%p <G:%p> -> {%p:%p} / {%p:%p}\n", vaddr, cr3, mmu_read((size_t)dir),
                *dir, mmu_read(tbl), *tbl);
    else
        kprintf(KLOG_DBG, " @%p <G:%p> -> {%p:%p}\n", vaddr, cr3, mmu_read((size_t)dir), *dir);
}



void mmu_dump()
{
}
