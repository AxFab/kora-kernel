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
#ifndef _KERNEL_CORE_H
#define _KERNEL_CORE_H 1

#include <kernel/arch.h>
#include <kernel/stdc.h>
#include <kernel/memory.h>

typedef struct per_cpu per_cpu_t;

struct per_cpu {
    int irq_semaphore;
};

/* - */
void cpu_setup(xtime_t *now);
/* - */
void cpu_setjmp(cpu_state_t *buf, void *stack, void *func, void *arg);
/* - */
int cpu_save(cpu_state_t *buf);
/* - */
_Noreturn void cpu_restore(cpu_state_t *buf);
/* - */
_Noreturn void cpu_halt();
/* - */
int cpu_no();

per_cpu_t *cpu_store();

/* - */
void mmu_enable();
/* - */
void mmu_leave();
/* - */
void mmu_context(mspace_t *mspace);
/* - */
int mmu_resolve(size_t vaddr, page_t phys, int flags);
/* - */
page_t mmu_read(size_t vaddr);
/* - */
page_t mmu_drop(size_t vaddr);
/* - */
page_t mmu_protect(size_t vaddr, int flags);
/* - */
void mmu_create_uspace(mspace_t *mspace);
/* - */
void mmu_destroy_uspace(mspace_t *mspace);


/* - */
void platform_start();
/* - */
void module_loader();
/* - */
int task_start(const char *name, void *func, void *arg);

#endif  /* _KERNEL_CORE_H */
