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
#include <kernel/task.h>
#include <errno.h>
#include <sys/signum.h>
#include "apic.h"
#include "pic.h"

fault_t x86_exceptions[] = {
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

/* Acknowledge the processor than a IRQ have been handled */
extern size_t *apic_regs;
void irq_ack(int no)
{
    if (apic_regs != NULL) {
        if (no >= 16) {
            apic_regs[APIC_EOI] = 0;
            return;
        }
    } else {
        if (no >= 8)
            outb(PIC2_CMD, PIC_EOI);
        outb(PIC1_CMD, PIC_EOI);
    }
}

/* Disable a specific IRQ */
void irq_mask(int no)
{
    if (no >= 16)
        return;

    else if (no >= 8)
        outb(PIC2_DATA, inb(PIC2_DATA) | 1 << (no - 8));

    else
        outb(PIC1_DATA, inb(PIC1_DATA) | 1 << no);
}

/* Enable a specific IRQ */
void irq_unmask(int no)
{
    if (no >= 16)
        return;

    else if (no >= 8)
        outb(PIC2_DATA, inb(PIC2_DATA) & ~(1 << (no - 8)));

    else
        outb(PIC1_DATA, inb(PIC1_DATA) & ~(1 << no));
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define x86_PFEC_PRST  (1 << 0)
#define x86_PFEC_WR  (1 << 1)
#define x86_PFEC_USR  (1 << 2)
#define x86_PFEC_RSVD  (1 << 3)
#define x86_PFEC_INSTR  (1 << 4)
#define x86_PFEC_PK  (1 << 5)


void x86_fault(int no, regs_t *regs)
{
    kprintf(-1, "REGS\n");
    kprintf(-1, "  eax:%08x, ecx:%08x, edx:%08x, ebx:%08x, \n", regs->eax, regs->ecx, regs->edx, regs->ebx);
    kprintf(-1, "  ebp:%08x, esp:%08x, esi:%08x, edi:%08x, \n", regs->ebp, regs->esp, regs->esi, regs->edi);
    kprintf(-1, "  cs:%02x, ds:%02x, eip:%08x, eflags:%08x \n", regs->cs, regs->ds, regs->eip, regs->eflags);
    irq_fault(&x86_exceptions[MIN(0x20, (unsigned)no)]);
}

void x86_error(int no, int code, regs_t *regs)
{
    char buf[64];
    fault_t fault = x86_exceptions[MIN(0x20, (unsigned)no)];
    snprintf(buf, 64, fault.name, code);
    fault.name = buf;
    irq_fault(&fault);
}

void x86_pgflt(size_t vaddr, int code, regs_t *regs)
{
    int reason = 0;
    task_t *task = kCPU.running;
    if ((code & x86_PFEC_PRST) == 0)
        reason |= PGFLT_MISSING;
    if (code & x86_PFEC_WR)
        reason |= PGFLT_WRITE;
    page_fault(task ? task->usmem : NULL, vaddr, reason);
}

void x86_syscall(regs_t *regs)
{
    int ret = irq_syscall(regs->eax, regs->ecx, regs->edx,
                          regs->ebx, regs->esi, regs->edi);

    regs->eax = ret;
    regs->edx = errno;
}

