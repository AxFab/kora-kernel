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
#include <kora/hmap.h>
// #include <kernel/arch.h>
// #include <kernel/vma.h>

enum {

    // VMA_SHARED = 0x10000,
    VMA_COW = 0x20000,
    VMA_FIXED = 0x40000,
    VMA_CLEAN = 0x80000,
    VMA_BACKEDUP = 0x100000,

    VMA_HEAP = 0x1000,
    VMA_STACK = 0x2000,
    VMA_FILE = 0x3000,
    VMA_PIPE = 0x4000,
    VMA_PHYS = 0x5000,
    VMA_ANON = 0x6000,
    VMA_SHDANON = 0x7000,
    VMA_FILECPY = 0x8000,
    VMA_DLTEXT = 0x9000,
    VMA_DLDATA = 0xA000,
    VMA_DLRODT = 0xB000,
    VMA_DLIB = 0xC000,
    VMA_TYPE = 0xF000,
};

#define VM_UNMAPED 0x40000
#define VM_FAST_ALLOC 0400

#define VPG_BACKEDUP 0
#define VPG_SHARED 1
#define VPG_PRIVATE 2


typedef struct inode inode_t;
typedef struct dlproc dlproc_t;
typedef struct dlib dlib_t;
typedef struct task task_t;

typedef struct vma vma_t;
typedef struct vmsp vmsp_t;
typedef struct vma_ops vma_ops_t;
typedef struct page_sharing page_sharing_t;


///* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
vma_t *vmsp_find_area(vmsp_t *vmsp, size_t address);
size_t vmsp_map(vmsp_t *vmsp, size_t address, size_t length, void *ptr, xoff_t offset, int flags);
int vmsp_unmap(vmsp_t *vmsp, size_t base, size_t length);
int vmsp_protect(vmsp_t *vmsp, size_t base, size_t length, int flags);

vmsp_t *vmsp_create();
vmsp_t *vmsp_open(vmsp_t *vmsp);
vmsp_t *vmsp_clone(vmsp_t *vmsp);
void vmsp_sweep(vmsp_t *vmsp);
void vmsp_close(vmsp_t *vmsp);

int vmsp_resolve(vmsp_t *vmsp, size_t address, bool missing, bool write);
void vmsp_display(vmsp_t *vmsp);

vmsp_t *memory_space_at(size_t address);

///* Create a memory space for a user application */
//vmsp_t *mspace_create();
///* Increment memory space RCU */
//vmsp_t *mspace_open(vmsp_t *mspace);
///* Decrement memory space RCU */
//void mspace_close(vmsp_t *mspace);
///* Copy all VMA and associated pages */
//vmsp_t *mspace_clone(vmsp_t *model);
//
//vmsp_t *mspace_from(size_t vaddr);
//
///* Map a memory area inside the provided address space. */
//void *mspace_map(vmsp_t *mspace, size_t address, size_t length, inode_t *ino, xoff_t offset, int flags);
///* Change the protection flags of a memory area. */
//int mspace_protect(vmsp_t *mspace, size_t address, size_t length, int flags);
///* Change the flags of a memory area. */
//int mspace_unmap(vmsp_t *mspace, size_t address, size_t length);
///* Search a VMA structure at a specific address */
//vma_t *mspace_search_vma(vmsp_t *mspace, size_t address);
///* Print the information of memory space -- used for /proc/{x}/mmap  */
//void mspace_display(vmsp_t *mspace);

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
void page_sweep(vmsp_t *mspace, size_t address, size_t length, bool clean);
/* Resolve a page fault */
// int page_fault(size_t address, int reason);
/* - */
void page_teardown();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* - */
void mmu_enable();
/* - */
void mmu_leave();
/* - */
void mmu_context(vmsp_t *mspace);
/* - */
size_t mmu_resolve(size_t vaddr, size_t phys, int falgs);
/* - */
size_t mmu_set(size_t directory, size_t vaddr, size_t phys, int flags);
/* - */
size_t mmu_read(size_t vaddr);
/* - */
int mmu_read_flags(size_t vaddr);
/* - */
size_t mmu_drop(size_t vaddr);
/* - */
bool mmu_dirty(size_t vaddr);
/* - */
size_t mmu_protect(size_t vaddr, int falgs);
/* - */
void mmu_create_uspace(vmsp_t *mspace);
/* - */
void mmu_destroy_uspace(vmsp_t *mspace);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void memory_initialize();
void memory_sweep();
void memory_info();

bool page_shared(page_sharing_t *share, size_t page, int count);

#define PF_ERR "\033[91mSIGSEGV\033[0m "
#define KB  (PAGE_SIZE / 1024)

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

    vmsp_t *kspace;  /* Kernel address space */
    vmsp_t *uspace;
};

/* - */
extern struct kMmu __mmu;


//struct vma_cow_reg
//{
//    atomic_int rcu;
//    vma_cow_reg_t *parent;
//    size_t pages_count;
//    size_t pages[0];
//};

#define VMS_NAME(m) ((m) == __mmu.kspace ? "Krn" : "Usr")

#define VMSP_MAX_SIZE 0x08000000 // 128Mb

struct vmsp
{

    atomic_int usage;  /* Usage counter */
    bbtree_t tree;  /* Binary tree of VMAs sorted by addresses */
    size_t directory;  /* Page global directory */
    size_t lower_bound;  /* Lower bound of the address space */
    size_t upper_bound;  /* Upper bound of the address space */
    size_t v_size;  /* Virtual allocated page counter */
    size_t p_size;  /* Private allocated page counter */
    size_t s_size;  /* Shared allocated page counter */
    size_t t_size;  /* Table allocated page counter */
    splock_t lock;  /* Memory space protection lock */
    dlproc_t *proc;
    size_t max_size;
    page_sharing_t *share;
};

struct vma
{
    atomic_int usage;
    bbnode_t node;  /* Binary tree node of VMAs contains the base address */
    vmsp_t *space;
    size_t length;  /* The length of this VMA */
    union
    {
        inode_t *ino; // File or pipe
        dlib_t *lib; // Library
        task_t *tsk; // Stack
    };
    xoff_t offset;  /* An offset to a file or an physical address, depending of type */
    int flags;  /* VMA flags */
    vma_ops_t *ops;
};

struct vma_ops 
{
    size_t(*fetch)(vmsp_t *vmsp, vma_t *vma, xoff_t offset, bool blocking);
    void (*release)(vmsp_t *vmsp, vma_t *vma, xoff_t offset, size_t page);
    void (*resolve)(vmsp_t *vmsp, vma_t *vma, size_t vaddr, size_t page);
    int (*shared)(vmsp_t *vmsp, vma_t *vma, size_t address, size_t page);
    void (*unmap)(vmsp_t *vmsp, vma_t *vma, size_t address);
    int (*protect)(vmsp_t *vmsp, vma_t *vma, int flags);
    void (*split)(vma_t *va1, vma_t *va2);
    void (*clone)(vmsp_t *vmsp1, vmsp_t *vmsp2, vma_t *va1, vma_t *va2);
    char *(*print)(vma_t *vma, char *buf, size_t len);
    void (*close)(vma_t *vma);

};


struct page_sharing
{
    splock_t lock;
    hmap_t map;
    atomic_int usage;
};


///* Helper to print VMA info into syslogs */
//char *vma_print(char *buf, int len, vma_t *vma);
///* - */
//vma_t *vma_create(vmsp_t *mspace, size_t address, size_t length, inode_t *ino, xoff_t offset, int flags);
///* - */
//vma_t *vma_clone(vmsp_t *mspace, vma_t *model);
///* Split one VMA into two. */
//vma_t *vma_split(vmsp_t *mspace, vma_t *area, size_t length);
///* Close a VMA and release private pages. */
//int vma_close(vmsp_t *mspace, vma_t *vma, int arg);
///* Change the flags of a VMA. */
//int vma_protect(vmsp_t *mspace, vma_t *vma, int flags);
//
//
//int vma_resolve(vma_t *vma, size_t address, size_t length);
//
//int vma_copy_on_write(vma_t *vma, size_t address, size_t length);
//


/* Close all VMAs */
//void mspace_sweep(vmsp_t *mspace);


int mspace_check(vmsp_t *mspace, const void *ptr, size_t len, int flags);
int mspace_check_str(vmsp_t *mspace, const char *str, size_t max);
int mspace_check_strarray(vmsp_t *mspace, const char **str);

#endif /* _KERNEL_MEMORY_H */
