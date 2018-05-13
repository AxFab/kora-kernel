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
#include <kernel/asm/vma.h>
#include <kernel/asm/mmu.h>
#include <kernel/memory.h>
#include <kernel/task.h>
#include <string.h>
#include <assert.h>
#include <bits/signum.h>
#include <errno.h>

// #define kTSK  (*kCPU.task)

void sys_irq(int no);

#define SGM_CODE_KERNEL 0x10

struct regs {
    uint16_t gs, unused_g;
    uint16_t fs, unused_f;
    uint16_t es, unused_e;
    uint16_t ds, unused_d;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t tmp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eip;
    uint16_t cs, unused_c;
    uint32_t eflags;
    /* ESP and SS are only pop on priviledge change  */
    uint32_t esp;
    uint32_t ss;
};

struct cpu_ex {
    int no;
    int signum;
    int flags;
    const char *name;
    const char *mnemonic;
};

struct cpu_ex signal_exception_x86[] = {
    { 0, SIGFPE, 0, "Divide by zero", "#DE" }, // FAULT
    { 1, SIGTRAP, 0, "Debug", "#DB" }, // FAULT / TRAP
    { 2, SIGFPE, 0, "Non-maskable Interrupt", NULL }, // Interrupt
    { 3, SIGTRAP, 0, "Breakpoint", "#BP" }, // TRAP
    { 4, SIGTRAP, 0, "Overflow", "#OF" }, // TRAP
    { 5, SIGABRT, 0, "Bound Range Exceeded", "#BR" }, // FAULT
    { 6, SIGKILL, 0, "Invalid Opcode", "#UD" }, // FAULT
    { 7, SIGKILL, 0, "Device Not Available", "#NM" }, // FAULT
    { 8, SIGKILL, 0, "Double Fault", "#DF" }, // ABORT, ERR_CODE
    { 9, SIGABRT, 0, "Coprocessor Segment Overrun", NULL }, // FAULT
    { 10, SIGSEGV, 0, "Invalid TSS", "#TS" }, // FAULT, ERR_CODE
    { 11, SIGSEGV, 0, "Segment Not Present", "#NP" }, // FAULT, ERR_CODE
    { 12, SIGSEGV, 0, "Stack-Segment Fault", "#SS" }, // FAULT, ERR_CODE
    { 13, SIGSEGV, 0, "General Protection Fault", "#GP" }, // FAULT, ERR_CODE
    { 14, SIGSEGV, 0, "Page Fault", "#PF" }, // FAULT, ERR_CODE
    { 15, SIGKILL, 0, "Reserved", NULL },
    { 16, SIGFPE, 0, "x87 Floating-Point Exception", "#MF" }, // FAULT
    { 17, SIGABRT, 0, "Alignment Check", "#AC" }, // FAULT, ERR_CODE
    { 18, SIGABRT, 0, "Machine Check", "#MC" }, // ABORT
    { 19, SIGFPE, 0, "SIMD Floating-Point Exception", "#XF" }, // FAULT
    { 20, SIGABRT, 0, "Virtualization Exception", "#VE" }, // FAULT
    // { 30, SIGFPE, 0, "Security Exception", "#SX" }, // FAULT, ERR_CODE

    /* When the exception is a fault, the saved instruction pointer points to
     * the instruction which caused the exception. When the exception is a trap,
     * the saved instruction pointer points to the instruction after the
     * instruction which caused the exception. */
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

long kernel_scall(long no, long a1, long a2, long a3, long a4, long a5);

void sys_call_x86(regs_t *regs)
{
    assert(kCPU.running);
    assert(regs->cs != SGM_CODE_KERNEL);

    // task_enter_sys(regs, false);
    // kTSK.regs = regs;
    // kprintf(KLOG_DBG, "[x86 ] SYS CALL\n");
    long ret = kernel_scall(regs->eax, regs->ecx, regs->edx, regs->ebx, regs->esi,
                            regs->edi);
    regs->eax = ret;
    regs->ebx = errno;
    // regs->edx = ret >> 32;
    // task_signals();
    // task_leave_sys();
}

void sys_wait_x86(regs_t *regs)
{
    assert(kCPU.running);
    // task_enter_sys(regs, regs->cs == SGM_CODE_KERNEL);
    // kprintf(KLOG_DBG, "[x86 ] SYS WAIT\n");
    // task_pause(TS_INTERRUPTIBLE);
    // task_signals();
    // task_leave_sys();
}

void sys_sigret_x86(regs_t *regs)
{
    assert(kCPU.running);
    // task_enter_sys(regs, regs->cs == SGM_CODE_KERNEL);
    // // kprintf(KLOG_DBG, "[x86 ] SYS SIG_RETURN\n");
    // cpu_return_signal(kCPU.running, regs);
    // task_signals();
    // task_leave_sys();
}

void cpu_exception_x86(int no, regs_t *regs)
{
    assert(no >= 0 && no <= 20);

    // task_enter_sys(NULL, regs->cs == SGM_CODE_KERNEL);
    kprintf(KLOG_ERR, "[x86 ] Detected a cpu exception: %d - %s\n", no,
            signal_exception_x86[no].name);
    if (kCPU.running == NULL)
        kpanic("Kernel code triggered an exception.\n");
        // TODO -- If a module is responsable, close this module and send an alert.
    // task_kill(kCPU.running, signal_exception_x86[no].signum);
    // task_switch();
    for (;;);
}

void cpu_error_x86(int no, int code, regs_t *regs)
{
    cpu_exception_x86(no, regs);
}

void page_error_x86(int code, regs_t *regs)
{
    cpu_exception_x86(13, regs);
}


#define x86_PFEC_PRST  (1 << 0)
#define x86_PFEC_WR  (1 << 1)
#define x86_PFEC_USR  (1 << 2)
#define x86_PFEC_RSVD  (1 << 3)
#define x86_PFEC_INSTR  (1 << 4)
#define x86_PFEC_PK  (1 << 5)

void page_fault_x86(size_t address, int code, regs_t *regs)
{
    irq_disable();
    // task_enter_sys(NULL, regs->cs == SGM_CODE_KERNEL);
    mspace_t *mem = kCPU.running ? mem = kCPU.running->usmem : NULL;
    int reason = 0;
    if ((code & x86_PFEC_PRST) == 0) {
        reason |= PGFLT_MISSING;
    } if (code & x86_PFEC_WR) {
        reason |= PGFLT_WRITE;
    // } else {
    //     reason = PGFLT_ERROR;
    }
    // kprintf(KLOG_DBG, "[CPU ] #PF %08x (%o)\n", address, code);
    int ret = page_fault(mem, address, reason);
    // kprintf(KLOG_PF, "\e[91m#PF\e[0m at %08x\n", address);
    if (ret < 0) {
        stackdump(90);
        if (kCPU.running) {
            // task_kill(kCPU.running, SIGSEGV);
            // task_signals();
        } else {
            cpu_exception_x86(14, regs);
        }
    }
    // task_leave_sys();
    irq_enable();
}

void sys_irq_x86(int no, regs_t *regs)
{
    // task_enter_sys(regs, regs->cs == SGM_CODE_KERNEL);
    // kTSK.regs = regs;
    // kprintf(KLOG_DBG, "[x86 ] IRQ %d\n", no);
    // bufdump(regs, 0x60);
    sys_irq(no);
    kCPU.io_elapsed += cpu_elapsed(&kCPU.last);
    if (kCPU.running) {
        kCPU.running->other_elapsed += cpu_elapsed(&kCPU.running->last);
    }
    // task_signals();
    // task_leave_sys();
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// static void cpu_setup_stack(task_t *task, size_t instr, size_t stack,
//                             long arg)
// {
//     int i;
//     // TODO -- Is arch dependent!
//     int sp = task->kstack_len / sizeof(size_t);

//     task->kstack[--sp] = arg; // ARG
//     task->kstack[--sp] = 0; // ARG
//     if (task->usmem) {
//         task->kstack[--sp] = 0x33; // SS
//         task->kstack[--sp] = stack; // ESP
//         task->kstack[--sp] = 0x200; // FLAGS
//         task->kstack[--sp] = 0x23; // CS
//     } else {
//         task->kstack[--sp] = 0x200; // FLAGS
//         task->kstack[--sp] = 0x08; // CS
//     }
//     task->kstack[--sp] = instr; // EIP

//     task->kstack[--sp] = arg; // EAX (ARG)
//     for (i = 0; i < 7; ++i) {
//         task->kstack[--sp] = 0;    // ECX - EDI
//     }

//     if (task->usmem) {
//         for (i = 0; i < 4; ++i) {
//             task->kstack[--sp] = 0x2B;    // DS - GD
//         }
//     } else {
//         for (i = 0; i < 4; ++i) {
//             task->kstack[--sp] = 0x10;    // DS - GD
//         }
//     }
//     // task->rp = 0;
//     task->regs/*[0]*/ = (regs_t *)&task->kstack[sp];
// }

// void cpu_setup_task(task_t *task, size_t entry, long args)
// {
//     size_t ustask = ((size_t)task->ustack) + task->ustack_len - 0x16;
//     cpu_setup_stack(task, entry, ustask, args);
// }

// void cpu_setup_signal(task_t *task, size_t entry, long args)
// {
//     task->sig_regs = (regs_t*)kalloc(sizeof(regs_t));
//     memcpy(task->sig_regs, task->regs/*[task->rp]*/, sizeof(regs_t));
//     size_t ustask = task->regs/*[task->rp]*/->esp;
//     cpu_setup_stack(task, entry, ustask, args);
// }

// void cpu_return_signal(task_t *task, regs_t *regs)
// {
//     task->regs = regs;
//     memcpy(task->regs/*[task->rp]*/, task->sig_regs, sizeof(regs_t));
//     kfree(task->sig_regs);
//     task->sig_regs = NULL;
// }

// bool cpu_task_return_uspace(task_t *task)
// {
//     return task->regs/*[task->rp]*/->cs != 0x08;
// }
