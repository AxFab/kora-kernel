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

// Used for interruptions
#define INTGATE  0x8E00
// Used for special system calls
#define INTGATE_USER  0xEE00
// Used for system calls
#define TRAPGATE  0xEF00


typedef struct x86_idesc x86_idesc_t;

PACK(struct x86_idesc {
    uint16_t offset0_15;
    uint16_t segment;
    uint16_t type;
    uint16_t offset16_31;
});

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void int_default();
void int_syscall();

void int_exDV();
void int_exDB();
void int_exNMI();
void int_exBP();
void int_exOF();
void int_exBR();
void int_exUD();
void int_exNM();
void int_exDF();
void int_exCO();
void int_exTS();
void int_exNP();
void int_exSS();
void int_exGP();
void int_exPF();
void int_exMF();
void int_exAC();
void int_exMC();
void int_exXF();
void int_exVE();
void int_exSX();

void int_irq0();
void int_irq1();
void int_irq2();
void int_irq3();
void int_irq4();
void int_irq5();
void int_irq6();
void int_irq7();
void int_irq8();
void int_irq9();
void int_irq10();
void int_irq11();
void int_irq12();
void int_irq13();
void int_irq14();
void int_irq15();
void int_irq16();
void int_irq17();
void int_irq18();
void int_irq19();
void int_irq20();
void int_irq21();
void int_irq22();
void int_irq23();
void int_irq24();
void int_irq25();
void int_irq26();
void int_irq27();
void int_irq28();
void int_irq29();
void int_irq30();
void int_irq31();

void int_isr123();
void int_isr124();
void int_isr125();
void int_isr126();
void int_isr127();
void int_irqLT();

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


static void x86_write_idesc(int no, int segment, uint32_t address, int type)
{
    x86_idesc_t *idesc = (void *)(0x800 + no * sizeof(x86_idesc_t));
    idesc->offset0_15 = (address & 0xffff);
    idesc->segment = segment;
    idesc->type = type;
    idesc->offset16_31 = (address & 0xffff0000) >> 16;
}

void x86_idt()
{
    int i;
    for (i = 0; i < 256; ++i)
        x86_write_idesc(i, 0x08, (uint32_t)int_default, TRAPGATE);

    // Hardware Exception
    x86_write_idesc(0x0, 0x08, (uint32_t)int_exDV, INTGATE);
    x86_write_idesc(0x1, 0x08, (uint32_t)int_exDB, INTGATE);
    x86_write_idesc(0x2, 0x08, (uint32_t)int_exNMI, TRAPGATE);
    x86_write_idesc(0x3, 0x08, (uint32_t)int_exBP, TRAPGATE);
    x86_write_idesc(0x4, 0x08, (uint32_t)int_exOF, TRAPGATE);
    x86_write_idesc(0x5, 0x08, (uint32_t)int_exBR, INTGATE);
    x86_write_idesc(0x6, 0x08, (uint32_t)int_exUD, INTGATE);
    x86_write_idesc(0x7, 0x08, (uint32_t)int_exNM, INTGATE);
    x86_write_idesc(0x8, 0x08, (uint32_t)int_exDF, INTGATE);
    x86_write_idesc(0x9, 0x08, (uint32_t)int_exCO, INTGATE);
    x86_write_idesc(0xA, 0x08, (uint32_t)int_exTS, INTGATE);
    x86_write_idesc(0xB, 0x08, (uint32_t)int_exNP, INTGATE);
    x86_write_idesc(0xC, 0x08, (uint32_t)int_exSS, INTGATE);
    x86_write_idesc(0xD, 0x08, (uint32_t)int_exGP, INTGATE);
    x86_write_idesc(0xE, 0x08, (uint32_t)int_exPF, INTGATE);

    x86_write_idesc(0x10, 0x08, (uint32_t)int_exMF, INTGATE);
    x86_write_idesc(0x11, 0x08, (uint32_t)int_exAC, INTGATE);
    x86_write_idesc(0x12, 0x08, (uint32_t)int_exMC, INTGATE);
    x86_write_idesc(0x13, 0x08, (uint32_t)int_exXF, INTGATE);
    x86_write_idesc(0x14, 0x08, (uint32_t)int_exVE, INTGATE);
    x86_write_idesc(0x15, 0x08, (uint32_t)int_exSX, INTGATE);

    // Hardware Interrupt Request
    x86_write_idesc(0x20, 0x08, (uint32_t)int_irq0, INTGATE);
    x86_write_idesc(0x21, 0x08, (uint32_t)int_irq1, INTGATE);
    x86_write_idesc(0x22, 0x08, (uint32_t)int_irq2, INTGATE);
    x86_write_idesc(0x23, 0x08, (uint32_t)int_irq3, INTGATE);
    x86_write_idesc(0x24, 0x08, (uint32_t)int_irq4, INTGATE);
    x86_write_idesc(0x25, 0x08, (uint32_t)int_irq5, INTGATE);
    x86_write_idesc(0x26, 0x08, (uint32_t)int_irq6, INTGATE);
    x86_write_idesc(0x27, 0x08, (uint32_t)int_irq7, INTGATE);
    x86_write_idesc(0x28, 0x08, (uint32_t)int_irq8, INTGATE);
    x86_write_idesc(0x29, 0x08, (uint32_t)int_irq9, INTGATE);
    x86_write_idesc(0x2A, 0x08, (uint32_t)int_irq10, INTGATE);
    x86_write_idesc(0x2B, 0x08, (uint32_t)int_irq11, INTGATE);
    x86_write_idesc(0x2C, 0x08, (uint32_t)int_irq12, INTGATE);
    x86_write_idesc(0x2D, 0x08, (uint32_t)int_irq13, INTGATE);
    x86_write_idesc(0x2E, 0x08, (uint32_t)int_irq14, INTGATE);
    x86_write_idesc(0x2F, 0x08, (uint32_t)int_irq15, INTGATE);
    x86_write_idesc(0x30, 0x08, (uint32_t)int_irq16, INTGATE);
    x86_write_idesc(0x31, 0x08, (uint32_t)int_irq17, INTGATE);
    x86_write_idesc(0x32, 0x08, (uint32_t)int_irq18, INTGATE);
    x86_write_idesc(0x33, 0x08, (uint32_t)int_irq19, INTGATE);
    x86_write_idesc(0x34, 0x08, (uint32_t)int_irq20, INTGATE);
    x86_write_idesc(0x35, 0x08, (uint32_t)int_irq21, INTGATE);
    x86_write_idesc(0x36, 0x08, (uint32_t)int_irq22, INTGATE);
    x86_write_idesc(0x37, 0x08, (uint32_t)int_irq23, INTGATE);
    x86_write_idesc(0x38, 0x08, (uint32_t)int_irq24, INTGATE);
    x86_write_idesc(0x39, 0x08, (uint32_t)int_irq25, INTGATE);
    x86_write_idesc(0x3A, 0x08, (uint32_t)int_irq26, INTGATE);
    x86_write_idesc(0x3B, 0x08, (uint32_t)int_irq27, INTGATE);
    x86_write_idesc(0x3C, 0x08, (uint32_t)int_irq28, INTGATE);
    x86_write_idesc(0x3D, 0x08, (uint32_t)int_irq29, INTGATE);
    x86_write_idesc(0x3E, 0x08, (uint32_t)int_irq30, INTGATE);
    x86_write_idesc(0x3F, 0x08, (uint32_t)int_irq31, INTGATE);

    // Syscalls entry
    x86_write_idesc(0x40, 0x08, (uint32_t)int_syscall, TRAPGATE);

    // APIC ISR
    x86_write_idesc(0x7B, 0x08, (uint32_t)int_isr123, INTGATE);
    x86_write_idesc(0x7C, 0x08, (uint32_t)int_isr124, INTGATE);
    x86_write_idesc(0x7D, 0x08, (uint32_t)int_isr125, INTGATE);
    x86_write_idesc(0x7E, 0x08, (uint32_t)int_isr126, INTGATE);
    x86_write_idesc(0x7F, 0x08, (uint32_t)int_isr127, INTGATE_USER);

    x86_write_idesc(0xFF, 0x08, (uint32_t)int_irqLT, INTGATE); // | 0x60
}
