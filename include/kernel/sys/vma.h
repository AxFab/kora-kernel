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
 *
 *      Memory managment unit configuration.
 */
#ifndef _KERNEL_SYS_VMA_H
#define _KERNEL_SYS_VMA_H 1

#include <kernel/types.h>
#include <skc/bbtree.h>
#include <skc/splock.h>

#define VMA_EXEC (1 << 0)
#define VMA_WRITE (1 << 1)
#define VMA_READ (1 << 2)
#define VMA_SHARED (1 << 3)

#define VMA_RIGHTS 7

#define VMA_CAN_EXEC (1 << 4)
#define VMA_CAN_WRITE (1 << 5)
#define VMA_CAN_READ (1 << 6)
#define VMA_COPY_ON_WRITE (1 << 7)

#define VMA_HEAP 0x100
#define VMA_STACK 0x200
#define VMA_FILE 0x300
#define VMA_PHYS 0x400
#define VMA_ANON 0x500
#define VMA_TYPE 0xF00

#define VMA_DEAD (1 << 15)
#define VMA_LOCKED (1 << 14)

// #define VMA_CAN_GROW_UP 0x4000
// #define VMA_CAN_GROW_DOWN 0x8000
// #define VMA_NOBLOCK 0x10000
// #define VMA_HUGETABLE 0x20000

#define VMA_MAP_FIXED (1 << 16)

#define VMA_PF_NO_PAGE 1
#define VMA_PF_WRITE 2

#define VMA_CFG_MAXSIZE (128 * _Mib_)


// Flag preset
#define VMA_FG_CODE  (VMA_EXEC | VMA_READ | VMA_SHARED | VMA_CAN_EXEC | VMA_CAN_READ | VMA_COPY_ON_WRITE | VMA_FILE)
#define VMA_FG_DATAI  (VMA_READ | VMA_SHARED |VMA_CAN_WRITE | VMA_CAN_READ | VMA_COPY_ON_WRITE | VMA_FILE)
#define VMA_FG_DATA  (VMA_WRITE | VMA_READ | VMA_CAN_WRITE | VMA_CAN_READ | VMA_FILE)
#define VMA_FG_STACK  (VMA_WRITE | VMA_READ | VMA_CAN_WRITE | VMA_CAN_READ | VMA_STACK)
#define VMA_FG_HEAP  (VMA_WRITE | VMA_READ | VMA_CAN_WRITE | VMA_CAN_READ | VMA_HEAP)
#define VMA_FG_PHYS  (VMA_WRITE | VMA_READ | VMA_CAN_WRITE | VMA_CAN_READ | VMA_PHYS)

#define VMA_FG_RW_FILE  (VMA_WRITE | VMA_READ | VMA_CAN_WRITE | VMA_CAN_READ | VMA_FILE)
#define VMA_FG_RO_FILE  (VMA_READ | VMA_SHARED | VMA_CAN_READ | VMA_COPY_ON_WRITE | VMA_FILE)


typedef struct vma vma_t;

struct vma {
  bbnode_t node;
  size_t length;
  inode_t *ino;
  off_t offset;
  int flags;
};

struct mspace {
  bbtree_t tree;
  page_t directory;
  size_t lower_bound;
  size_t upper_bound;
  size_t v_size;
  size_t p_size;
  size_t s_size;
  splock_t lock;
};


#endif /* _KERNEL_SYS_VMA_H */
