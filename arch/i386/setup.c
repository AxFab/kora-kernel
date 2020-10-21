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
#include <kernel/stdc.h>
#include <kernel/arch.h>
#include <kernel/tasks.h>
#include <string.h>
#include "apic.h"


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
void int_irqLT();

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
void GDT(int no, uint32_t base, uint32_t limit, int access, int other)
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

void IDT(int no, int segment, uint32_t address, int type)
{
    struct x86_idt *ptr = (struct x86_idt *)(0x800);
    ptr += no;
    ptr->offset0_15 = (address & 0xffff);
    ptr->segment = segment;
    ptr->type = type;
    ptr->offset16_31 = (address & 0xffff0000) >> 16;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_ENABLE 0x800


// EAX, EBX, EDX, ECX
#define x86_FEATURES_FPU(cpu)   (0 != ((cpu)[2] & (1 << 0)))
#define x86_FEATURES_VME(cpu)   (0 != ((cpu)[2] & (1 << 1)))
#define x86_FEATURES_PE(cpu)   (0 != ((cpu)[2] & (1 << 2)))
#define x86_FEATURES_PSE(cpu)   (0 != ((cpu)[2] & (1 << 3)))
#define x86_FEATURES_TSC(cpu)   (0 != ((cpu)[2] & (1 << 4)))
#define x86_FEATURES_MSR(cpu)   (0 != ((cpu)[2] & (1 << 5)))
#define x86_FEATURES_PAE(cpu)   (0 != ((cpu)[2] & (1 << 6)))
#define x86_FEATURES_MCE(cpu)   (0 != ((cpu)[2] & (1 << 7)))
#define x86_FEATURES_CX8(cpu)   (0 != ((cpu)[2] & (1 << 8)))
#define x86_FEATURES_APIC(cpu)   (0 != ((cpu)[2] & (1 << 9)))
#define x86_FEATURES_SEP(cpu)   (0 != ((cpu)[2] & (1 << 11)))
#define x86_FEATURES_MTRR(cpu)   (0 != ((cpu)[2] & (1 << 12)))
#define x86_FEATURES_PGE(cpu)   (0 != ((cpu)[2] & (1 << 13)))
#define x86_FEATURES_MCA(cpu)   (0 != ((cpu)[2] & (1 << 14)))
#define x86_FEATURES_CMOV(cpu)   (0 != ((cpu)[2] & (1 << 15)))
#define x86_FEATURES_PAT(cpu)   (0 != ((cpu)[2] & (1 << 16)))
#define x86_FEATURES_PSE36(cpu)   (0 != ((cpu)[2] & (1 << 17)))
#define x86_FEATURES_PSN(cpu)   (0 != ((cpu)[2] & (1 << 18)))
#define x86_FEATURES_CLF(cpu)   (0 != ((cpu)[2] & (1 << 19)))
#define x86_FEATURES_DTES(cpu)   (0 != ((cpu)[2] & (1 << 21)))
#define x86_FEATURES_ACPI(cpu)   (0 != ((cpu)[2] & (1 << 22)))
#define x86_FEATURES_MMX(cpu)   (0 != ((cpu)[2] & (1 << 23)))
#define x86_FEATURES_FXSR(cpu)   (0 != ((cpu)[2] & (1 << 24)))
#define x86_FEATURES_SSE(cpu)   (0 != ((cpu)[2] & (1 << 25)))
#define x86_FEATURES_SSE2(cpu)   (0 != ((cpu)[2] & (1 << 26)))
#define x86_FEATURES_SS(cpu)   (0 != ((cpu)[2] & (1 << 27)))
#define x86_FEATURES_HTT(cpu)   (0 != ((cpu)[2] & (1 << 28)))
#define x86_FEATURES_TM1(cpu)   (0 != ((cpu)[2] & (1 << 29)))
#define x86_FEATURES_IA64(cpu)   (0 != ((cpu)[2] & (1 << 30)))
#define x86_FEATURES_PBE(cpu)   (0 != ((cpu)[2] & (1 << 31)))

#define x86_FEATURES_SSE3(cpu)   (0 != ((cpu)[3] & (1 << 0)))
#define x86_FEATURES_PCLMUL(cpu)   (0 != ((cpu)[3] & (1 << 1)))
#define x86_FEATURES_DTES64(cpu)   (0 != ((cpu)[3] & (1 << 2)))
#define x86_FEATURES_MONITOR(cpu)   (0 != ((cpu)[3] & (1 << 3)))
#define x86_FEATURES_DS_CPL(cpu)   (0 != ((cpu)[3] & (1 << 4)))
#define x86_FEATURES_VMX(cpu)   (0 != ((cpu)[3] & (1 << 5)))
#define x86_FEATURES_SMX(cpu)   (0 != ((cpu)[3] & (1 << 6)))
#define x86_FEATURES_EST(cpu)   (0 != ((cpu)[3] & (1 << 7)))
#define x86_FEATURES_TM2(cpu)   (0 != ((cpu)[3] & (1 << 8)))
#define x86_FEATURES_SSSE3(cpu)   (0 != ((cpu)[3] & (1 << 9)))
#define x86_FEATURES_CID(cpu)   (0 != ((cpu)[3] & (1 << 10)))
#define x86_FEATURES_FMA(cpu)   (0 != ((cpu)[3] & (1 << 12)))
#define x86_FEATURES_CX16(cpu)   (0 != ((cpu)[3] & (1 << 13)))
#define x86_FEATURES_ETPRD(cpu)   (0 != ((cpu)[3] & (1 << 14)))
#define x86_FEATURES_PDCM(cpu)   (0 != ((cpu)[3] & (1 << 15)))
#define x86_FEATURES_DCA(cpu)   (0 != ((cpu)[3] & (1 << 18)))
#define x86_FEATURES_SSE4_1(cpu)   (0 != ((cpu)[3] & (1 << 19)))
#define x86_FEATURES_SSE4_2(cpu)   (0 != ((cpu)[3] & (1 << 20)))
#define x86_FEATURES_x2APIC(cpu)  (0 != ((cpu)[3] & (1 << 21)))
#define x86_FEATURES_MOVBE(cpu)   (0 != ((cpu)[3] & (1 << 22)))
#define x86_FEATURES_POPCNT(cpu)   (0 != ((cpu)[3] & (1 << 23)))
#define x86_FEATURES_AES(cpu)   (0 != ((cpu)[3] & (1 << 25)))
#define x86_FEATURES_XSAVE(cpu)   (0 != ((cpu)[3] & (1 << 26)))
#define x86_FEATURES_OSXSAVE(cpu)   (0 != ((cpu)[3] & (1 << 27)))
#define x86_FEATURES_AVX(cpu)   (0 != ((cpu)[3] & (1 << 28)))

// Read 64bits data from MSR registers
static void rdmsr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}


int all_features[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

void cpuid_setup()
{
    int cpu_name[5];
    if (cpu_table == NULL) {
        cpu_table = (cpu_x86_t *)kalloc(sizeof(cpu_x86_t));
        cpu_table[0].stack = 0x4000;
    }

    // Get CPU Vendor name
    x86_cpuid(0, 0, cpu_name);
    cpu_name[4] = 0;
    cpu_table[cpu_no()].vendor = strdup((char *)&cpu_name[1]);

    // Get CPU features
    x86_cpuid(1, 0, cpu_table[cpu_no()].features);
    all_features[0] &= cpu_table[cpu_no()].features[0];
    all_features[1] &= cpu_table[cpu_no()].features[1];
    all_features[2] &= cpu_table[cpu_no()].features[2];
    all_features[3] &= cpu_table[cpu_no()].features[3];


    char *tmp = kalloc(1024);
    tmp[0] = 0;
    if (x86_FEATURES_FPU(all_features))
        strcat(tmp, "FPU, ");
    if (x86_FEATURES_VME(all_features))
        strcat(tmp, "VME, ");
    if (x86_FEATURES_PE(all_features))
        strcat(tmp, "PE, ");
    if (x86_FEATURES_PSE(all_features))
        strcat(tmp, "PSE, ");
    if (x86_FEATURES_TSC(all_features))
        strcat(tmp, "TSC, ");
    if (x86_FEATURES_MSR(all_features))
        strcat(tmp, "MSR, ");
    if (x86_FEATURES_PAE(all_features))
        strcat(tmp, "PAE, ");
    if (x86_FEATURES_MCE(all_features))
        strcat(tmp, "MCE, ");
    if (x86_FEATURES_CX8(all_features))
        strcat(tmp, "CX8, ");
    if (x86_FEATURES_APIC(all_features))
        strcat(tmp, "APIC, ");
    if (x86_FEATURES_SEP(all_features))
        strcat(tmp, "SEP, ");
    if (x86_FEATURES_MTRR(all_features))
        strcat(tmp, "MTRR, ");
    if (x86_FEATURES_PGE(all_features))
        strcat(tmp, "PGE, ");
    if (x86_FEATURES_MCA(all_features))
        strcat(tmp, "MCA, ");
    if (x86_FEATURES_CMOV(all_features))
        strcat(tmp, "CMOV, ");
    if (x86_FEATURES_PAT(all_features))
        strcat(tmp, "PAT, ");
    if (x86_FEATURES_PSE36(all_features))
        strcat(tmp, "PSE36, ");
    if (x86_FEATURES_PSN(all_features))
        strcat(tmp, "PSN, ");
    if (x86_FEATURES_CLF(all_features))
        strcat(tmp, "CLF, ");
    if (x86_FEATURES_DTES(all_features))
        strcat(tmp, "DTES, ");
    if (x86_FEATURES_ACPI(all_features))
        strcat(tmp, "ACPI, ");
    if (x86_FEATURES_MMX(all_features))
        strcat(tmp, "MMX, ");
    if (x86_FEATURES_FXSR(all_features))
        strcat(tmp, "FXSR, ");
    if (x86_FEATURES_SSE(all_features))
        strcat(tmp, "SSE, ");
    if (x86_FEATURES_SSE2(all_features))
        strcat(tmp, "SSE2, ");
    if (x86_FEATURES_SS(all_features))
        strcat(tmp, "SS, ");
    if (x86_FEATURES_HTT(all_features))
        strcat(tmp, "HTT, ");
    if (x86_FEATURES_TM1(all_features))
        strcat(tmp, "TM1, ");
    if (x86_FEATURES_IA64(all_features))
        strcat(tmp, "IA64, ");
    if (x86_FEATURES_PBE(all_features))
        strcat(tmp, "PBE, ");
    if (x86_FEATURES_SSE3(all_features))
        strcat(tmp, "SSE3, ");
    if (x86_FEATURES_PCLMUL(all_features))
        strcat(tmp, "PCLMUL, ");
    if (x86_FEATURES_DTES64(all_features))
        strcat(tmp, "DTES64, ");
    if (x86_FEATURES_MONITOR(all_features))
        strcat(tmp, "MONITOR, ");
    if (x86_FEATURES_DS_CPL(all_features))
        strcat(tmp, "DS_CPL, ");
    if (x86_FEATURES_VMX(all_features))
        strcat(tmp, "VMX, ");
    if (x86_FEATURES_SMX(all_features))
        strcat(tmp, "SMX, ");
    if (x86_FEATURES_EST(all_features))
        strcat(tmp, "EST, ");
    if (x86_FEATURES_TM2(all_features))
        strcat(tmp, "TM2, ");
    if (x86_FEATURES_SSSE3(all_features))
        strcat(tmp, "SSSE3, ");
    if (x86_FEATURES_CID(all_features))
        strcat(tmp, "AVX, ");
    if (x86_FEATURES_FMA(all_features))
        strcat(tmp, "FMA, ");
    if (x86_FEATURES_CX16(all_features))
        strcat(tmp, "CX16, ");
    if (x86_FEATURES_ETPRD(all_features))
        strcat(tmp, "ETPRD, ");
    if (x86_FEATURES_PDCM(all_features))
        strcat(tmp, "PDCM, ");
    if (x86_FEATURES_DCA(all_features))
        strcat(tmp, "DCA, ");
    if (x86_FEATURES_SSE4_1(all_features))
        strcat(tmp, "SSE4_1, ");
    if (x86_FEATURES_SSE4_2(all_features))
        strcat(tmp, "SSE4_2, ");
    if (x86_FEATURES_x2APIC(all_features))
        strcat(tmp, "x2APIC, ");
    if (x86_FEATURES_MOVBE(all_features))
        strcat(tmp, "MOVBE, ");
    if (x86_FEATURES_POPCNT(all_features))
        strcat(tmp, "POPCNT, ");
    if (x86_FEATURES_AES(all_features))
        strcat(tmp, "AES, ");
    if (x86_FEATURES_XSAVE(all_features))
        strcat(tmp, "XSAVE, ");
    if (x86_FEATURES_OSXSAVE(all_features))
        strcat(tmp, "OSXSAVE, ");
    if (x86_FEATURES_AVX(all_features))
        strcat(tmp, "AVX, ");

    kprintf(KL_DBG, "CPU(%d) :: %s :: %s\n", cpu_no(), &cpu_name[1], tmp);
    kfree(tmp);

    /*
    if (x86_FEATURES_FPU(*cpu)) {
        x86_active_FPU();
    }
    if (x86_FEATURES_SSE(*cpu)) {
        x86_enable_SSE();
    } */

    if (!x86_FEATURES_MSR(cpu_table[cpu_no()].features)) {
        kprintf(KL_MSG, "CPU%d: No MSR capability\n", cpu_no());
        return;
    } else if (!x86_FEATURES_APIC(cpu_table[cpu_no()].features)) {
        kprintf(KL_MSG, "CPU%d: No APIC capability\n", cpu_no());
        return;
    }

    if (cpu_no() == 0) {
        uint32_t regA, regB;
        rdmsr(IA32_APIC_BASE_MSR, &regA, &regB);
        if ((regA & (1 << 11)) == 0) {
            kprintf(KL_MSG, "CPU%d: APIC disabled\n", cpu_no());
            return;
        } else if ((regA & (1 << 8)) == 0) {
            kprintf(KL_MSG, "CPU%d: Unfound BSP\n", cpu_no());
            return;
        }

        apic_mmio = regA & ~(PAGE_SIZE - 1);
        kprintf(KL_MSG, "CPU%d: APIC at %p\n", cpu_no(), apic_mmio);
    }
}

// #include <kernel/task.h>

// void cpu_tss(task_t *task)
// {
//     int i = cpu_no();
//     size_t ptr = (size_t)task->kstack + PAGE_SIZE - 16;
//     TSS_BASE[i].esp0 = ptr;
//     // kprintf(-1, "CPU%d set TSS at %p \n", i, TSS_BASE[i].esp0);
// }

void tss_setup()
{
    int i = cpu_no();
    // kprintf(-1, "CPU%d - at %p \n", i, &kCPU);

    TSS_BASE[i].debug_flag = 0;
    TSS_BASE[i].io_map = 0;
    TSS_BASE[i].esp0 = cpu_table[i].stack + PAGE_SIZE - 16;
    TSS_BASE[i].ss0 = 0x18;
    GDT(i + 7, TSS_CPU(i), 0x67, 0xe9, 0x00); // TSS CPU(i)
    x86_set_tss(i + 7);

    kprintf(-1, "CPU(%d) TSS no %d at %x using stack %x\n", i, i + 7, &TSS_BASE[i], TSS_BASE[i].esp0);
}

// create cpufeatures structs
void cpu_setup()
{
    // call for one, reach by all
    // setup pic (BSP only)
    // pic_setup();
    // look for ACPI (BSP)
    pic_setup();
    // acpi_setup();
    cpuid_setup();
    xtime_t now = rtc_time();
    // kprintf(0, "Unix Epoch: %lld \n", now);
    // apic_setup();
    tss_setup();
    // hpet_setup();
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
    // kprintf(KL_ERR, "End of CPU setup.\n");
    // for (;;);
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


    // // Hardware Interrupt Request
    IDT(0x20, 0x08, (uint32_t)int_irq0, INTGATE);
    IDT(0x21, 0x08, (uint32_t)int_irq1, INTGATE);
    IDT(0x22, 0x08, (uint32_t)int_irq2, INTGATE);
    IDT(0x23, 0x08, (uint32_t)int_irq3, INTGATE);
    IDT(0x24, 0x08, (uint32_t)int_irq4, INTGATE);
    IDT(0x25, 0x08, (uint32_t)int_irq5, INTGATE);
    IDT(0x26, 0x08, (uint32_t)int_irq6, INTGATE);
    IDT(0x27, 0x08, (uint32_t)int_irq7, INTGATE);
    IDT(0x28, 0x08, (uint32_t)int_irq8, INTGATE);
    IDT(0x29, 0x08, (uint32_t)int_irq9, INTGATE);
    IDT(0x2A, 0x08, (uint32_t)int_irq10, INTGATE);
    IDT(0x2B, 0x08, (uint32_t)int_irq11, INTGATE);
    IDT(0x2C, 0x08, (uint32_t)int_irq12, INTGATE);
    IDT(0x2D, 0x08, (uint32_t)int_irq13, INTGATE);
    IDT(0x2E, 0x08, (uint32_t)int_irq14, INTGATE);
    IDT(0x2F, 0x08, (uint32_t)int_irq15, INTGATE);
    IDT(0x30, 0x08, (uint32_t)int_irq16, INTGATE);
    IDT(0x31, 0x08, (uint32_t)int_irq17, INTGATE);
    IDT(0x32, 0x08, (uint32_t)int_irq18, INTGATE);
    IDT(0x33, 0x08, (uint32_t)int_irq19, INTGATE);
    IDT(0x34, 0x08, (uint32_t)int_irq20, INTGATE);
    IDT(0x35, 0x08, (uint32_t)int_irq21, INTGATE);
    IDT(0x36, 0x08, (uint32_t)int_irq22, INTGATE);
    IDT(0x37, 0x08, (uint32_t)int_irq23, INTGATE);
    IDT(0x38, 0x08, (uint32_t)int_irq24, INTGATE);
    IDT(0x39, 0x08, (uint32_t)int_irq25, INTGATE);
    IDT(0x3A, 0x08, (uint32_t)int_irq26, INTGATE);
    IDT(0x3B, 0x08, (uint32_t)int_irq27, INTGATE);
    IDT(0x3C, 0x08, (uint32_t)int_irq28, INTGATE);
    IDT(0x3D, 0x08, (uint32_t)int_irq29, INTGATE);
    IDT(0x3E, 0x08, (uint32_t)int_irq30, INTGATE);
    IDT(0x3F, 0x08, (uint32_t)int_irq31, INTGATE);

    IDT(0x40, 0x08, (uint32_t)int_syscall, TRAPGATE);

    IDT(0xFF, 0x08, (uint32_t)int_irqLT, INTGATE);
}

