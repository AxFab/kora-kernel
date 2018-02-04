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
#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H 1

#include <kernel/types.h>

typedef struct mspace mspace_t;
typedef struct vma vma_t;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Map a memory area inside the provided address space. */
void *mspace_map(mspace_t *mspace, size_t address, size_t length,
                 inode_t *ino, off_t offset, off_t limit, int flags);
/* Change the protection flags of a memory area. */
int mspace_protect(mspace_t *mspace, size_t address, size_t length, int flags);
/* Change the flags of a memory area. */
int mspace_unmap(mspace_t *mspace, size_t address, size_t length);
/* Release all VMA marked as DEAD. */
int mspace_scavenge(mspace_t *mspace);
/* Print the information of memory space -- used for /proc/{x}/mmap  */
void mspace_display();
/* Create a memory space for a user application */
mspace_t *mspace_create();
/* Release all VMA and free all mspace data structure. */
void mspace_sweep(mspace_t *mspace);
/* Search a VMA structure at a specific address */
vma_t *mspace_search_vma(mspace_t *mspace, size_t address);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* - */
void page_initialize();
/* - */
void page_range(long long base, long long length);
/* Allocat a single page for the system and return it's physical address */
page_t page_new ();
/* Mark a physique page, returned by `mmu_new_page`, as available again */
void page_release(page_t paddress);
/* Free all used pages into a range of virtual addresses */
void page_sweep(size_t address, size_t length, bool clean);
/* Resolve a page fault */
int page_fault(mspace_t *mspace, size_t address, int reason);

int page_resolve(mspace_t *mspace, size_t address, size_t length);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* - */
void mmu_detect_ram();
/* - */
int mmu_resolve(size_t vaddress, page_t paddress, int access, bool clean);
/* - */
page_t mmu_read(size_t vaddress, bool drop, bool clean);
/* - */
page_t mmu_directory();
/* - */
void mmu_release_dir(page_t dir);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct kMmu {
    size_t upper_physical_page;  /* Maximum amount of memory */
    size_t pages_amount;  /* Maximum pages amount */
    size_t free_pages;  /* Number of unsued free pages */
    size_t page_size;  /* Page size */
    size_t uspace_lower_bound;  /* Userspace lower bound */
    size_t uspace_upper_bound;  /* Userspace upper bound */
    size_t kheap_lower_bound;  /* Kernel heap lower bound */
    size_t kheap_upper_bound;  /* Kernel heap upper bound */
    size_t max_vma_length;  /* Maximum size of a VMA */
    mspace_t *kspace;  /* Kernel address space */
};

/* - */
extern struct kMmu kMMU;


#endif /* _KERNEL_MEMORY_H */
