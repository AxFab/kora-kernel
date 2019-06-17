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

#include <bits/cdefs.h>
typedef unsigned long page_t;



#include <setjmp.h>

typedef void (*entry_t)(size_t);

typedef struct {
    jmp_buf jbuf;
    entry_t  entry;
    size_t param;
} cpu_state_t;


#endif /* _KERNEL_ASM_MMU_H */
