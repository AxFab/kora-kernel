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
#include <kernel/core.h>
#include <kernel/memory.h>

#define MMU_KRN(vaddr)  (size_t*)(0xFFBFF000 | (((vaddr) >> 20) & ~3))
#define MMU_DIR(vaddr)  (size_t*)(0xFFFFF000 | (((vaddr) >> 20) & ~3))
#define MMU_TBL(vaddr)  (size_t*)(0xFFC00000 | (((vaddr) >> 10) & ~3))

#define MMU_KRN_DIR_PG  ((size_t*)0x2000)
#define MMU_KRN_TBL_PG  ((size_t*)0x3000)

#define MMU_BOUND_ULOWER  0x00400000
#define MMU_BOUND_UUPPER  0xC0000000
#define MMU_BOUND_KLOWER  0xC0000000
#define MMU_BOUND_KUPPER  0xFF000000

void mboot_memory();


/* Page entry flags:
 *  bit0: Is present
 *  bit1: R/W - Allow write operations
 *  bit2: U/S - Accesible into user mode
 *  bit3: PWT - Page write throught
 *  bit4: PCD - Uncachable page
 *  bit5: A - Have been accessed
 *  bit6: D - Have been written (PT only)
 *  bit7: PAT (PT only) or Big page (PD only)
 *  bit8: G - Global page (if CR4.PGE = 1)
 */

#define PG_PRESENT 0x001
#define PG_WRITABLE 0x002
#define PG_USERMODE 0x004
#define PG_PWT 0x008
#define PG_PCD 0x010
#define PG_ACCESSED 0x020
#define PG_DIRTY 0x040
#define PG_BIGPAGE 0x080
#define PG_GLOBAL 0x100


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void mmu_setup(sys_info_t *sysinfo)
{
    // Setup first heap arena [3Mib - 4Mib]
    setup_allocator((void *)(3 * _Mib_), 1 * _Mib_);
    memory_initialize(); // TODO -- Refactor memory code
}

void mmu_enable()
{
    // Record physical available memory
    mboot_memory();
    kMMU.kspace->lower_bound = MMU_BOUND_KLOWER;
    kMMU.kspace->upper_bound = MMU_BOUND_KUPPER;
    kMMU.kspace->directory = (size_t)0x2000;
}

void mmu_leave()
{
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void mmu_context(mspace_t *mspace)
{
    int i;
    page_t dir_pg = mspace->directory;
    page_t *dir = (page_t *)kmap(PAGE_SIZE, NULL, dir_pg, VMA_PHYS | VM_RW);
    // unsigned table = ((unsigned)&mspace) >> 22;
    /* Check the current stack page is present  */
    page_t *krn = (page_t *)0xFFBFF000;
    for (i = 0; i < 1; ++i)
        dir[i] = krn[i];
    for (i = 768; i < 1020; ++i)
        dir[i] = krn[i];
    // dir[table] = ((page_t *)0xFFBFF000)[table];
    kunmap(dir, PAGE_SIZE);
    x86_set_cr3(dir_pg);
}

void mmu_create_uspace(mspace_t *mspace)
{
    unsigned i;
    page_t dir_pg = page_new();
    page_t *dir = (page_t *)kmap(PAGE_SIZE, NULL, dir_pg, VMA_PHYS | VM_RW);
    memset(dir, 0,  PAGE_SIZE);
    dir[1023] = dir_pg | (PG_PRESENT | PG_WRITABLE | PG_GLOBAL);;
    dir[1022] = (page_t)MMU_KRN_DIR_PG | (PG_PRESENT | PG_WRITABLE | PG_GLOBAL);;
    dir[0] = (page_t)MMU_KRN_TBL_PG | (PG_PRESENT | PG_WRITABLE | PG_GLOBAL);;
    // TODO - COPY KERNEL HEAP TABLE PAGES
    for (i = MMU_BOUND_KLOWER >> 22; i < MMU_BOUND_KUPPER >> 22; ++i)
        dir[i] = ((page_t *)0xFFBFF000)[i];
    kunmap(dir, PAGE_SIZE);

    mspace->p_size++; // TODO g_size
    mspace->lower_bound = MMU_BOUND_ULOWER;
    mspace->upper_bound = MMU_BOUND_UUPPER;
    mspace->directory = dir_pg;
}

void mmu_destroy_uspace(mspace_t *mspace)
{
    unsigned i;
    page_t dir_pg = mspace->directory;
    page_t *dir = (page_t *)kmap(PAGE_SIZE, NULL, dir_pg, VMA_PHYS | VM_RW);

    for (i = MMU_BOUND_ULOWER >> 22; i < MMU_BOUND_UUPPER >> 22; ++i) {
        if (dir[i]) {
            mspace->p_size--;
            page_release(dir[i] & (PAGE_SIZE - 1));
        }
    }
    kunmap(dir, PAGE_SIZE);
    // TODO mspace->p_size--;
    page_release(dir_pg);
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static int mmu_flags(size_t vaddr, int flags)
{
    int pgf = PG_PRESENT;
    if (flags & VM_WR)
        pgf |= PG_WRITABLE;
    if (vaddr < MMU_BOUND_KLOWER)
        pgf |= PG_USERMODE;
    else
        pgf |= PG_GLOBAL;
    if (flags & VM_UNCACHABLE)
        pgf |= PG_PCD;
    return pgf;
}


size_t mmu_protect(size_t vaddr, int flags)
{
    size_t *dir = MMU_DIR(vaddr);
    size_t *tbl = MMU_TBL(vaddr);
    if ((*dir & 1) == 0 || ((*tbl & 1) == 0))
        return 0;
    size_t pg = *tbl & ~(PAGE_SIZE - 1);
    *tbl = pg | mmu_flags(vaddr, flags);
    asm volatile(
        "movl %0,%%eax\n"
        "invlpg (%%eax)\n"
        :: "r"(vaddr) : "%eax");
    return pg;
}


/* Get physical address for virtual provided one */
size_t mmu_read(size_t vaddr)
{
    size_t *dir = MMU_DIR(vaddr);
    size_t *tbl = MMU_TBL(vaddr);
    if ((*dir & 1) == 0 || ((*tbl & 1) == 0))
        return 0;
    return *tbl & ~(PAGE_SIZE - 1);
}

/* Release a virtual page, returns physical one in case release is required */
size_t mmu_drop(size_t vaddr)
{
    // size_t cr3 = x86_get_cr3();
    // kprintf(-1, " - %08x\n", vaddr);
    size_t *dir = MMU_DIR(vaddr);
    size_t *tbl = MMU_TBL(vaddr);
    if ((*dir & 1) == 0 || ((*tbl & 1) == 0))
        return 0;
    size_t pg = *tbl & ~(PAGE_SIZE - 1);
    // if (vaddr < 0x500000)
    //     kprintf(-1, "[MMU] Drop page at %p using %p {%p.%p}\n", vaddr, pg, cr3, tbl);
    *tbl = 0;
    asm volatile(
        "movl %0,%%eax\n"
        "invlpg (%%eax)\n"
        :: "r"(vaddr) : "%eax");
    return pg;
}


/* Resolve a single missing virtual page */
int mmu_resolve(size_t vaddr, size_t phys, int flags)
{
    int pages = 0;
    // size_t cr3 = x86_get_cr3();
    size_t *dir = MMU_DIR(vaddr);
    size_t *tbl = MMU_TBL(vaddr);
    if (*dir == 0) {
        if (vaddr >= MMU_BOUND_KLOWER) {
            size_t *krn = MMU_KRN(vaddr);
            if (*krn == 0) {
                pages++;
                *krn = page_new() | (PG_PRESENT | PG_WRITABLE | PG_GLOBAL);
                *dir = *krn;
                memset((void *)ALIGN_DW((size_t)tbl, PAGE_SIZE), 0, PAGE_SIZE);
            }
            *dir = *krn;
        } else {
            pages++;
            size_t pgd = page_new();
            *dir = pgd | (PG_PRESENT | PG_WRITABLE | PG_USERMODE);
            // if (vaddr < 0x500000)
            //     kprintf(-1, "[MMU] Missing table %p using %p {%p.%p}\n", vaddr, pgd, cr3, dir);
            memset((void *)ALIGN_DW((size_t)tbl, PAGE_SIZE), 0, PAGE_SIZE);
        }
    }

    if (*tbl == 0) {
        if (phys == 0) {
            pages++;
            phys = page_new();
        }
        // if (vaddr < 0x500000)
        //     kprintf(-1, "[MMU] Resolve at %p using %p {%p.%p}\n", vaddr, phys, cr3, tbl);
        *tbl = phys | mmu_flags(vaddr, flags);
    } else
        assert(vaddr >= MMU_BOUND_KLOWER);
    return pages; //(*tbl) & ~(PAGE_SIZE - 1);
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void x86_paging()
{
    // size_t *pd0 = (void *)0x2000;
    // pd0[0] = (size_t)0 | (PG_PRESENT | PG_WRITABLE | PG_BIGPAGE | PG_GLOBAL);
    // pd0[1023] = (size_t)pd0 | (PG_PRESENT | PG_WRITABLE | PG_GLOBAL);

    int i;
    size_t *pd0 = MMU_KRN_DIR_PG;
    size_t *pt0 = MMU_KRN_TBL_PG;
    for (i = 0; i < 1024; ++i) {
        pt0[i] = (i << 12) | (PG_PRESENT | PG_WRITABLE | PG_GLOBAL);
        pd0[i] = 0;
    }
    pd0[0] = (size_t)pt0 | (PG_PRESENT | PG_WRITABLE | PG_GLOBAL);
    pd0[1022] = (size_t)pd0 | (PG_PRESENT | PG_WRITABLE | PG_GLOBAL);
    pd0[1023] = (size_t)pd0 | (PG_PRESENT | PG_WRITABLE | PG_GLOBAL);
}


