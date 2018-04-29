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
 *
 *      Memory managment unit configuration.
 */
#ifndef _KERNEL_ASM_VMA_H
#define _KERNEL_ASM_VMA_H 1

#include <kernel/types.h>
#include <kora/bbtree.h>
#include <kora/splock.h>
#include <stdatomic.h>

#define VMA_EXEC 0x001  /* Give execute right to this VMA */
#define VMA_WRITE 0x002  /* Give write access to this VMA */
#define VMA_READ 0x004  /* Allow reading data from this VMA */
#define VMA_SHARED 0x008  /* Flag that allow the page to be shared */
#define VMA_CAN_EXEC  0x010  /* Allow the flag VMA_EXEC to be set */
#define VMA_CAN_WRITE  0x020  /* Allow the flag VMA_WRITE to be set */
#define VMA_CAN_READ  0x040  /* Allow the flag VMA_READ to be set */
#define VMA_COPY_ON_WRITE  0x080  /* A copy of the page should be made on write operation */

#define VMA_RIGHTS 0x007  /* Mask for VMA accesses */

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define VMA_HEAP 0x100  /* Flag the VMA as part of the HEAP structure */
#define VMA_STACK 0x200  /* Flag the VMA as part of a STACK structure */
#define VMA_FILE 0x300  /* Flag the VMA as backuped by a file */
#define VMA_PHYS 0x400  /* Flags the VMA as mapping a physical address */
#define VMA_ANON 0x500  /* Flags the VMA as anonymous */

#define VMA_TYPE 0xF00  /* Mask used to know the VMA type */

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define VMA_DEAD 0x8000  /* Set the VMA as invalid. The VMA has to be cleared */
#define VMA_LOCKED  0x1000  /* Lock the VMA */

// #define VMA_CAN_GROW_UP 0x4000
// #define VMA_CAN_GROW_DOWN 0x8000
// #define VMA_NOBLOCK 0x10000
// #define VMA_HUGETABLE 0x20000

#define VMA_MAP_FIXED  0x10000  /* Flags used only for mspace_map(), tell to not use the address as an hint. */
#define VMA_RESOLVE  0x20000  /* Flags used for kmap(), tell to resolve page if possible */

#define VMA_CFG_MAXSIZE (128 * _Mib_)  /* Maximum allowed VMA length */

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* Preset VMA right flags */
#define VMA_RO  (VMA_READ | VMA_CAN_READ)
#define VMA_RD  (VMA_READ | VMA_CAN_READ | VMA_CAN_WRITE)
#define VMA_WO  (VMA_WRITE | VMA_CAN_WRITE)
#define VMA_WR  (VMA_WRITE | VMA_CAN_READ | VMA_CAN_WRITE)
#define VMA_RW  (VMA_READ | VMA_CAN_READ | VMA_WRITE | VMA_CAN_WRITE)
#define VMA_RX  (VMA_READ | VMA_CAN_READ | VMA_EXEC | VMA_CAN_EXEC)

// Flag preset
#define VMA_FG_CODE  (VMA_RX | VMA_FILE)
#define VMA_FG_DATA  (VMA_RD | VMA_COPY_ON_WRITE | VMA_FILE)
#define VMA_FG_STACK  (VMA_RW | VMA_STACK)
#define VMA_FG_HEAP  (VMA_RW | VMA_HEAP)
#define VMA_FG_ANON  (VMA_RW | VMA_ANON)
#define VMA_FG_PHYS  (VMA_RW | VMA_PHYS)
#define VMA_FG_FIFO  (VMA_RW | VMA_ANON)

#define VMA_FG_RW_FILE  (VMA_RW | VMA_FILE)
#define VMA_FG_RO_FILE  (VMA_RO | VMA_FILE)

#define VMA_FILE_RO  (VMA_RO | VMA_FILE)
#define VMA_FILE_RW  (VMA_RW | VMA_FILE)

#define VMA_ANON_RW  (VMA_RW | VMA_ANON)


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/*
 * Page fault reason
 */
#define PGFLT_MISSING  0
#define PGFLT_WRITE_DENY  1
#define PGFLT_ERROR  2

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct mspace {
    atomic_uint users;  /* Usage counter */
    bbtree_t tree;  /* Binary tree of VMAs sorted by addresses */
    page_t directory;  /* Page global directory */
    size_t lower_bound;  /* Lower bound of the address space */
    size_t upper_bound;  /* Upper bound of the address space */
    size_t v_size;  /* Virtual allocated page counter */
    size_t p_size;  /* Private allocated page counter */
    size_t s_size;  /* Shared allocated page counter */
    size_t phys_pg_count;  /* Physical page used on this address space */
    splock_t lock;  /* Memory space protection lock */
};


struct vma {
    bbnode_t node;  /* Binary tree node of VMAs contains the base address */
    size_t length;  /* The length of this VMA */
    inode_t *ino;  /* If the VMA is linked to a file, a pointer to the inode */
    off_t offset;  /* An offset to a file or an physical address, depending of type */
    off_t limit;  /* If non zero, use to specify a limit after which one the rest of pages are blank. */
    int flags;  /* VMA flags */
    struct mspace *mspace;
};


#endif  /* _KERNEL_ASM_VMA_H */
