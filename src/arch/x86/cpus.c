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
 *
 *      Intel x86 CPU features.
 */
#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kernel/vma.h>
#include <kernel/mmu.h>
#include <bits/atomic.h>
#include <string.h>
#include <assert.h>
#include <time.h>


#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_ENABLE 0x800

void smp_start();
void smp_error();

#define cpu_count (*((uint16_t*)0x7f8)) // See start.asm - SMP_CPU_COUNT  0x7f8
#define AP_VECTOR (((size_t)smp_start) >> 12)
#define AE_VECTOR (((size_t)smp_error) >> 12)



struct x86_CPU {
    char vendor[16];
    int features[4];
};


struct x86_CPU all_cpu = {
    "",
    { 0xFF, 0xFF, 0xFF, 0xFF },
};

// EAX, EBX, EDX, ECX
#define x86_FEATURES_FPU(cpu)   (0 != ((cpu).features[2] & (1 << 0)))
#define x86_FEATURES_VME(cpu)   (0 != ((cpu).features[2] & (1 << 1)))
#define x86_FEATURES_PE(cpu)   (0 != ((cpu).features[2] & (1 << 2)))
#define x86_FEATURES_PSE(cpu)   (0 != ((cpu).features[2] & (1 << 3)))
#define x86_FEATURES_TSC(cpu)   (0 != ((cpu).features[2] & (1 << 4)))
#define x86_FEATURES_MSR(cpu)   (0 != ((cpu).features[2] & (1 << 5)))
#define x86_FEATURES_PAE(cpu)   (0 != ((cpu).features[2] & (1 << 6)))
#define x86_FEATURES_MCE(cpu)   (0 != ((cpu).features[2] & (1 << 7)))
#define x86_FEATURES_CX8(cpu)   (0 != ((cpu).features[2] & (1 << 8)))
#define x86_FEATURES_APIC(cpu)   (0 != ((cpu).features[2] & (1 << 9)))
#define x86_FEATURES_SEP(cpu)   (0 != ((cpu).features[2] & (1 << 11)))
#define x86_FEATURES_MTRR(cpu)   (0 != ((cpu).features[2] & (1 << 12)))
#define x86_FEATURES_PGE(cpu)   (0 != ((cpu).features[2] & (1 << 13)))
#define x86_FEATURES_MCA(cpu)   (0 != ((cpu).features[2] & (1 << 14)))
#define x86_FEATURES_CMOV(cpu)   (0 != ((cpu).features[2] & (1 << 15)))
#define x86_FEATURES_PAT(cpu)   (0 != ((cpu).features[2] & (1 << 16)))
#define x86_FEATURES_PSE36(cpu)   (0 != ((cpu).features[2] & (1 << 17)))
#define x86_FEATURES_PSN(cpu)   (0 != ((cpu).features[2] & (1 << 18)))
#define x86_FEATURES_CLF(cpu)   (0 != ((cpu).features[2] & (1 << 19)))
#define x86_FEATURES_DTES(cpu)   (0 != ((cpu).features[2] & (1 << 21)))
#define x86_FEATURES_ACPI(cpu)   (0 != ((cpu).features[2] & (1 << 22)))
#define x86_FEATURES_MMX(cpu)   (0 != ((cpu).features[2] & (1 << 23)))
#define x86_FEATURES_FXSR(cpu)   (0 != ((cpu).features[2] & (1 << 24)))
#define x86_FEATURES_SSE(cpu)   (0 != ((cpu).features[2] & (1 << 25)))
#define x86_FEATURES_SSE2(cpu)   (0 != ((cpu).features[2] & (1 << 26)))
#define x86_FEATURES_SS(cpu)   (0 != ((cpu).features[2] & (1 << 27)))
#define x86_FEATURES_HTT(cpu)   (0 != ((cpu).features[2] & (1 << 28)))
#define x86_FEATURES_TM1(cpu)   (0 != ((cpu).features[2] & (1 << 29)))
#define x86_FEATURES_IA64(cpu)   (0 != ((cpu).features[2] & (1 << 30)))
#define x86_FEATURES_PBE(cpu)   (0 != ((cpu).features[2] & (1 << 31)))

#define x86_FEATURES_SSE3(cpu)   (0 != ((cpu).features[3] & (1 << 0)))
#define x86_FEATURES_PCLMUL(cpu)   (0 != ((cpu).features[3] & (1 << 1)))
#define x86_FEATURES_DTES64(cpu)   (0 != ((cpu).features[3] & (1 << 2)))
#define x86_FEATURES_MONITOR(cpu)   (0 != ((cpu).features[3] & (1 << 3)))
#define x86_FEATURES_DS_CPL(cpu)   (0 != ((cpu).features[3] & (1 << 4)))
#define x86_FEATURES_VMX(cpu)   (0 != ((cpu).features[3] & (1 << 5)))
#define x86_FEATURES_SMX(cpu)   (0 != ((cpu).features[3] & (1 << 6)))
#define x86_FEATURES_EST(cpu)   (0 != ((cpu).features[3] & (1 << 7)))
#define x86_FEATURES_TM2(cpu)   (0 != ((cpu).features[3] & (1 << 8)))
#define x86_FEATURES_SSSE3(cpu)   (0 != ((cpu).features[3] & (1 << 9)))
#define x86_FEATURES_CID(cpu)   (0 != ((cpu).features[3] & (1 << 10)))
#define x86_FEATURES_FMA(cpu)   (0 != ((cpu).features[3] & (1 << 12)))
#define x86_FEATURES_CX16(cpu)   (0 != ((cpu).features[3] & (1 << 13)))
#define x86_FEATURES_ETPRD(cpu)   (0 != ((cpu).features[3] & (1 << 14)))
#define x86_FEATURES_PDCM(cpu)   (0 != ((cpu).features[3] & (1 << 15)))
#define x86_FEATURES_DCA(cpu)   (0 != ((cpu).features[3] & (1 << 18)))
#define x86_FEATURES_SSE4_1(cpu)   (0 != ((cpu).features[3] & (1 << 19)))
#define x86_FEATURES_SSE4_2(cpu)   (0 != ((cpu).features[3] & (1 << 20)))
#define x86_FEATURES_x2APIC(cpu)  (0 != ((cpu).features[3] & (1 << 21)))
#define x86_FEATURES_MOVBE(cpu)   (0 != ((cpu).features[3] & (1 << 22)))
#define x86_FEATURES_POPCNT(cpu)   (0 != ((cpu).features[3] & (1 << 23)))
#define x86_FEATURES_AES(cpu)   (0 != ((cpu).features[3] & (1 << 25)))
#define x86_FEATURES_XSAVE(cpu)   (0 != ((cpu).features[3] & (1 << 26)))
#define x86_FEATURES_OSXSAVE(cpu)   (0 != ((cpu).features[3] & (1 << 27)))
#define x86_FEATURES_AVX(cpu)   (0 != ((cpu).features[3] & (1 << 28)))


#define APIC_ID  (0x20 / 4)
#define APIC_VERS  (0x30 / 4)
#define APIC_TPR  (0x80 / 4)
#define APIC_APR  (0x90 / 4)
#define APIC_PPR  (0xA0 / 4)
#define APIC_EOI  (0xB0 / 4)
#define APIC_RRD  (0xC0 / 4)
#define APIC_LRD  (0xD0 / 4)
#define APIC_DRD  (0xE0 / 4)
#define APIC_SVR  (0xF0 / 4)

#define APIC_ESR  (0x280 / 4)
#define APIC_ICR_LOW  (0x300 / 4)
#define APIC_ICR_HIGH  (0x310 / 4)
#define APIC_LVT3  (0x370 / 4) // LVT Error register


void x86_cpuid(int, int, int *);
void x86_enable_MMU();
void x86_enable_SSE();
void x86_active_FPU();
void x86_active_cache();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
// Read 64bits data from MSR registers
static void x86_rdmsr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

// Write 64bits data on MSR registers
// static void x86_wrmsr(uint32_t msr, uint32_t lo, uint32_t hi)
// {
//   asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

atomic_uint x86_delayTimer;
bool delay_reg = false;

void x86_delayIRQ()
{
    // Timer is in microseconds
    atomic32_xadd(&x86_delayTimer, 1000000 / CLOCK_HZ);
}

void x86_delayX(unsigned int microsecond)
{
    x86_delayTimer = 0;
    while (x86_delayTimer < microsecond)
        asm("pause");
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void x86_cpu_features(struct x86_CPU *cpu)
{
    // Get CPU Vendor name
    int cpu_name[5];
    x86_cpuid(0, 0, cpu_name);
    cpu_name[4] = 0;
    strcpy(cpu->vendor, (char *)&cpu_name[1]);

    x86_cpuid(0, 0, cpu->features);

    if (x86_FEATURES_FPU(*cpu))
        x86_active_FPU();
    if (x86_FEATURES_SSE(*cpu))
        x86_enable_SSE();

    all_cpu.features[0] &= cpu->features[0];
    all_cpu.features[1] &= cpu->features[1];
    all_cpu.features[2] &= cpu->features[2];
    all_cpu.features[3] &= cpu->features[3];
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct tm datetime;
int datetime_set = 0;

struct tm RTC_GetTime();
void PIT_set_interval(unsigned frequency);
uint64_t cpu_ticks();

time_t cpu_time()
{
    if (datetime_set == 0) {
        datetime = RTC_GetTime();
        datetime_set = 1;
    }
    return mktime(&datetime);
}

uint64_t time_elapsed(uint64_t *last)
{
    uint64_t ticks = cpu_ticks();
    if (last == NULL)
        return ticks;
    uint64_t elapsed = ticks - *last;
    *last = ticks;
    return elapsed;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

bool APIC_ON = false;
volatile uint32_t *apic;

int cpu_no()
{
    if (!APIC_ON)
        return 0;

    return (apic[APIC_ID] >> 24) & 0xf;
}

void cpu_on()
{
    apic[APIC_SVR] |= 0x800;
}

/* Request to start other CPUs */
void cpu_awake()
{
    struct x86_CPU cpu;
    // Check cpu features first
    x86_cpu_features(&cpu);

    if (!x86_FEATURES_MSR(cpu)) {
        kprintf(KLOG_MSG, "CPU vendor: %s, No MSR capability\n", cpu.vendor);
        return;
    } else if (!x86_FEATURES_APIC(cpu)) {
        kprintf(KLOG_MSG, "CPU vendor: %s, No APIC capability\n", cpu.vendor);
        return;
    }

    uint32_t regA, regB;
    x86_rdmsr(IA32_APIC_BASE_MSR, &regA, &regB);
    if ((regA & (1 << 11)) == 0) {
        kprintf(KLOG_MSG, "CPU vendor: %s, APIC disabled\n", cpu.vendor);
        return;
    } else if ((regA & (1 << 8)) == 0) {
        kprintf(KLOG_MSG, "CPU vendor: %s, unfound BSP\n", cpu.vendor);
        return;
    }

    // Map APIC
    // kprintf(0, "CPU vendor: %s, all good...\n", cpu.vendor);
    apic = (uint32_t *)kmap(PAGE_SIZE, NULL, regA & 0xfffff000,
                            VMA_PHYSIQ | VMA_UNCACHABLE);
    // kprintf(0, "Map APIC \n");

    // PIT_set_interval(CLOCK_HZ * 10);

    // irq_register(0, (irq_handler_t)x86_delayIRQ, NULL);
    // irq_reset(true);
    // bool irq = irq_enable();
    // assert(irq);


    // Set the Spourious Interrupt Vector Register bit 8 to start receiving interrupts
    // apic[APIC_SVR] |= 0x800; // Enable the APIC
    APIC_ON = true;

    cpu_apic();

    kprintf(KLOG_MSG, "Blocked, waiting APIC IRQs\n");
    irq_reset(true);
    for (;;);
    // apic[APIC_LVT3] = (apic[APIC_LVT3] & ~0xff) | (AE_VECTOR & 0xff);

    // Broadcast INIT IPI to all APs
    apic[APIC_ICR_LOW] = 0x0C4500;
    x86_delayX(10000); // 10-millisecond delay loop.

    // Load ICR encoding for broadcast SIPI IP (x2)
    apic[APIC_ICR_LOW] = 0x000C4600 | AP_VECTOR;
    x86_delayX(200); // 200-microsecond delay loop.
    apic[APIC_ICR_LOW] = 0x000C4600 | AP_VECTOR;
    x86_delayX(200); // 200-microsecond delay loop.

    // Wait timer interrupt - We should have init all CPUs
    irq_disable();
    // kSYS.cpuCount_ = cpu_count + 1;
    if (cpu_count == 0)
        kprintf(KLOG_MSG, "CPU%d: %s, single CPU\n", cpu_no(), cpu.vendor);
    else
        kprintf(KLOG_MSG, "BSP is CPU%d: %s, found a count of %d CPUs\n", cpu_no(),
                cpu.vendor, cpu_count + 1);

    char *tmp = kalloc(1024);
    tmp[0] = 0;
    if (x86_FEATURES_FPU(all_cpu))
        strcat(tmp, "FPU, ");
    if (x86_FEATURES_VME(all_cpu))
        strcat(tmp, "VME, ");
    if (x86_FEATURES_PE(all_cpu))
        strcat(tmp, "PE, ");
    if (x86_FEATURES_PSE(all_cpu))
        strcat(tmp, "PSE, ");
    if (x86_FEATURES_TSC(all_cpu))
        strcat(tmp, "TSC, ");
    if (x86_FEATURES_MSR(all_cpu))
        strcat(tmp, "MSR, ");
    if (x86_FEATURES_PAE(all_cpu))
        strcat(tmp, "PAE, ");
    if (x86_FEATURES_MCE(all_cpu))
        strcat(tmp, "MCE, ");
    if (x86_FEATURES_CX8(all_cpu))
        strcat(tmp, "CX8, ");
    if (x86_FEATURES_APIC(all_cpu))
        strcat(tmp, "APIC, ");
    if (x86_FEATURES_SEP(all_cpu))
        strcat(tmp, "SEP, ");
    if (x86_FEATURES_MTRR(all_cpu))
        strcat(tmp, "MTRR, ");
    if (x86_FEATURES_PGE(all_cpu))
        strcat(tmp, "PGE, ");
    if (x86_FEATURES_MCA(all_cpu))
        strcat(tmp, "MCA, ");
    if (x86_FEATURES_CMOV(all_cpu))
        strcat(tmp, "CMOV, ");
    if (x86_FEATURES_PAT(all_cpu))
        strcat(tmp, "PAT, ");
    if (x86_FEATURES_PSE36(all_cpu))
        strcat(tmp, "PSE36, ");
    if (x86_FEATURES_PSN(all_cpu))
        strcat(tmp, "PSN, ");
    if (x86_FEATURES_CLF(all_cpu))
        strcat(tmp, "CLF, ");
    if (x86_FEATURES_DTES(all_cpu))
        strcat(tmp, "DTES, ");
    if (x86_FEATURES_ACPI(all_cpu))
        strcat(tmp, "ACPI, ");
    if (x86_FEATURES_MMX(all_cpu))
        strcat(tmp, "MMX, ");
    if (x86_FEATURES_FXSR(all_cpu))
        strcat(tmp, "FXSR, ");
    if (x86_FEATURES_SSE(all_cpu))
        strcat(tmp, "SSE, ");
    if (x86_FEATURES_SSE2(all_cpu))
        strcat(tmp, "SSE2, ");
    if (x86_FEATURES_SS(all_cpu))
        strcat(tmp, "SS, ");
    if (x86_FEATURES_HTT(all_cpu))
        strcat(tmp, "HTT, ");
    if (x86_FEATURES_TM1(all_cpu))
        strcat(tmp, "TM1, ");
    if (x86_FEATURES_IA64(all_cpu))
        strcat(tmp, "IA64, ");
    if (x86_FEATURES_PBE(all_cpu))
        strcat(tmp, "PBE, ");
    if (x86_FEATURES_SSE3(all_cpu))
        strcat(tmp, "SSE3, ");
    if (x86_FEATURES_PCLMUL(all_cpu))
        strcat(tmp, "PCLMUL, ");
    if (x86_FEATURES_DTES64(all_cpu))
        strcat(tmp, "DTES64, ");
    if (x86_FEATURES_MONITOR(all_cpu))
        strcat(tmp, "MONITOR, ");
    if (x86_FEATURES_DS_CPL(all_cpu))
        strcat(tmp, "DS_CPL, ");
    if (x86_FEATURES_VMX(all_cpu))
        strcat(tmp, "VMX, ");
    if (x86_FEATURES_SMX(all_cpu))
        strcat(tmp, "SMX, ");
    if (x86_FEATURES_EST(all_cpu))
        strcat(tmp, "EST, ");
    if (x86_FEATURES_TM2(all_cpu))
        strcat(tmp, "TM2, ");
    if (x86_FEATURES_SSSE3(all_cpu))
        strcat(tmp, "SSSE3, ");
    if (x86_FEATURES_CID(all_cpu))
        strcat(tmp, "AVX, ");
    if (x86_FEATURES_FMA(all_cpu))
        strcat(tmp, "FMA, ");
    if (x86_FEATURES_CX16(all_cpu))
        strcat(tmp, "CX16, ");
    if (x86_FEATURES_ETPRD(all_cpu))
        strcat(tmp, "ETPRD, ");
    if (x86_FEATURES_PDCM(all_cpu))
        strcat(tmp, "PDCM, ");
    if (x86_FEATURES_DCA(all_cpu))
        strcat(tmp, "DCA, ");
    if (x86_FEATURES_SSE4_1(all_cpu))
        strcat(tmp, "SSE4_1, ");
    if (x86_FEATURES_SSE4_2(all_cpu))
        strcat(tmp, "SSE4_2, ");
    if (x86_FEATURES_x2APIC(all_cpu))
        strcat(tmp, "x2APIC, ");
    if (x86_FEATURES_MOVBE(all_cpu))
        strcat(tmp, "MOVBE, ");
    if (x86_FEATURES_POPCNT(all_cpu))
        strcat(tmp, "POPCNT, ");
    if (x86_FEATURES_AES(all_cpu))
        strcat(tmp, "AES, ");
    if (x86_FEATURES_XSAVE(all_cpu))
        strcat(tmp, "XSAVE, ");
    if (x86_FEATURES_OSXSAVE(all_cpu))
        strcat(tmp, "OSXSAVE, ");
    if (x86_FEATURES_AVX(all_cpu))
        strcat(tmp, "AVX, ");

    cpu_apic();
    kprintf(KLOG_MSG, "CPUs features: %s\n", tmp);
}




void cpu_setup_core_x86()
{
    struct x86_CPU cpu;

    // Check cpu features first
    x86_cpu_features(&cpu);

    kprintf(KLOG_MSG, "Wakeup CPU%d: %s.\n", cpu_no(), cpu.vendor);
    // TODO - Prepare structure - Compute TSS
    cpu_apic();
}


#define CPU_REBOOT 0xFE

void cpu_reboot()
{
    while ((inb(0x64) & 2) != 0);
    outb(0x64, 0xF4); /* Reset */
    for (;;);
}

void cpu_shutdown()
{
    // outw(0xB004, 0x2000);
}

#include <kernel/task.h>


static void _exit()
{
    for (;;)
        task_switch(TS_ZOMBIE, -42);
}

void cpu_stack(task_t *task, size_t entry, size_t param)
{
    size_t *stack = (size_t *)task->kstack;
    task->state[5] = entry;
    task->state[3] = (size_t)task->kstack;

    stack--;
    *stack = param;
    stack--;
    *stack = (size_t)_exit;
    task->state[4] = (size_t)stack;
}




