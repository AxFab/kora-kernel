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


#define TSS_BASE  ((struct x86_tss*)0x1000)
#define TSS_CPU(i)  (0x1000 + sizeof(struct x86_tss) * i)
#define INTGATE  0x8E00     /* used for interruptions */
#define INTGATE_USER  0xEE00     /* used for special system calls */
#define TRAPGATE  0xEF00     /* used for system calls */

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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

PACK(struct x86_gdt {
    uint16_t lim0_15;
    uint16_t base0_15;
    uint8_t base16_23;
    uint8_t access;
    uint8_t lim16_19 : 4;
    uint8_t other : 4;
    uint8_t base24_31;
});

PACK(struct x86_idt {
    uint16_t offset0_15;
    uint16_t segment;
    uint16_t type;
    uint16_t offset16_31;
});

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Write an GDT entry on the table */
static void GDT(int no, uint32_t base, uint32_t limit, int access, int other)
{
    struct x86_gdt *ptr = (struct x86_gdt *)(0);
    ptr += no;
    ptr->lim0_15 = (limit & 0xffff);
    ptr->base0_15 = (base & 0xffff);
    ptr->base16_23 = (base & 0xff0000) >> 16;
    ptr->access = access;
    ptr->lim16_19 = (limit & 0xf0000) >> 16;
    ptr->other = (other & 0xf);
    ptr->base24_31 = (base & 0xff000000) >> 24;
}

static void IDT(int no, int segment, uint32_t address, int type)
{
    struct x86_idt *ptr = (struct x86_idt *)(0x800);
    ptr += no;
    ptr->offset0_15 = (address & 0xffff);
    ptr->segment = segment;
    ptr->type = type;
    ptr->offset16_31 = (address & 0xffff0000) >> 16;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


// create cpufeatures structs
void cpu_setup()
{
    // call for one, reach by all
    // setup pic (BSP only)
    pic_setup();
    // look for ACPI (BSP)
    acpi_setup();
    //   save prepare cpus
    //   prepare io_apic, overwrite
    // TSS
    // get features
    // map APIC
    //   send spi (BSP)
    //   wait APs all here!
    // enable features (FPU, SSE...)
    // set time (BSP)
    // upgrade PIC to APIC
    // activate interval timer
    //  - no irq yet
}

void cpu_early_init() // GDT & IDT
{
    int i;
    // GDT - Global Descriptor Table
    GDT(0, 0, 0, 0, 0); // Empty
    GDT(1, 0x0, 0xfffff, 0x9B, 0x0D); // Kernel code
    GDT(2, 0x0, 0xfffff, 0x93, 0x0D); // Kernel data
    GDT(3, 0x0, 0x00000, 0x97, 0x0D); // Kernel stack
    GDT(4, 0x0, 0xfffff, 0xff, 0x0D); // User code
    GDT(5, 0x0, 0xfffff, 0xf3, 0x0D); // User data
    GDT(6, 0x0, 0x00000, 0xf7, 0x0D); // User stack

    // TSS - Task State Segment
    // for (i = 0; i < 32; ++i) {
    //     GDT(i + 7, TSS_CPU(i), 0x67, 0xe9, 0x00); // TSS CPU(i)
    //     TSS_BASE[i].debug_flag = 0x00;
    //     TSS_BASE[i].io_map = 0x00;
    //     TSS_BASE[i].esp0 = 0x100000 + (0x1000 * (i + 1)) - 4;
    //     TSS_BASE[i].ss0 = 0x18;
    // }

    // IDT - Interupt Descriptor Table
    for (i = 0; i < 256; ++i)
        IDT(i, 0x08, (uint32_t)int_default, TRAPGATE);

    // Hardware Exception
    IDT(0x0, 0x08, (uint32_t)int_exDV, INTGATE);
    IDT(0x1, 0x08, (uint32_t)int_exDB, INTGATE);
    IDT(0x2, 0x08, (uint32_t)int_exNMI, TRAPGATE);
    IDT(0x3, 0x08, (uint32_t)int_exBP, TRAPGATE);
    IDT(0x4, 0x08, (uint32_t)int_exOF, TRAPGATE);
    IDT(0x5, 0x08, (uint32_t)int_exBR, INTGATE);
    IDT(0x6, 0x08, (uint32_t)int_exUD, INTGATE);
    IDT(0x7, 0x08, (uint32_t)int_exNM, INTGATE);
    IDT(0x8, 0x08, (uint32_t)int_exDF, INTGATE);
    IDT(0x9, 0x08, (uint32_t)int_exCO, INTGATE);
    IDT(0xA, 0x08, (uint32_t)int_exTS, INTGATE);
    IDT(0xB, 0x08, (uint32_t)int_exNP, INTGATE);
    IDT(0xC, 0x08, (uint32_t)int_exSS, INTGATE);
    IDT(0xD, 0x08, (uint32_t)int_exGP, INTGATE);
    IDT(0xE, 0x08, (uint32_t)int_exPF, INTGATE);

    IDT(0x10, 0x08, (uint32_t)int_exMF, INTGATE);
    IDT(0x11, 0x08, (uint32_t)int_exAC, INTGATE);
    IDT(0x12, 0x08, (uint32_t)int_exMC, INTGATE);
    IDT(0x13, 0x08, (uint32_t)int_exXF, INTGATE);
    IDT(0x14, 0x08, (uint32_t)int_exVE, INTGATE);
    IDT(0x15, 0x08, (uint32_t)int_exSX, INTGATE);


    // // Hardware Interrupt Request - Master
    IDT(0x20, 0x08, (uint32_t)int_irq0, INTGATE);
    IDT(0x21, 0x08, (uint32_t)int_irq1, INTGATE);
    IDT(0x22, 0x08, (uint32_t)int_irq2, INTGATE);
    IDT(0x23, 0x08, (uint32_t)int_irq3, INTGATE);
    IDT(0x24, 0x08, (uint32_t)int_irq4, INTGATE);
    IDT(0x25, 0x08, (uint32_t)int_irq5, INTGATE);
    IDT(0x26, 0x08, (uint32_t)int_irq6, INTGATE);
    IDT(0x27, 0x08, (uint32_t)int_irq7, INTGATE);

    // // Hardware Interrupt Request - Slave
    IDT(0x28, 0x08, (uint32_t)int_irq8, INTGATE);
    IDT(0x29, 0x08, (uint32_t)int_irq9, INTGATE);
    IDT(0x2A, 0x08, (uint32_t)int_irq10, INTGATE);
    IDT(0x2B, 0x08, (uint32_t)int_irq11, INTGATE);
    IDT(0x2C, 0x08, (uint32_t)int_irq12, INTGATE);
    IDT(0x2D, 0x08, (uint32_t)int_irq13, INTGATE);
    IDT(0x2E, 0x08, (uint32_t)int_irq14, INTGATE);
    IDT(0x2F, 0x08, (uint32_t)int_irq15, INTGATE);

    // Hardware Interrupt Request - IO APIC
    IDT(0x30, 0x08, (uint32_t)int_irq16, INTGATE);
    IDT(0x31, 0x08, (uint32_t)int_irq17, INTGATE);
    IDT(0x32, 0x08, (uint32_t)int_irq18, INTGATE);
    IDT(0x33, 0x08, (uint32_t)int_irq19, INTGATE);

    IDT(0x40, 0x08, (uint32_t)int_syscall, TRAPGATE);
}

