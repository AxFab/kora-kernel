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
#include <kernel/core.h>
#include <kernel/memory.h>

#define APIC_ID  (0x20 / 4) // Local APIC ID
#define APIC_VERS  (0x30 / 4) // Local APIC Version
#define APIC_TPR  (0x80 / 4) // Task Priority Register
#define APIC_APR  (0x90 / 4) // Arbitration Priority Register
#define APIC_PPR  (0xA0 / 4) // Processor Priority Register
#define APIC_EOI  (0xB0 / 4) // EOI Register
#define APIC_RRD  (0xC0 / 4) // Remote Read Register
#define APIC_LDR  (0xD0 / 4) // Logical Destination Register
#define APIC_DFR  (0xE0 / 4) // Destination Format Register
#define APIC_SVR  (0xF0 / 4) // Spurious Interrupt Vector Register

#define APIC_ESR  (0x280 / 4) // Error Status Register
#define APIC_ICR_LOW  (0x300 / 4)
#define APIC_ICR_HIGH  (0x310 / 4)
#define APIC_LVT_TMR (0x320 / 4) // LVT Timer
#define APIC_LVT_PERF (0x320 / 4) // LVT Perf
#define APIC_LVT_LINT0 (0x320 / 4) // LVT Int 0
#define APIC_LVT_LINT1 (0x320 / 4) // LVT Int 1
#define APIC_LVT_ERR  (0x370 / 4) // LVT Error register
#define APIC_TM_INC (0x380 / 4) // Timer initial counter
#define APIC_TM_CUC (0x390 / 4) // Timer current counter
#define APIC_TM_DIV (0x3E0 / 4) // Timer divide

void x86_delay(int);

typedef struct acpi_head acpi_head_t;
typedef struct acpi_rsdp acpi_rsdp_t;
typedef struct acpi_rsdt acpi_rsdt_t;
typedef struct acpi_madt acpi_madt_t;
typedef struct acpi_gas acpi_gas_t;
typedef struct acpi_fadt acpi_fadt_t;

typedef struct madt_lapic madt_lapic_t;
typedef struct madt_ioapic madt_ioapic_t;
typedef struct madt_override madt_override_t;

typedef struct acpi_hpet acpi_hpet_t;

PACK(struct acpi_rsdp {
    char signature[8];
    uint8_t checksum;
    char oem[6];
    uint8_t revision;
    uint32_t rsdt;
    uint32_t length;
    uint64_t xsdt;
});

PACK(struct acpi_head {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem[6];
    char oem_tbl[8];
    uint32_t oem_rev;
    uint32_t creator_id;
    uint32_t creator_revision;
});

PACK(struct acpi_rsdt {
    acpi_head_t header;
    uint32_t tables[0];
});

PACK(struct acpi_madt {
    acpi_head_t header;
    uint32_t local_apic;
    uint32_t flags;
    uint8_t records[0];
});

PACK(struct acpi_gas {
    uint8_t address_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t base;
});

PACK(struct acpi_fadt {
    acpi_head_t header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;

    uint8_t reserved;
    uint8_t profile;
    uint16_t sci_irq;
    uint32_t smi_command_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_ctrl;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_ctrl_block;
    uint32_t pm1b_ctrl_block;
    uint32_t pm2_ctrl_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_ctrl_length;
    uint8_t pm2_ctrl_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;
    uint8_t cstate_ctrl;
    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;

    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;

    uint16_t boot_flags;
    uint8_t reserved2;
    uint32_t flags;

    acpi_gas_t reset_register;
    uint8_t reset_command;
    uint8_t reserved3[3];

    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;

    acpi_gas_t x_pm1a_event_block;
    acpi_gas_t x_pm1b_event_block;
    acpi_gas_t x_pm1a_ctrl_block;
    acpi_gas_t x_pm1b_ctrl_block;
    acpi_gas_t x_pm2_ctrl_block;
    acpi_gas_t x_pm_timer_block;
    acpi_gas_t x_gpe0_block;
    acpi_gas_t x_gpe1_block;
});

PACK(struct madt_lapic {
    uint8_t type;
    uint8_t size;
    uint8_t acpi_id;
    uint8_t apic_id;
    uint32_t flags;
});

PACK(struct madt_ioapic {
    uint8_t type;
    uint8_t size;
    uint8_t apic_id;
    uint8_t reserved;
    uint32_t base;
    uint32_t gsi;
});

PACK(struct madt_override {
    uint8_t type;
    uint8_t size;
    uint8_t bus;
    uint8_t irq;
    uint32_t gsi;
    uint16_t flags;
});

PACK(struct acpi_hpet {
    acpi_head_t header;
    uint16_t cap;
    uint16_t vendor;
    acpi_gas_t base;
    uint8_t hpet_seq;
    uint16_t min_periodic_count;
    uint8_t page_protect;
});

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

size_t *cpu_kstacks;

static void lapic_register(sys_info_t *sysinfo, madt_lapic_t *info)
{
    int no = info->acpi_id;
    kprintf(-1, " - lapic: acpi:%d apic:%d, flags %x\n",
            no, info->apic_id, info->flags);

    cpu_info_t *cpu = &sysinfo->cpu_table[no];

    cpu->id = info->apic_id;
    if (no == 0)
        cpu->stack = (void *)0x4000;
    else
        cpu->stack = kmap(PAGE_SIZE, NULL, 0, VMA_STACK | VM_RW | VM_RESOLVE);
    cpu_kstacks[no] = (size_t)cpu->stack + 0xFFC;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


#define IOAPIC_ID  0x00
#define IOAPIC_VERSION  0x01
#define IOAPIC_ARBITRATION  0x02
#define IOAPIC_IRQ_TABLE  0x10

static uint32_t ioapic_read(int no, uint8_t *ioapic, uint32_t index)
{
    volatile uint32_t *select = (uint32_t *)ioapic;
    volatile uint32_t *data = (uint32_t *)(ioapic + 16);
    select[0] = index;
    return data[0];
}

static void ioapic_write(int no, uint8_t *ioapic, uint32_t index,
                         uint32_t value)
{
    volatile uint32_t *select = (uint32_t *)ioapic;
    volatile uint32_t *data = (uint32_t *)(ioapic + 16);
    select[0] = index;
    data[0] = value;
}

static void ioapic_write_irq(int no, uint8_t *ioapic, int irq, uint64_t flags)
{
    uint64_t value = (0x20 + irq) | flags;
    uint32_t reg = (irq << 1) + 0x10;
    ioapic_write(no, ioapic, reg, value & 0xffffffff);
    ioapic_write(no, ioapic, reg + 1, (value >> 32) & 0xffffffff);
}

static void ioapic_register(sys_info_t *sysinfo, madt_ioapic_t *info)
{
    uint8_t *ioapic = kmap(PAGE_SIZE, NULL, info->base, VM_RW | VMA_PHYS | VM_UNCACHABLE);

    int irq_count = (ioapic_read(info->apic_id, ioapic,
                                 IOAPIC_VERSION) >> 16) & 0xFF;

    kprintf(-1, " - ioapic: apic:%d, base:%x, GSIs:%d, IRQs %d\n",
            info->apic_id, info->base, info->gsi, irq_count);

    int i;
    uint64_t flags;
    for (i = 0; i < irq_count; ++i) {
        flags = (i == 0 || i == 23) ? (0xFFULL << 56) | 0x800 : 0x8000;
        ioapic_write_irq(info->apic_id, ioapic, i, flags);
    }

    // kunmap(ioapic, PAGE_SIZE);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static void apic_override_register(sys_info_t *sysinfo, madt_override_t *info)
{
    kprintf(-1, " - override IRQ %d on bus %d, GSIs %d, %s %s.\n",
            info->irq, info->bus, info->gsi,
            (info->flags & 8) ? "level" : "edge",
            (info->flags & 2) ? "low" : "high");

}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


static int acpi_checksum(acpi_head_t *header)
{
    uint8_t *ptr = (uint8_t *)header;
    uint8_t *end = ptr + header->length;
    uint8_t sum = 0;
    while (ptr < end)
        sum += *(ptr++);
    return sum;
}

static void acpi_fadt_setup(sys_info_t *sysinfo, acpi_fadt_t *fadt)
{
    kprintf(-1, "FADT Table at %p\n", fadt);
}


static void acpi_madt_setup(sys_info_t *sysinfo, acpi_madt_t *madt)
{
    kprintf(-1, "MADT Table at %p\n", madt);
    sysinfo->arch->apic = madt->local_apic;
    kprintf(-1, "Local APIC at %p\n", sysinfo->arch->apic);

    uint8_t *ptr = madt->records;
    uint8_t *end = ptr + madt->header.length - offsetof(acpi_madt_t, records);

    // First count the number of `madt_lapic_t` to count CPUs
    int cpus = 0;
    while (ptr < end) {
        madt_lapic_t *lapic = (madt_lapic_t *)ptr;
        switch (lapic->type) {
        case 0:
            cpus++;
            break;
        }
        ptr += lapic->size;
    }

    // Alloc CPUs table
    sysinfo->cpu_count = cpus;
    sysinfo->cpu_table = kalloc(cpus * sizeof(cpu_info_t));
    sysinfo->cpu_table->arch = kalloc(cpus * sizeof(x86_cpu_t));
    for (int i = 1; i < cpus; ++i)
        sysinfo->cpu_table[i].arch = &sysinfo->cpu_table[0].arch[i];
    cpu_kstacks = kalloc(cpus * sizeof(size_t));

    // Browser entries
    ptr = madt->records;
    while (ptr < end) {
        madt_lapic_t *lapic = (madt_lapic_t *)ptr;
        switch (lapic->type) {
        case 0:
            lapic_register(sysinfo, (madt_lapic_t *)lapic);
            break;
        case 1:
            ioapic_register(sysinfo, (madt_ioapic_t *)lapic);
            break;
        case 2:
            apic_override_register(sysinfo, (madt_override_t *)lapic);
            break;
        case 4:
            // SIZE 6 -> ff 00 00 01
            break;
        }
        ptr += lapic->size;
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static void acpi_hpet_setup(sys_info_t *sysinfo, acpi_hpet_t *hpet)
{
    kprintf(-1, "HPET Table at %p\n", hpet);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Initialize ACPI - Thermal monitor and clock ctrl */
void acpi_setup(sys_info_t *sysinfo)
{
    // Search for 'RSD PTR ' in hardware space, aligned on 16B.
    char *ptr = (char *)0xE0000;
    for (; ptr < (char *)0x100000; ptr += 16) {
        if (memcmp(ptr, "RSD PTR ", 8) != 0)
            continue;
        kprintf(-1, "Found ACPI at %p\n", ptr);
        break;
    }

    if (ptr >= (char *)0x100000) {
        kprintf(-1, "Not found ACPI.\n");
        sysinfo->arch->acpi = NULL;
        sysinfo->cpu_count = 1;
        sysinfo->cpu_table = kalloc(sizeof(cpu_info_t));
        sysinfo->cpu_table->arch = kalloc(sizeof(x86_cpu_t));
        cpu_kstacks = kalloc(sizeof(size_t));
        sysinfo->cpu_table->stack = (void *)0x4000;
        cpu_kstacks = (void *)0x4000;
        return;
    }


    // Search RSDP and RSDT
    sysinfo->arch->acpi = ptr;
    acpi_rsdp_t *rsdp = (void*)ptr;
    size_t phoff = ALIGN_DW(rsdp->rsdt, PAGE_SIZE);
    void *map = kmap(PAGE_SIZE, NULL, phoff, VMA_PHYS | VM_UNCACHABLE);
    acpi_rsdt_t *rsdt = ADDR_OFF(map, (rsdp->rsdt & (PAGE_SIZE - 1)));
    if (acpi_checksum(&rsdt->header) != 0) {
        kprintf(-1, "Invalid ACPI RSDT checksum.\n");
        return;
    }

    // Browse table
    int i, n = (rsdt->header.length - sizeof(acpi_head_t)) / sizeof(uint32_t);
    for (i = 0; i < n; ++i) {
        acpi_head_t *rstb = ADDR_OFF(map, (rsdt->tables[i] & (PAGE_SIZE - 1)));
        if (acpi_checksum(rstb) != 0) {
            kprintf(-1, "Invalid ACPI RSDT entry checksum.\n");
            continue;
        }

        switch (*((uint32_t *)rstb)) {
        case 0x50434146: // FACP
            acpi_fadt_setup(sysinfo, (acpi_fadt_t *)rstb);
            break;
        case 0x43495041: // APIC
            acpi_madt_setup(sysinfo, (acpi_madt_t *)rstb);
            break;
        case 0x54455048: // HPET
            acpi_hpet_setup(sysinfo, (acpi_hpet_t *)rstb);
            break;
        default:
            kprintf(-1, "ACPI RSDT entry unknown '%08x'.\n", *((uint32_t *)rstb));
            break;
        }
    }

}

volatile uint32_t *apic_ptr = NULL;
void ap_start(void);

void apic_setup(sys_info_t *sysinfo)
{
    if (sysinfo->arch->apic == 0)
        return;

    // Should always be 0xFEE00000
    kprintf(-1, "Local APIC at %x\n", sysinfo->arch->apic);
    apic_ptr = kmap(PAGE_SIZE, NULL, sysinfo->arch->apic, VM_RW | VMA_PHYS | VM_UNCACHABLE);

    kprintf(-1, "Send INIT IPI to all APs\n");
    apic_ptr[APIC_ICR_LOW] = 0x0C4500;
    x86_delay(100/*00*/); // 10-millisecond delay loop.

    kprintf(-1, "Broadcast SIPI IPI to APs (x2)\n");
    uint32_t sipi_ipi = 0x000C4600 | ((size_t)ap_start >> 12);

    apic_ptr[APIC_ICR_LOW] = sipi_ipi;
    x86_delay(2); // 200-microsecond delay loop.

    apic_ptr[APIC_ICR_LOW] = sipi_ipi;
    x86_delay(500); // 50-millisecond delay loop.
}
