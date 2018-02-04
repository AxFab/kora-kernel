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
 */
#ifndef _KERNEL_CPU_H
#define _KERNEL_CPU_H 1

#include <kernel/types.h>
#include <kora/mcrs.h>

typedef struct task task_t;

typedef int(*irq_handler_t)(void *);

void irq_reset(bool enable);
/* - */
bool irq_enable();
/* - */
void irq_disable();
/* - */
void irq_register(int no, irq_handler_t func, void *data);
/* - */
void irq_unregister(int no, irq_handler_t func, void *data);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int cpu_no(); // TODO optimization by writing this one as `pure'.
/* - */
time_t cpu_time();
/* - */
void cpu_enable_mmu();
/* - */
void cpu_awake();

uint64_t cpu_elapsed(uint64_t *last);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct kCpu {
    task_t *running;

    /* Time statistics */
    uint64_t last;  /* Register to compute elpased time. Unit is platform dependant. */
    uint64_t user_elapsed;  /* Time spend into user space code */
    uint64_t sys_elapsed;  /* Time spend into kernel space code */
    uint64_t irq_elapsed;  /* Time spend into IRQ handling */
    uint64_t io_elapsed;  /* Time spend into IO handling part */
    uint64_t idle_elapsed;  /* Time spend into idle state */
    uint64_t wait_elapsed;

    int errno;
};

struct kSys {
    struct kCpu *cpus[32];
};

extern struct kSys kSYS;

#define kCPU (*kSYS.cpus[cpu_no()])

#define CLOCK_HZ  100 // 10ms

#endif /* _KERNEL_CPU_H */
