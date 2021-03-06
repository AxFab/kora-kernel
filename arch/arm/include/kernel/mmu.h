/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#ifndef _KERNEL_ASM_MMU_H
#define _KERNEL_ASM_MMU_H 1

#include <kora/mcrs.h>
#include <bits/cdefs.h>

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



#define KRN_PG_DIR 0x2000
#define KRN_PG_TBL 0x3000
#define KRN_SP_LOWER 3072
#define KRN_SP_UPPER 4088

/* Page entry flags:
 *  bit0: Is present
 *  bit1: Allow write operations
 *  bit2: Accesible into user mode
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


typedef size_t cpu_state_t[6];


#endif /* _KERNEL_ASM_MMU_H */
