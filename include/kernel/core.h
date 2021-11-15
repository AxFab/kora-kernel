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


typedef struct cpu_info cpu_info_t;
typedef struct sys_info sys_info_t;


struct cpu_info {
    int id;
    int irq_semaphore;
    int mode;
    void *stack;
    acpu_info_t *arch;
    uint64_t ticks_system;
    uint64_t ticks_user;
    uint64_t ticks_irq;
    uint64_t ticks_idle;
};

struct sys_info {
    int cpu_count;
    int is_ready;
    xtime_t uptime;
    cpu_info_t *cpu_table;
    asys_info_t *arch;
};


#define KMODE_SYSTEM 0
#define KMODE_IRQ 1
#define KMODE_USER 2
#define KMODE_IDLE 3


sys_info_t *ksys();
cpu_info_t *kcpu();


// /* - */
// void cpu_setup(xtime_t *now);
// /* - */
// void cpu_setjmp(cpu_state_t *buf, void *stack, void *func, void *arg);
// /* - */
// int cpu_save(cpu_state_t *buf);
// /* - */
// _Noreturn void cpu_restore(cpu_state_t *buf);
// /* - */
// _Noreturn void cpu_halt();
// /* - */
// int cpu_no();

// per_cpu_t *cpu_store();

// /* - */
// void mmu_enable();
// /* - */
// void mmu_leave();
// /* - */
// void mmu_context(mspace_t *mspace);
// /* - */
// int mmu_resolve(size_t vaddr, page_t phys, int flags);
// /* - */
// page_t mmu_read(size_t vaddr);
// /* - */
// page_t mmu_drop(size_t vaddr);
// /* - */
// page_t mmu_protect(size_t vaddr, int flags);
// /* - */
// void mmu_create_uspace(mspace_t *mspace);
// /* - */
// void mmu_destroy_uspace(mspace_t *mspace);


/* - */
void arch_start();
/* - */
void module_loader();
/* - */
int task_start(const char *name, void *func, void *arg);

#endif  /* _KERNEL_CORE_H */
