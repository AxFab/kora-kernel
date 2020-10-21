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
#include <kernel/stdc.h>
#include <kernel/tasks.h>
#include <kernel/irq.h>
#include <kora/llist.h>


bool __irq_active = false;
int __irq_semaphore = 0; // TODO -- per cpu

void irq_reset(bool enable)
{
    assert(__irq_semaphore <= 1);
    __irq_active = true;
    if (enable) {
        __irq_semaphore = 0;
        __asm_irq_on_;
    } else {
        __irq_semaphore = 1;
        __asm_irq_off_;
    }
}


bool irq_enable()
{
    if (__irq_active) {
        assert(__irq_semaphore > 0);
        if (--__irq_semaphore == 0) {
            __asm_irq_on_;
            return true;
        }
    }
    return false;
}

void irq_disable()
{
    if (__irq_active) {
        __asm_irq_off_;
        ++__irq_semaphore;
    }
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
#define IRQ_COUNT 32

typedef void (*irq_handler_t)(void *);

typedef struct irq_record irq_record_t;

struct irq_vector {
    llhead_t list;
    // TODO -- Lock !? what if it doesn't return !
} irqv[IRQ_COUNT];

struct irq_record {
    llnode_t node;
    irq_handler_t func;
    void *data;
};



void irq_register(int no, irq_handler_t func, void *data)
{
    if (no < 0 || no >= IRQ_COUNT)
        return;
    kprintf(KL_IRQ, "Register IRQ%d <%08x(%08x)>\n", no, func, data);
    irq_record_t *record = (irq_record_t *)kalloc(sizeof(irq_record_t));
    record->func = func;
    record->data = data;
    ll_append(&irqv[no].list, &record->node);
}

void irq_unregister(int no, irq_handler_t func, void *data)
{
    if (no < 0 || no >= IRQ_COUNT)
        return;
    irq_record_t *record;
    for ll_each(&irqv[no].list, record, irq_record_t, node) {
        if (record->func == func || record->data == data) {
            ll_remove(&irqv[no].list, &record->node);
            kfree(record);
            return;
        }
    }
}


void irq_enter(int no)
{
    assert(no >= 0 && no < IRQ_COUNT);
    assert(__irq_semaphore == 0);
    irq_disable();
    // clock_elapsed(CPU_IRQ);

    irq_record_t *record;
    if (irqv[no].list.count_ == 0)
        kprintf(KL_IRQ, "Received IRQ%d, on CPU%d: no handlers.\n", no, cpu_no());

    irq_ack(no);
    for ll_each(&irqv[no].list, record, irq_record_t, node)
        record->func(record->data);

    //clock_elapsed(prev);
    __irq_semaphore = 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void irq_fault(const char *name, unsigned signum)
{
    if (__current == NULL) {
        kprintf(KL_ERR, "Fault on CPU%d: %s\n", cpu_no(), name);
        for (;;);
    }

    kprintf(KL_ERR, "Failure task.%d on CPU%d: %s\n", __current->pid, cpu_no(), name);
    if (signum != 0)
        task_raise(&__scheduler, __current, signum);
    scheduler_switch(TS_READY);
}



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

long irq_syscall(long no, long a1, long a2, long a3, long a4, long a5)
{
    assert(__irq_semaphore == 0);

    return -1;
}

