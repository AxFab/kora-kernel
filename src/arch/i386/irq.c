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



/* Acknowledge the processor than a IRQ have been handled */
void irq_ack(int no)
{
    if (no >= 16) {
        apic[APIC_EIO] = 0;
        return;
    }
    if (no >= 8)
        outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

/* Disable a specific IRQ */
void irq_mask(int no)
{
    if (no >= 16) {
        return;
    } else if (no >= 8) {
        outb(PIC2_DATA, inb(PIC2_DATA) | 1 << (no - 8));
    } else {
        outb(PIC1_DATA, inb(PIC1_DATA) | 1 << no);
    }
}

/* Enable a specific IRQ */
void irq_unmask(int no)
{
    if (no >= 16) {
        return;
    } else if (no >= 8) {
        outb(PIC2_DATA, inb(PIC2_DATA) & ~(1 << (no - 8)));
    } else {
        outb(PIC1_DATA, inb(PIC1_DATA) & ~(1 << no));
    }
}

/* x86 entry after an IRQ */
void irq_x86(int no, regs_t *regs) // -> irq_enter()
{
    irq_enter(no);
}


void irq_trap_x86(int no, regs_t *regs)
{
    irq_fault(&x86_exceptions[MIN(0x20, (unsigned)no)]);
}

void irq_fault_x86(int no, int code, regs_t *regs)
{
    char buf[64];
    fault_t fault = x86_exceptions[MIN(0x20, (unsigned)no)];
    snprint(buf, 64, fault->name, code);
    fault->name = buf;
    irq_fault(&fault);
}

void irq_pgflt_x86(size_t vaddr, int code, regs_t *regs)
{
    int flags = 0;
    if (code & 1)
        flags |= PFLT_PRESENT;
    irq_pagefault(vaddr, flags);
}

void irq_syscall_x86(regs_t *regs)
{
    int ret = irq_syscall(regs->eax, regs->ecx, regs->edx, regs->ebx, regs->esi, regs->edi);
    regs->eax = ret;
    regs->edx = errno;
}