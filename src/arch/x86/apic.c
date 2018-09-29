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
 *
 *      Intel x86 CPU features.
 */
#include <kernel/core.h>
#include <kernel/cpu.h>

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
#define APIC_ICR_LOW  (0x300 / 4) // Interrupt Command Register
#define APIC_ICR_HIGH  (0x310 / 4) // Interrupt Command Register

#define APIC_LVT_TIMER  (0x320 / 4) // LVT Timer Register
#define APIC_LVT_PERF  (0x340 / 4) // LVT Performance Register
#define APIC_LVT_LINT0  (0x350 / 4) // LVT Local INT0 Register
#define APIC_LVT_LINT1  (0x360 / 4) // LVT Local INT1 Register
#define APIC_LVT_ERROR  (0x370 / 4) // LVT Error Register


#define APIC_ICTR  (0x380 / 4) // Initial Count Time-Register
#define APIC_CCTR  (0x390 / 4) // Current Count Time-Register
#define APIC_DCTR  (0x3E0 / 4) // Divide Configuration Time-Register

extern volatile uint32_t *apic;


#define APIC_DISABLE_ISRS  (1 << 16)

#define APIC_DM_NMI (4 << 8)  // Delivery Mode NMI

void cpu_apic_timer_init()
{
    // Set up local IRQ (isrs)
    apic[APIC_SVR] = 0x64; // IRQ20 - Enable the APIC
    apic[APIC_LVT_TIMER] = 0x62; // | (1 << 17); // Enable local timer - IRQ18

    // Initialize local APIC to a well known state
    apic[APIC_DFR] = 0xFFFFFFFF;
    uint32_t ldr = (apic[APIC_LDR] & 0xFFFFFF) | 1;
    apic[APIC_LDR] = ldr;

    apic[APIC_LVT_TIMER]  = APIC_DISABLE_ISRS;
    apic[APIC_LVT_PERF]  = APIC_DM_NMI;
    // apic[APIC_LVT_LINT0] = APIC_DISABLE_ISRS;
    apic[APIC_LVT_LINT1] = APIC_DISABLE_ISRS;
    apic[APIC_TPR] = 0;

    // Enable APIC
    apic[APIC_SVR] = 0x64 | (1 << 8);// | (1 << 11);
    apic[APIC_LVT_TIMER] = 0x62; // | (1 << 17);
    apic[APIC_DCTR] = 0x3; // Divide by 16
    apic[APIC_ICTR] = 0xFFFFFFFF;
    x86_delayX(10000); // 10-millisecond delay loop.
    apic[APIC_LVT_TIMER] = APIC_DISABLE_ISRS;

    uint32_t ticksIn10ms = 0xFFFFFFFF - apic[APIC_ICTR];
    apic[APIC_LVT_TIMER] = 0x62 | (1 << 17); // IRQ18
    apic[APIC_DCTR] = 0x3; // Divide by 16
    apic[APIC_ICTR] = ticksIn10ms;
    kprintf(KLOG_MSG, "Set cpu period of 10ms. %d.\n", ticksIn10ms);
}

typedef struct acpi_rsdp acpi_rsdp_t;
typedef struct acpi_rsdt acpi_rsdt_t;
typedef struct acpi_madt acpi_madt_t;

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
    struct acpi_head header;
    uint32_t tables[0];
});
PACK(struct acpi_madt {
    struct acpi_head header;
    uint32_t local_apic;
    uint32_t flags;
    uint8_t records[0];
});

typedef struct madt_lapic madt_lapic_t;

PACK(struct madt_lapic {
    uint8_t type;
    uint8_t size;
    uint8_t acpi_id;
    uint8_t apic_id;
    uint32_t flags;
});

void cpu_apic()
{
    char *ptr = (char *)0xE0000;
    for (; ptr < (char *)0x100000; ptr += 16) {
        if (memcmp(ptr, "RSD PTR ", 8) != 0)
            continue;
        kprintf(-1, "Found ACPI at %p\n", ptr);
        break;
    }

    if (ptr >= 0x100000)
        kprintf(-1, "No found ACPI.\n");

    else {

    }

    acpi_rsdp_t *rsdp = (acpi_rsdp_t *)ptr;
    kdump(rsdp, 0x20);

    void *tbl = kmap(PAGE_SIZE, NULL, ALIGN_DW(rsdp->rsdt, PAGE_SIZE), VMA_PHYSIQ);
    acpi_rsdt_t *rsdt = (acpi_rsdt_t *)((size_t)tbl | (rsdp->rsdt &
                                        (PAGE_SIZE - 1)));
    kprintf(-1, "ACPI rsdt:%p\n");
    kdump(rsdt, 0x30);
    for (int i = 0; i < (rsdt->header.length - 0x24) / 4; ++i) {
        kprintf(-1, "ACPI table.%d:%p\n", i, rsdt->tables[i]);
        acpi_rsdt_t *rtbl = (acpi_rsdt_t *)((size_t)tbl | (rsdt->tables[i] &
                                            (PAGE_SIZE - 1)));
        kdump(rtbl, rtbl->header.length);
    }

    acpi_madt_t *madt = (acpi_madt_t *)((size_t)tbl | (rsdt->tables[1] &
                                        (PAGE_SIZE - 1)));
    kprintf(-1, "LOCAL APIC at: 0x%x\n", madt->local_apic);




    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    kprintf(-1, "APIC ID %u\n", apic[APIC_ID]);
    kprintf(-1, "APIC Version %u\n", apic[APIC_VERS]);
    // apic[APIC_TPR] = 0;
    // apic[APIC_SVR] = 0x64 | (1 << 8) | (1 << 11);
    // apic[APIC_DFR] = 0xFFFFFFFF;
    // apic[APIC_LDR] = 0xFF << 24;
    // apic[APIC_EOI] = 0;

    // apic[APIC_ICTR] = 0x2;
    // apic[APIC_DCTR] = 0x3; // Divide by 16

    // // cpu_apic_timer_init();
    // apic[APIC_LVT_LINT0] = 0x60; // 16
    // apic[APIC_LVT_LINT1] = 0x61; // 17
    // apic[APIC_LVT_TIMER] = 0x62 | (1 << 17); // 18
    // // apic[APIC_LVT_ERROR] = 0x63; // 19
    // // apic[APIC_CCTR] = 50;

    // // apic[APIC_DCTR] = 0x3; // Divide by 16
    // // apic[APIC_ICTR] = 0xFFFFFFFF;


    // apic[APIC_EOI] = 0;
    // apic[APIC_EOI] = 0;
    // apic[APIC_EOI] = 0;
    // apic[APIC_EOI] = 0;
    // apic[APIC_EOI] = 0;
    // apic[APIC_EOI] = 0;
    // apic[APIC_EOI] = 0;
    // apic[APIC_EOI] = 0;
    // apic[APIC_EOI] = 0;
    // apic[APIC_EOI] = 0;
    // Set the Spourious Interrupt Vector Register bit 8 to start receiving interrupts
}

