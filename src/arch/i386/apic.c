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
#include "acpi.h"
#include "apic.h"

size_t apic_mmio;
volatile uint32_t *apic_regs = NULL;


cpu_x86_t *cpu_table = NULL;

void lapic_register(madt_lapic_t *info)
{
    kprintf(KLOG_ERR, " - lapic: acpi:%d apic:%d, flags %x\n",
        info->acpi_id, info->apic_id, info->flags);
    cpu_table[info->acpi_id].id = info->apic_id;
    if (info->acpi_id == 0)
        cpu_table[info->acpi_id].stack = 0x4000;
    else
        cpu_table[info->acpi_id].stack = (size_t)kmap(PAGE_SIZE, NULL, 0, VMA_STACK_RW | VMA_RESOLVE);
}

void lapic_setup(int cpus)
{
    cpu_table = (cpu_x86_t*)kalloc(sizeof(cpu_x86_t) * cpus);
    struct kCpu* cpu0 = kSYS.cpus;
    kSYS.cpus = (struct kCpu*)kalloc(sizeof(struct kCpu) * cpus);
    kSYS.cpus[0] = *cpu0;
    // TODO - kCPU !
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define IOAPIC_ID  0x00
#define IOAPIC_VERSION  0x01
#define IOAPIC_ARBITRATION  0x02
#define IOAPIC_IRQ_TABLE  0x10

static uint32_t ioapic_read(int no, uint8_t *ioapic, uint32_t index)
{
    volatile uint32_t *select = (uint32_t*)ioapic;
    volatile uint32_t *data = (uint32_t*)(ioapic + 16);
    select[0] = index;
    return data[0];
}

static void ioapic_write(int no, uint8_t *ioapic, uint32_t index, uint32_t value)
{
    volatile uint32_t *select = (uint32_t*)ioapic;
    volatile uint32_t *data = (uint32_t*)(ioapic + 16);
    select[0] = index;
    data[0] = value;
}

static void ioapic_write_irq(int no, uint8_t *ioapic, int irq)
{
    uint64_t value = 0x20 + irq;
    uint32_t reg = (irq << 1) + 0x10;
    ioapic_write(no, ioapic, reg, value & 0xffffffff);
    ioapic_write(no, ioapic, reg + 1, (value >> 32) & 0xffffffff);
}

static uint64_t ioapic_read_irq(int no, uint8_t *ioapic, int irq)
{
    uint64_t value = 0;
    uint32_t reg = (irq << 1) + 0x10;
    value |= ioapic_read(no, ioapic, reg);
    value |= (uint64_t)ioapic_read(no, ioapic, reg + 1) << 32;
    return value;
}

void ioapic_register(madt_ioapic_t *info)
{
    uint8_t *ioapic = kmap(PAGE_SIZE, NULL, info->base, VMA_PHYSIQ);

    int irq_count = (ioapic_read(info->apic_id, ioapic, IOAPIC_VERSION) >> 16) & 0xFF;

    kprintf(KLOG_ERR, " - ioapic: apic:%d, base:%x, GSIs:%d, IRQs %d\n",
        info->apic_id, info->base, info->gsi, irq_count);

    int i;
    for (i = 0; i < irq_count; ++i) {
        ioapic_write_irq(info->apic_id, ioapic, i);
    }

    // kunmap(ioapic, PAGE_SIZE);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void apic_override_register(madt_override_t *info)
{
    kprintf(KLOG_ERR, " - override IRQ %d on bus %d, GSIs %d, %s %s.\n",
        info->irq, info->bus, info->gsi,
        info->flags & 8 ? "level" : "edge",
        info->flags & 2 ? "low" : "high");

}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void ap_start();


void apic_init()
{
    apic_regs[APIC_TPR] = 0;
    apic_regs[APIC_SVR] = 0x1FF;
    apic_regs[APIC_DFR] = 0xFFFFFFFF;
    apic_regs[APIC_LDR] = 0xFF000000;
    apic_regs[APIC_EOI] = 0;

    kprintf(-1, "CPU%d - at %p \n", cpu_no(), &kCPU);
    kprintf(-1, "CPU%d IRQ sem%d \n", cpu_no(), kCPU.irq_semaphore);
}

void apic_setup()
{
    if (apic_regs != NULL || apic_mmio == NULL)
        return;

    kprintf(KLOG_ERR, "Local APIC at %x\n", apic_mmio);
    apic_regs = kmap(PAGE_SIZE, NULL, apic_mmio, VMA_PHYSIQ);

    kprintf(KLOG_DBG, "Send INIT IPI to all APs\n");
    // INIT IPI to all APs
    apic_regs[APIC_ICR_LOW] = 0x0C4500;
    x86_delay(100/*00*/); // 10-millisecond delay loop.

    kprintf(KLOG_DBG, "Broadcast SIPI IP to APs (x2)\n");
    // Load ICR encoding for broadcast SIPI IP (x2)
    apic_regs[APIC_ICR_LOW] = 0x000C4600 | ((size_t)ap_start >> 12);
    x86_delay(2); // 200-microsecond delay loop.
    apic_regs[APIC_ICR_LOW] = 0x000C4600 | ((size_t)ap_start >> 12);
    x86_delay(2); // 200-microsecond delay loop.

    apic_init();

    // kunmap(apic_mmio, PAGE_SIZE);
}


void ap_setup()
{
    kprintf(KLOG_DBG, "CPU%d is awake !\n", cpu_no());
    cpuid_setup();
    apic_init();
}
