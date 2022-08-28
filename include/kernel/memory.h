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
#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H 1

#include <kernel/stdc.h>
#include <kora/bbtree.h>
#include <kora/splock.h>
// #include <kernel/arch.h>
// #include <kernel/vma.h>

enum {

    // VMA_SHARED = 0x10000,
    VMA_COW = 0x20000,
    VMA_FIXED = 0x40000,
    VMA_CLEAN = 0x80000,

    VMA_HEAP = 0x1000,
    VMA_STACK = 0x2000,
    VMA_FILE = 0x3000,
    VMA_PIPE = 0x4000,
    VMA_PHYS = 0x5000,
    VMA_ANON = 0x6000,
    VMA_CODE = 0x7000,
    VMA_TYPE = 0x7000,
};

typedef struct inode inode_t;
typedef struct dlproc dlproc_t;

typedef struct vma vma_t;
typedef struct mspace mspace_t;
typedef struct vma_ops vma_ops_t;
typedef struct vma_cow_reg vma_cow_reg_t;


struct vma_ops
{
    int (*open)(vma_t *vma, int access, inode_t *ino, xoff_t offset);
    int (*resolve)(vma_t *vma, size_t address);
    int (*cow)(vma_t *vma, size_t address);
    void (*close)(vma_t *vma);
    char *(*print)(vma_t *vma, char *buf, int len);
    int (*protect)(vma_t *vma, int flags);
    void (*split)(vma_t *vma, vma_t *area, size_t length);
    int (*clone)(vma_t *vma, vma_t *model);
};

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
void *mspace_map(mspace_t *mspace, size_t address, size_t length, inode_t *ino, xoff_t offset, int flags);
/* Change the protection flags of a memory area. */
int mspace_protect(mspace_t *mspace, size_t address, size_t length, int flags);
/* Change the flags of a memory area. */
int mspace_unmap(mspace_t *mspace, size_t address, size_t length);
/* Search a VMA structure at a specific address */
vma_t *mspace_search_vma(mspace_t *mspace, size_t address);
/* Print the information of memory space -- used for /proc/{x}/mmap  */
void mspace_display(mspace_t *mspace);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* - */
void page_range(long long base, long long length);
/* Allocate a single page for the system and return it's physical address */
size_t page_new();
/* Look for count pages in continuous memory */
size_t page_get(int zone, int count);
/* Mark a physique page, returned by `mmu_new_page`, as available again */
void page_release(size_t paddress);
/* Free all used pages into a range of virtual addresses */
void page_sweep(mspace_t *mspace, size_t address, size_t length, bool clean);
/* Resolve a page fault */
int page_fault(size_t address, int reason);
/* - */
void page_teardown();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* - */
void mmu_enable();
/* - */
void mmu_leave();
/* - */
void mmu_context(mspace_t *mspace);
/* - */
size_t mmu_resolve(mspace_t *mspace, size_t vaddr, size_t phys, int falgs);
/* - */
size_t mmu_read(mspace_t *mspace, size_t vaddr);
/* - */
int mmu_read_flags(mspace_t *mspace, size_t vaddr);
/* - */
size_t mmu_drop(mspace_t *mspace, size_t vaddr);
/* - */
bool mmu_dirty(mspace_t *mspace, size_t vaddr);
/* - */
size_t mmu_protect(mspace_t *mspace, size_t vaddr, int falgs);
/* - */
void mmu_create_uspace(mspace_t *mspace);
/* - */
void mmu_destroy_uspace(mspace_t *mspace);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void memory_initialize();
void memory_sweep();
void memory_info();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define PGFLT_MISSING  1
#define PGFLT_WRITE  2
#define PGFLT_ERROR  4


struct kMmu {
    size_t upper_physical_page;  /* Maximum amount of memory */
    size_t pages_amount;  /* Maximum pages amount */
    atomic_int free_pages;  /* Number of unused free pages */
    size_t page_size;  /* Page size */
    // size_t uspace_lower_bound;  /* Userspace lower bound */
    // size_t uspace_upper_bound;  /* Userspace upper bound */
    // size_t kheap_lower_bound;  /* Kernel heap lower bound */
    // size_t kheap_upper_bound;  /* Kernel heap upper bound */
    size_t max_vma_size;  /* Maximum size of a VMA */

    long soft_page_fault;

    mspace_t *kspace;  /* Kernel address space */
    mspace_t *uspace;
};

/* - */
extern struct kMmu kMMU;


struct mspace {
    atomic_int users;  /* Usage counter */
    bbtree_t tree;  /* Binary tree of VMAs sorted by addresses */
    size_t directory;  /* Page global directory */
    size_t lower_bound;  /* Lower bound of the address space */
    size_t upper_bound;  /* Upper bound of the address space */
    size_t v_size;  /* Virtual allocated page counter */
    size_t p_size;  /* Physical allocated page counter */
    size_t a_size;  /* Total allocated page counter ? */
    size_t s_size;  /* Shared allocated page counter */
    size_t t_size;  /* Table allocated page counter */
    size_t phys_pg_count;  /* Physical page used on this address space */
    splock_t lock;  /* Memory space protection lock */
    dlproc_t *proc;
};


struct vma_cow_reg
{
    atomic_int rcu;
    vma_cow_reg_t *parent;
    size_t pages_count;
    size_t pages[0];
};

struct vma {
    bbnode_t node;  /* Binary tree node of VMAs contains the base address */
    size_t length;  /* The length of this VMA */
    inode_t *ino;  /* If the VMA is linked to a file, a pointer to the inode */
    xoff_t offset;  /* An offset to a file or an physical address, depending of type */
    int flags;  /* VMA flags */
    vma_ops_t *ops;
    vma_cow_reg_t *cow_reg;
    mspace_t *mspace;
};


/* Helper to print VMA info into syslogs */
char *vma_print(char *buf, int len, vma_t *vma);
/* - */
vma_t *vma_create(mspace_t *mspace, size_t address, size_t length, inode_t *ino, xoff_t offset, int flags);
/* - */
vma_t *vma_clone(mspace_t *mspace, vma_t *model);
/* Split one VMA into two. */
vma_t *vma_split(mspace_t *mspace, vma_t *area, size_t length);
/* Close a VMA and release private pages. */
int vma_close(mspace_t *mspace, vma_t *vma, int arg);
/* Change the flags of a VMA. */
int vma_protect(mspace_t *mspace, vma_t *vma, int flags);


int vma_resolve(vma_t *vma, size_t address, size_t length);

int vma_copy_on_write(vma_t *vma, size_t address, size_t length);



/* Close all VMAs */
void mspace_sweep(mspace_t *mspace);


int mspace_check(mspace_t *mspace, const void *ptr, size_t len, int flags);
int mspace_check_str(mspace_t *mspace, const char *str, size_t max);



#endif /* _KERNEL_MEMORY_H */
