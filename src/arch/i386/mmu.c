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
#include <string.h>

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

void mmu_enable()
{
    int i;
    // Setup first heap arena [1Mib - 2Mib]
    setup_allocator((void *)(1024 * _Kib_), 1024 * _Kib_);

    // Record physical available memory


    // Prepare kernel PGD
    page_t *dir = MMU_KRN_DIR_PG;
    page_t *tbl = MMU_KRN_TBL_PG;
    memset(dir, 0, PAGE_SIZE);
    memset(tbl, 0, PAGE_SIZE);
    tbl[1023] = tbl[1022] = (page_t)dir | MMU_K_RW;
    for (i = 0; i < 512; ++i)
        tbl[i] = (i * PAGE_SIZE) | MMU_K_RW;

    kMMU.kspace->lower_bound = MMU_KSPACE_LOWER;
    kMMU.kspace->upper_bound = MMU_KSPACE_UPPER;
    x86_enable_mmu();
}

void mmu_leave()
{

}

int mmu_resolve(size_t vaddr, page_t phys, int flags)
{
    page_t *dir = MMU_DIR(vaddr);
    page_t *tbl = MMU_TBL(vaddr);
    if (*dir == 0) {
        if (vaddr >= MMU_KSPACE_LOWER) {
            page_t *krn = MMU_KRN(vaddr);

        }
    }
}

page_t mmu_read(size_t vaddr)
{
    page_t *dir = MMU_DIR(vaddr);
    page_t *tbl = MMU_TBL(vaddr);
    if ((*dir & 1) == 0 || ((*tbl & 1) == 0))
        return 0;
    return *tbl & ~(PAGE_SIZE - 1);
}

page_t mmu_drop(size_t vaddr)
{
    page_t *dir = MMU_DIR(vaddr);
    page_t *tbl = MMU_TBL(vaddr);
    if ((*dir & 1) == 0 || ((*tbl & 1) == 0))
        return 0;
    page_t pg = *tbl & ~(PAGE_SIZE - 1);
    *tbl = 0;
    return pg;
}

int mmu_protect(size_t vaddr, int flags)
{
    page_t *dir = MMU_DIR(vaddr);
    page_t *tbl = MMU_TBL(vaddr);
    if ((*dir & 1) == 0 || ((*tbl & 1) == 0))
        return -1;
    page_t pg = *tbl & ~(PAGE_SIZE - 1);
    *tbl = pg | mmu_flags(vaddr, flags);
    return 0;
}

void mmu_open_uspace(mspace_t *mspace);
void mmu_close_uspace(mspace_t *mspace);
void mmu_context(mspace_t *mspace);

void mmu_explain(size_t vaddr)
{

}

void mmu_dump()
{

}
