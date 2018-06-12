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
#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kora/llist.h>
#include <kernel/task.h>
#include <bits/signum.h>
#include <assert.h>

#define IRQ_COUNT 32

#define IRQ_MAX 32
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
typedef struct irq_record irq_record_t;
struct irq_vector {
    llhead_t list;
} irqv[IRQ_COUNT];

struct irq_record {
    llnode_t node;
    irq_handler_t func;
    void *data;
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void irq_register(int no, irq_handler_t func, void *data)
{
    if (no < 0 || no >= IRQ_COUNT) {
        return;
    }
    kprintf(KLOG_IRQ, "Register IRQ%d <%08x(%08x)> \n", no, func, data);
    irq_record_t *record = (irq_record_t *)kalloc(sizeof(irq_record_t));
    record->func = func;
    record->data = data;
    ll_append(&irqv[no].list, &record->node);
}

void irq_unregister(int no, irq_handler_t func, void *data)
{
    if (no < 0 || no >= IRQ_COUNT) {
        return;
    }
    irq_record_t *record;
    for ll_each(&irqv[no].list, record, irq_record_t, node) {
        if (record->func == func || record->data == data) {
            ll_remove(&irqv[no].list, &record->node);
            kfree(record);
            return;
        }
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

bool irq_enable();
bool irq_active = false;

void irq_reset(bool enable)
{
    irq_active = true;
    kCPU.irq_semaphore = 0;
    if (enable)
        IRQ_ON;
    else
        IRQ_OFF;
}

bool irq_enable()
{
    if (irq_active) {
        assert(kCPU.irq_semaphore > 0);
        if (--kCPU.irq_semaphore == 0) {
            IRQ_ON;
            return true;
        }
    }
    return false;
}

void irq_disable()
{
    if (irq_active) {
        IRQ_OFF;
        ++kCPU.irq_semaphore;
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void irq_ack(int no);

void sys_irq(int no)
{
    // irq_disable();
    assert(no >= 0 && no < IRQ_COUNT);
    irq_record_t *record;
    if (irqv[no].list.count_ == 0) {
        irq_ack(no);
        kprintf(KLOG_IRQ, "Received IRQ%d on cpu %d, no handlers.\n", no, cpu_no());
        return;
    }
    for ll_each(&irqv[no].list, record, irq_record_t, node) {
        record->func(record->data);
    }
    irq_ack(no);
    // irq_enable();
}

// #define HZ 100
// #define TICKS_PER_SEC 10000 /* 100 µs */
// int timer_cpu = 0;
// splock_t xtime_lock;
// uint64_t jiffies = 0;
// uint64_t ticks = 0;
// uint64_t ticks_last = 0;
// uint64_t ticks_elapsed = 0;

// void scheduler_ticks();

// void ticks_init()
// {
//     splock_init(&xtime_lock);
//     time_elapsed(&ticks_last);
// }

// void sys_ticks()
// {
//     if (timer_cpu == cpu_no()) {
//         splock_lock(&xtime_lock);
//         ticks += TICKS_PER_SEC / HZ;
//         ticks_elapsed += time_elapsed(&ticks_last);
//         jiffies++;
//         // Update Wall time
//         // Compute global load
//         splock_unlock(&xtime_lock);
//     }

//     // seat_ticks();
//     scheduler_ticks();
// }

void irq_enter(int no)
{
    irq_disable();
    assert(kCPU.irq_semaphore == 1);
    task_t *task = kCPU.running;
    // if (task)
    //     task->elapsed_user = time_elapsed(&task->elapsed_last);
    // kCPU.elapsed_user = time_elapsed(&kCPU->elapsed_last);

    assert(no >= 0 && no < IRQ_MAX);
    irq_record_t *record;
    if (irqv[no].list.count_ == 0) {
        kprintf(KLOG_IRQ, "Received IRQ%d, no handlers.\n", no);
    }
    for ll_each(&irqv[no].list, record, irq_record_t, node) {
        record->func(record->data);
    }
    irq_ack(no);

    // if (task)
    //     task->elapsed_others = time_elapsed(&task->elapsed_last);
    // kCPU.elapsed_io = time_elapsed(&kCPU->elapsed_last);

    assert(kCPU.irq_semaphore == 1);
    irq_reset(false);
}

void irq_fault(const fault_t *fault)
{
    assert(fault != NULL);
    // assert(kCPU.irq_semaphore == 0);
    assert(kCPU.running != NULL);
    task_t *task = kCPU.running;
    // task->elapsed_user = time_elapsed(&task->elapsed_last);
    // kCPU.elapsed_user = time_elapsed(&kCPU->elapsed_last);
    kprintf(KLOG_IRQ, "Task.%d raise exception: %s\n", fault->name);
    if (fault->raise != 0)
        task_kill(kCPU.running, fault->raise);
    if (task->status == TS_ABORTED)
        task_switch(TS_ZOMBIE, -1);
    // if (task->signals != 0)
    //     task_signals();
    // task->elapsed_system = time_elapsed(&task->elapsed_last);
    // kCPU.elapsed_system = time_elapsed(&kCPU->elapsed_last);
    // assert(kCPU.irq_semaphore == 0);
}

void irq_pagefault(size_t vaddr, int reason)
{
    irq_disable();
    assert(kCPU.irq_semaphore == 1);
    task_t *task = kCPU.running;
    // if (task)
    //     task->elapsed_user = time_elapsed(&task->elapsed_last);
    // kCPU.elapsed_user = time_elapsed(&kCPU->elapsed_last);

    if (page_fault(/*task ? task->uspace: */NULL, vaddr, reason) != 0) {
        task_kill(kCPU.running, SIGSEGV);
        if (task->status == TS_ABORTED)
            task_switch(TS_ZOMBIE, -1);
        // if (task->signals != 0)
        //     task_signals();
    }

    // if (task)
    //     task->elapsed_system = time_elapsed(&task->elapsed_last);
    // kCPU.elapsed_system = time_elapsed(&kCPU->elapsed_last);
}
