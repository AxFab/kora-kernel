/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2018  <Fabien Bavent>
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
#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H 1

#include <kernel/types.h>
#include <kernel/vma.h>

typedef struct vma vma_t;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Create a memory space for a user application */
mspace_t *mspace_create();
/* Increment memory space RCU */
mspace_t *mspace_open(mspace_t *mspace);
/* Decrement memory space RCU */
void mspace_close(mspace_t *mspace);
/* Copy all VMA and associated pages */
mspace_t *mspace_clone(mspace_t *model);

/* Map a memory area inside the provided address space. */
void *mspace_map(mspace_t *mspace, size_t address, size_t length,
                 inode_t *ino, off_t offset, int flags);
/* Change the protection flags of a memory area. */
int mspace_protect(mspace_t *mspace, size_t address, size_t length, int flags);
/* Change the flags of a memory area. */
int mspace_unmap(mspace_t *mspace, size_t address, size_t length);
/* Search a VMA structure at a specific address */
vma_t *mspace_search_vma(mspace_t *mspace, size_t address);
/* Print the information of memory space -- used for /proc/{x}/mmap  */
void mspace_display();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* - */
void page_initialize();
/* - */
void page_range(long long base, long long length);
/* Allocat a single page for the system and return it's physical address */
page_t page_new();
/* Mark a physique page, returned by `mmu_new_page`, as available again */
void page_release(page_t paddress);
/* Free all used pages into a range of virtual addresses */
void page_sweep(mspace_t *mspace, size_t address, size_t length, bool clean);
/* Resolve a page fault */
int page_fault(mspace_t *mspace, size_t address, int reason);


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* - */
void mmu_enable();
/* - */
void mmu_leave();
/* - */
void mmu_context(mspace_t *mspace);
/* - */
int mmu_resolve(size_t vaddr, page_t phys, int falgs);
/* - */
page_t mmu_read(size_t vaddr);
/* - */
page_t mmu_drop(size_t vaddr);
/* - */
page_t mmu_protect(size_t vaddr, int falgs);
/* - */
void mmu_create_uspace(mspace_t *mspace);
/* - */
void mmu_destroy_uspace(mspace_t *mspace);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void memory_initialize();
void memory_sweep();
void memory_info();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


struct kMmu {
    size_t upper_physical_page;  /* Maximum amount of memory */
    size_t pages_amount;  /* Maximum pages amount */
    size_t free_pages;  /* Number of unused free pages */
    size_t page_size;  /* Page size */
    // size_t uspace_lower_bound;  /* Userspace lower bound */
    // size_t uspace_upper_bound;  /* Userspace upper bound */
    // size_t kheap_lower_bound;  /* Kernel heap lower bound */
    // size_t kheap_upper_bound;  /* Kernel heap upper bound */
    size_t max_vma_size;  /* Maximum size of a VMA */

    long soft_page_fault;

    mspace_t *kspace;  /* Kernel address space */
};

/* - */
extern struct kMmu kMMU;


#endif /* _KERNEL_MEMORY_H */
