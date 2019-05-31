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
#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kora/llist.h>
#include <kernel/task.h>
#include <kernel/syscalls.h>
#include <kora/syscalls.h>
#include <sys/signum.h>
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
    if (no < 0 || no >= IRQ_COUNT)
        return;
    kprintf(KLOG_IRQ, "Register IRQ%d <%08x(%08x)> \n", no, func, data);
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

void clock_elapsed(int status)
{
    int64_t ticks = cpu_clock(0);
    int64_t elapsed = ticks - kCPU.last_elapsed;
    if (kCPU.status == CPU_SYS)
        kCPU.sys_elapsed += elapsed;
    else if (kCPU.status == CPU_USER)
        kCPU.user_elapsed += elapsed;
    if (kCPU.status == CPU_IRQ)
        kCPU.irq_elapsed += elapsed;
    if (kCPU.status == CPU_IDLE)
        kCPU.idle_elapsed += elapsed;
    kCPU.last_elapsed = ticks;
    kCPU.status = status;
}

void irq_ack(int no);


void irq_enter(int no)
{
    irq_disable();
    assert(kCPU.irq_semaphore == 1);

    int status = kCPU.status;
    clock_elapsed(CPU_IRQ);

    assert(no >= 0 && no < IRQ_MAX);
    irq_record_t *record;
    if (irqv[no].list.count_ == 0)
        kprintf(KLOG_IRQ, "Received IRQ%d, on CPU%d: no handlers.\n", no, cpu_no());
    for ll_each(&irqv[no].list, record, irq_record_t, node)
        record->func(record->data);
    irq_ack(no);


    clock_elapsed(status);

    assert(kCPU.irq_semaphore == 1);
    irq_reset(false);
}

#ifdef KORA_KRN

void irq_fault(const fault_t *fault)
{
    assert(fault != NULL);
    // assert(kCPU.irq_semaphore == 0);
    // assert(kCPU.running != NULL);
    task_t *task = kCPU.running;
    // task->elapsed_user = clock_elapsed(&task->elapsed_last);
    // kCPU.elapsed_user = clock_elapsed(&kCPU->elapsed_last);
    if (task == NULL) {
        stackdump(8);
        kprintf(KLOG_IRQ, "Fault on CPU%d raise exception: %s\n", cpu_no(), fault->name);
        kpanic("Kernel trigger an exception\n");
    }

    kprintf(KLOG_IRQ, "Task.%d on CPU%d raise exception: %s\n", task->pid, cpu_no(), fault->name);
    mspace_display(task->usmem);
    stackdump(8);
    if (fault->raise != 0)
        task_kill(kCPU.running, fault->raise);
    if (task->status == TS_ABORTED)
        scheduler_switch(TS_ZOMBIE, -1);
    // if (task->signals != 0)
    //     task_signals();
    // task->elapsed_system = clock_elapsed(&task->elapsed_last);
    // kCPU.elapsed_system = clock_elapsed(&kCPU->elapsed_last);
    // assert(kCPU.irq_semaphore == 0);
    sys_exit(5); // DEBUG
}

void irq_pagefault(size_t vaddr, int reason)
{
    irq_disable();
    assert(kCPU.irq_semaphore == 1);
    mspace_t *mspace = NULL;
    task_t *task = kCPU.running;
    if (task) {
        mspace = task->usmem;
        // task->elapsed_user = clock_elapsed(&task->elapsed_last);
    }
    // kCPU.elapsed_user = clock_elapsed(&kCPU->elapsed_last);

    if (page_fault(mspace, vaddr, reason) != 0) {
        task_kill(kCPU.running, SIGSEGV);
        if (task->status == TS_ABORTED)
            scheduler_switch(TS_ZOMBIE, -1);
        // if (task->signals != 0)
        //     task_signals();
    }

    // if (task)
    //     task->elapsed_system = clock_elapsed(&task->elapsed_last);
    // kCPU.elapsed_system = clock_elapsed(&kCPU->elapsed_last);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define SCALL_ENTRY(i, n,a,r)  [i] = { #n, a, (void*)sys_##n, txt_##n, r }

scall_entry_t syscall_entries[64] = {
    // SYS_POWER
    // SYS_SCALL
    // SCALL_ENTRY(SYS_SYSLOG, sys_syslog, "%p", false),
    // SCALL_ENTRY(SYS_SYSINFO, sys_sysinfo, "%d, %p, %d", false),

    // SYS_YIELD
    SCALL_ENTRY(SYS_EXIT, exit, "%d", false),
    // SYS_WAIT
    // SYS_EXEC
    // SYS_CLONE

    // SYS_SIGRAISE
    // SYS_SIGACTION
    // SYS_SIGRETURN

    SCALL_ENTRY(SYS_MMAP, mmap, "%p, %p, 0%o, %d, %d", true),
    SCALL_ENTRY(SYS_MUNMAP, munmap, "%p, %p", true),
    // SYS_MPROTECT

    SCALL_ENTRY(SYS_OPEN, open, "%d, \"%s\", 0%o, 0%o", true),
    SCALL_ENTRY(SYS_CLOSE, close, "%d", true),
    SCALL_ENTRY(SYS_READ, read, "%d, %p, %d", true),
    SCALL_ENTRY(SYS_WRITE, write, "%d, %p, %d", true),
    // SYS_SEEK

    SCALL_ENTRY(SYS_WINDOW, window, "%d, %d, %d, 0%o", true),
    SCALL_ENTRY(SYS_FCNTL, fcntl, "%d, %d, %p", true),
    // SYS_PIPE
};


long irq_syscall(long no, long a1, long a2, long a3, long a4, long a5)
{
    if (no < 0 || no > 64 || &syscall_entries[no] == NULL || syscall_entries[no].name == NULL) {
        kprintf(-1, "\033[96msyscall(%d) = -1\033[0m\n", no);
        return -1;
    }

    long ret;
    int pid = kCPU.running->pid;
    scall_entry_t *entry = &syscall_entries[no];
    char arg_buf[50];
    snprintf(arg_buf, 50, entry->args, a1, a2, a3, a4, a5);

    if (!entry->ret)
        kprintf(-1, "[task %3d] \033[96m%s(%s)\033[0m\n", pid, entry->name, arg_buf);
    ret = entry->routine(a1, a2, a3, a4, a5);
    if (entry->ret)
        kprintf(-1, "[task %3d] \033[96m%s(%s) = %d\033[0m\n", pid, entry->name, arg_buf, ret);

    return ret;
}

#endif
