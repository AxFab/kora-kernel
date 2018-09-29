/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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

#define PAGE_SIZE 4096
typedef unsigned long page_t;
/* Larger page in order to support 36bits physical address.
typedef unsigned long long page_t; */

#define MMU_USPACE_LOWER  (1 * _Mib_)
#define MMU_USPACE_UPPER  (512U * _Mib_)
#define MMU_KSPACE_LOWER  (1024U * _Mib_)
#define MMU_KSPACE_UPPER  ((1024U + 16) * _Mib_)


#define MMU_BMP  (mmu_bmp)
#define MMU_LG  (128 / 8)   // 512 * PAGE_SIZE => 2 Mb
extern unsigned char mmu_bmp[];


#define FPTR "%016p"
typedef size_t cpu_state_t[8];


#endif /* _KERNEL_ASM_MMU_H */
