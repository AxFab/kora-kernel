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
#ifndef _KERNEL_IRQ_H
#define _KERNEL_IRQ_H 1

#include <kernel/stdc.h>


typedef void (*irq_handler_t)(void *);

void irq_reset(bool enable);
bool irq_enable();
void irq_disable();

void irq_register(int no, irq_handler_t func, void *data);
void irq_unregister(int no, irq_handler_t func, void *data);
void irq_ack(int no);

void irq_enter(int no);
void irq_fault(const char *name, unsigned signum);
long irq_syscall(unsigned no, long a1, long a2, long a3, long a4, long a5);


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define SCALL_ENTRY(n,a1,a2,a3,a4,a5,rt,s) \
    { .scall = (void*)sys_##n, .name = #n, .args = {a1, a2, a3, a4, a5, rt},\
      .split = s }

enum {
    ARG_NONE,
    ARG_STR,
    ARG_INT,
    ARG_FLG,
    ARG_LEN,
    ARG_FD,
    ARG_PTR,
};



typedef struct syscall_info syscall_info_t;

struct syscall_info {
    long(*scall)(long a1, long a2, long a3, long a4, long a5);
    const char *name;
    char args[6];
    int split;
};

#endif /* _KERNEL_IRQ_H */
