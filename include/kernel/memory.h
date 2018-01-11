/*
 *      This file is part of the SmokeOS project.
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
#include <kernel/asm/mmu.h>
#include <skc/mcrs.h>

typedef struct mspace mspace_t;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Map a memory area inside the provided address space. */
void *memory_map(mspace_t *mspace, size_t address, size_t length, inode_t* ino, off_t offset, int flags);
/* Change the flags of a memory area. */
int memory_flag(mspace_t *mspace, size_t address, size_t length, int flags);
/* Remove disabled memory area */
int memory_scavenge(mspace_t *mspace);
/* Display the state of the current address space */
void memory_display();
/* Create a memory space for a user application */
mspace_t *memory_userspace();
/* - */
void memory_sweep(mspace_t *mspace);

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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* - */
void mmu_detect_ram();
/* - */
int mmu_resolve(size_t vaddress, page_t paddress, int access, bool clean);
/* - */
page_t mmu_drop(size_t vaddress, bool clean);
/* - */
page_t mmu_directory();
/* - */
void mmu_release_dir(page_t dir);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct kMmu {
  /* Maximum amount of memory */
  size_t upper_physical_page;
  /* Maximum pages amount */
  size_t pages_amount;
  /* Number of unsued free pages */
  size_t free_pages;
  /* Page size */
  size_t page_size;
  /* Userspace lower bound */
  size_t uspace_lower_bound;
  /* Userspace upper bound */
  size_t uspace_upper_bound;
  /* Kernel heap lower bound */
  size_t kheap_lower_bound;
  /* Kernel heap upper bound */
  size_t kheap_upper_bound;
  /* Maximum size of a VMA */
  size_t max_vma_length;
  /* Kernel address space */
  mspace_t *kspace;
};

/* - */
extern struct kMmu kMMU;


#endif /* _KERNEL_MEMORY_H */
