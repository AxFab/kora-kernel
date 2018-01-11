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
#ifndef _KERNEL_CPU_H
#define _KERNEL_CPU_H 1

#include <kernel/types.h>
#include <skc/mcrs.h>

typedef int(*irq_handler_t)(void*);

/* - */
void irq_enable();
/* - */
void irq_disable();
/* - */
void irq_register(int no, irq_handler_t func, void* data);
/* - */
void irq_unregister(int no, irq_handler_t func, void *data);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int cpu_no();
/* - */
time_t cpu_time();
/* - */
void cpu_enable_mmu();
/* - */
void cpu_awake();



#define CLOCK_HZ  100 // 10ms

#endif /* _KERNEL_CPU_H */
