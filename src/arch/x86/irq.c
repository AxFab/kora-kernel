/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <assert.h>
#include <stdbool.h>
#include <bits/atomic.h>
#include <kernel/core.h>
#include <kernel/cpu.h>

bool irq_enable();
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

bool irq_active = false;

void irq_reset(bool enable)
{
    irq_active = true;
    kCPU.irq_semaphore = enable ? 1 : 0;
    asm("cli");
    if (enable)
        irq_enable();
}

bool irq_enable()
{
    if (irq_active) {
        assert(kCPU.irq_semaphore > 0);
        if (--kCPU.irq_semaphore == 0) {
            asm("sti");
            return true;
        }
    }
    return false;
}

void irq_disable()
{
    if (irq_active) {
        asm("cli");
        ++kCPU.irq_semaphore;
    }
}

#define PIC1_CMD 0x20
#define PIC2_CMD 0xA0
#define PIC_EOI 0x20


#define APIC_EOI  (0xB0 / 4) // EOI Register
extern volatile uint32_t *apic;

void irq_ack(int no)
{
    if (no >= 16) {
        apic[APIC_EOI] = 1;
        return;
    }
    if (no >= 8)
        outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}


