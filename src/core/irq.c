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
#include <kernel/stdc.h>
#include <kernel/tasks.h>
#include <kernel/irq.h>
#include <kernel/syscalls.h>
#include <kernel/core.h>
#include <kora/llist.h>
#include <sys/signum.h>

extern sys_info_t sysinfo;

void irq_zero()
{
    cpu_info_t * pc = kcpu();
    pc->irq_semaphore = 0;
}

void irq_reset(bool enable)
{
    cpu_info_t * pc = kcpu();
    assert(pc == NULL || pc->irq_semaphore <= 1);
    if (enable) {
        if (pc)
            pc->irq_semaphore = 0;
        __asm_irq_on_;
    } else {
        if (pc)
            pc->irq_semaphore = 1;
        __asm_irq_off_;
    }
}


bool irq_enable()
{
    cpu_info_t * pc = kcpu();
    if (pc != NULL && sysinfo.is_ready) {
        assert(pc->irq_semaphore > 0);
        if (--pc->irq_semaphore == 0) {
            __asm_irq_on_;
            return true;
        }
    }
    return false;
}

void irq_disable()
{
    cpu_info_t * pc = kcpu();
    if (pc != NULL && sysinfo.is_ready) {
        __asm_irq_off_;
        ++pc->irq_semaphore;
    }
}

bool irq_ready()
{
    cpu_info_t * pc = kcpu();
    if (pc != NULL && sysinfo.is_ready)
        return pc->irq_semaphore == 0;
    return true;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
#define IRQ_COUNT 32

typedef void (*irq_handler_t)(void *);

typedef struct irq_record irq_record_t;

struct irq_vector {
    llhead_t list;
    splock_t lock;
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
    splock_lock(&irqv[no].lock);
    ll_append(&irqv[no].list, &record->node);
    splock_unlock(&irqv[no].lock);
}

void irq_unregister(int no, irq_handler_t func, void *data)
{
    if (no < 0 || no >= IRQ_COUNT)
        return;
    irq_record_t *record;
    splock_lock(&irqv[no].lock);
    for ll_each(&irqv[no].list, record, irq_record_t, node) {
        if (record->func == func || record->data == data) {
            ll_remove(&irqv[no].list, &record->node);
            kfree(record);
            break;
        }
    }
    splock_unlock(&irqv[no].lock);
}


void irq_enter(int no)
{
    assert(no >= 0 && no < IRQ_COUNT);
    assert(irq_ready());
    irq_disable();
    task_t *task = __current;
    // __current = NULL; // We need it on scheduler_switch!

    // clock_elapsed(CPU_IRQ);

    irq_record_t *record;
    if (irqv[no].list.count_ == 0)
        kprintf(KL_IRQ, "Received IRQ%d, on CPU%d: no handlers.\n", no, cpu_no());
    // else if (no != 0)
    //     kprintf(KL_IRQ, "Received IRQ%d, on CPU%d: %d handler(s).\n", no, cpu_no(), irqv[no].list.count_);

    for ll_each(&irqv[no].list, record, irq_record_t, node)
        record->func(record->data);

    //clock_elapsed(prev);
    irq_zero();
    assert(__current == task);
    __current = task;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void irq_fault(const char *name, unsigned signum)
{
    if (__current == NULL) {
        kprintf(KL_ERR, "Fault on CPU%d: %s\n", cpu_no(), name);
        for (;;);
    }

    kprintf(KL_ERR, "Task.%d] Failure on CPU%d: %s\n", __current->pid, cpu_no(), name);
    if (signum != 0)
        task_raise(__current, signum);
    scheduler_switch(TS_READY);
}



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

EXPORT_SYMBOL(irq_enable, 0);
EXPORT_SYMBOL(irq_disable, 0);
EXPORT_SYMBOL(irq_register, 0);
EXPORT_SYMBOL(irq_unregister, 0);
