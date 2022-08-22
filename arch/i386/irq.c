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
#include <stddef.h>
#include <stdint.h>
#include <bits/cdefs.h>
#include <kernel/tasks.h>
#include <kernel/irq.h>
#include <sys/signum.h>
#include <errno.h>

typedef struct regs regs_t;
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

typedef struct x86fault x86fault_t;

#define x86_PFEC_PRST  (1 << 0)
#define x86_PFEC_WR  (1 << 1)
#define x86_PFEC_USR  (1 << 2)
#define x86_PFEC_RSVD  (1 << 3)
#define x86_PFEC_INSTR  (1 << 4)
#define x86_PFEC_PK  (1 << 5)

struct x86fault {
    int raise;
    const char *name;
    const char *mnemonic;
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static void dump_x86_regs(regs_t *regs)
{
    kprintf(-1, "EAX=%08x   EBX=%08x   ECX=%08x   EDX=%08x   EIP=%08x\n"
                "ESI=%08x   EDI=%08x   ESP=%08x   EBP=%08x   - - - - - - - -\n"
                "CS=%04x   DS=%04x   SS=%04x   ES=%04x   FS=%04x   GS=%04x\n",
                regs->eax, regs->ebx, regs->ecx, regs->edx, regs->eip,
                regs->esi, regs->edi, regs->esp, regs->ebp,
                regs->cs, regs->ds, regs->ss, regs->es, regs->fs, regs->gs);
    // Flags: o d l s z A P C
}

x86fault_t x86_exceptions[] = {
    { .name = "Divide by zero", .mnemonic = "#DE", .raise = SIGFPE }, // 0 - FAULT
    { .name = "Debug", .mnemonic = "#DB", .raise = SIGTRAP }, // 1 - FAULT / TRAP
    { .name = "Non-maskable Interrupt", .mnemonic = NULL, .raise = SIGFPE }, // 2 - Interrupt
    { .name = "Breakpoint", .mnemonic = "#BP", .raise = SIGTRAP }, // 3 - TRAP
    { .name = "Overflow", .mnemonic = "#OF", .raise = SIGTRAP }, // 4 - TRAP
    { .name = "Bound Range Exceeded", .mnemonic = "#BR", .raise = SIGABRT }, // 5 - FAULT
    { .name = "Invalid Opcode", .mnemonic = "#UD", .raise = SIGKILL }, // 6 - FAULT
    { .name = "Device Not Available", .mnemonic = "#NM", .raise = SIGKILL }, // 7 - FAULT
    { .name = "Double Fault", .mnemonic = "#DF", .raise = SIGKILL }, // 8 - ABORT, ERR_CODE
    { .name = "Coprocessor Segment Overrun", .mnemonic = NULL, .raise = SIGABRT }, // 9 - FAULT
    { .name = "Invalid TSS", .mnemonic = "#TS", .raise = SIGSEGV }, // 10 - FAULT, ERR_CODE
    { .name = "Segment Not Present", .mnemonic = "#NP", .raise = SIGSEGV }, // 11 - FAULT, ERR_CODE
    { .name = "Stack-Segment Fault", .mnemonic = "#SS", .raise = SIGSEGV }, // 12 - FAULT, ERR_CODE
    { .name = "General Protection Fault", .mnemonic = "#GP", .raise = SIGSEGV }, // 13 - FAULT, ERR_CODE
    { .name = "Page Fault", .mnemonic = "#PF", .raise = SIGSEGV }, // 14 - FAULT, ERR_CODE
    { .name = "Reserved", .raise = SIGKILL }, // 15 -
    { .name = "x87 Floating-Point Exception", .mnemonic = "#MF", .raise = SIGFPE }, // 16 - FAULT
    { .name = "Alignment Check", .mnemonic = "#AC", .raise = SIGABRT }, // 17 - FAULT, ERR_CODE
    { .name = "Machine Check", .mnemonic = "#MC", .raise = SIGABRT }, // 18 - ABORT
    { .name = "SIMD Floating-Point Exception", .mnemonic = "#XF", .raise = SIGFPE }, // 19 - FAULT
    { .name = "Virtualization Exception", .mnemonic = "#VE", .raise = SIGABRT }, // 20 - FAULT

    { .name = "Security Exception", .mnemonic = "#SX", .raise = SIGFPE }, // 30 - FAULT, ERR_CODE
    { .name = "Reserved", .raise = SIGKILL }, // 31 -
    { .name = "Unknown", .raise = SIGKILL }, // 32 -

    /* When the exception is a fault, the saved instruction pointer points to
     * the instruction which caused the exception. When the exception is a trap,
     * the saved instruction pointer points to the instruction after the
     * instruction which caused the exception. */
};

void x86_fault(int no, regs_t *regs)
{
    x86fault_t *fault = &x86_exceptions[MIN(0x17, (unsigned)no)];
    dump_x86_regs(regs);
    irq_fault(fault->name, fault->raise);
}

void x86_error(int no, int code, regs_t *regs)
{
    x86fault_t *fault = &x86_exceptions[MIN(0x17, (unsigned)no)];
    kprintf(-1, "\033[31m#GP: %d\033[0m\n", code);
    dump_x86_regs(regs);
    irq_fault(fault->name, fault->raise);
}

void x86_pgflt(size_t vaddr, int code, regs_t *regs)
{
    int reason = 0;
    task_t *task = __current; //kCPU.running;
    if ((code & x86_PFEC_PRST) == 0)
        reason |= PGFLT_MISSING;
    if (code & x86_PFEC_WR)
        reason |= PGFLT_WRITE;
    page_fault(task ? task->vm : NULL, vaddr, reason);
}


#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1

#define PIC_EOI 0x20


void x86_irq(int no, regs_t *regs)
{
    irq_enter(no);
    pic_ack(no);
    // kprintf(-1, "cpu%d - x86-IRQ %d\n", cpu_no(), no);
}

void x86_ipi(int no, regs_t *regs)
{
    kprintf(-1, "cpu%d - x86-IPI %d\n", cpu_no(), no);
}

void x86_syscall(regs_t *regs)
{
    int ret = irq_syscall(regs->eax, regs->ecx, regs->edx,
                          regs->ebx, regs->esi, regs->edi);
    regs->eax = ret;
    regs->edx = errno;
}

