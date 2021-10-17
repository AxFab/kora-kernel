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
#ifndef __KERNEL_CPU_H
#define __KERNEL_CPU_H 1

#include <stddef.h>
#include <stdint.h>

typedef size_t cpu_state_t[6];

#define IRQ_ON ((void)0)
#define IRQ_OFF ((void)0)

#define KSTACK (PAGE_SIZE)

void outl(int, uint32_t);
void outw(int, uint16_t);
uint32_t inl(int);
uint16_t inw(int);


#endif  /* __KERNEL_CPU_H */
