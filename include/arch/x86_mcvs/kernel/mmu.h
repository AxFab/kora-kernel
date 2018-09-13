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
#ifndef _KERNEL_ASM_MMU_H
#define _KERNEL_ASM_MMU_H 1

#include <kora/mcrs.h>

#define PAGE_SIZE 4096
typedef unsigned long page_t;
/* Larger page in order to support 36bits physical address.
typedef unsigned long long page_t; */

#define MMU_USPACE_LOWER  (4 * _Mib_)
#define MMU_USPACE_UPPER  (512 * _Mib_)
#define MMU_KSPACE_LOWER  (3072U * _Mib_)
#define MMU_KSPACE_UPPER  (4088U * _Mib_)


#define MMU_UTBL_LOW  (MMU_USPACE_LOWER / (4 * _Mib_))
#define MMU_UTBL_HIGH  (MMU_USPACE_UPPER / (4 * _Mib_))
#define MMU_KTBL_LOW  (MMU_KSPACE_LOWER / (4 * _Mib_))
#define MMU_KTBL_HIGH  (MMU_KSPACE_UPPER / (4 * _Mib_))


#define MMU_BMP  ((uint8_t*)0x120000)
#define MMU_LG  (128 * _Kib_)   // 128 Kb => 4 Gb


#define KRN_PG_DIR 0x2000
#define KRN_PG_TBL 0x3000
#define KRN_SP_LOWER 3072
#define KRN_SP_UPPER 4088

/* Page entry flags:
 *  bit0: Is present
 *  bit1: Allow write operations
 *  bit2: Accessible into user mode
 */
#define MMU_WRITE   2
#define MMU_USERMD  4

#define MMU_K_RW 3
#define MMU_K_RO 1
#define MMU_U_RO 5
#define MMU_U_RW 7


#define MMU_PAGES(s)  (*((page_t*)((0xffc00000 | ((s) >> 10)) & ~3)))
#define MMU_K_DIR  ((page_t*)0xffbff000)
#define MMU_U_DIR  ((page_t*)0xfffff000)

void grub_memory();

/* Kernel memory mapping :: 2 First MB

  0x....0000      0 Kb      2 Kb     GDT
  0x....0800      2 Kb      2 Kb     IDT
  0x....1000      4 Kb      4 Kb     TSS (x32, ~128 bytes each CPU)
  0x....2000      8 Kb      4 Kb     K pages dir
  0x....3000     12 Kb      4 Kb     K pages tbl0
  0x....4000     16 Kb    112 Kb     -
  0x...20000    128 Kb    608 Kb     Kernel code
  0x...B8000    736 Kb    288 Kb     Hardware
  0x..100000   1024 Kb    128 Kb     Kernel initial Stack (x32 each CPU)
  0x..120000   1152 Kb    128 Kb     Pages bitmap
  0x..140000   1280 Kb    256 Kb     -
  0x..180000   1536 Kb    512 Kb     Initial Heap Area
 */

#include <setjmp.h>

#define FPTR "%08p"

typedef void (*entry_t)(size_t);

typedef struct {
	jmp_buf jbuf;
	entry_t  entry;
	size_t param;
} cpu_state_t;


#endif /* _KERNEL_ASM_MMU_H */
