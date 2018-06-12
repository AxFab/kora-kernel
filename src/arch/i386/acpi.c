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
#include <string.h>

typedef struct acpi_head acpi_head_t;
typedef struct acpi_rsdp acpi_rsdp_t;
typedef struct acpi_rsdt acpi_rsdt_t;
typedef struct acpi_madt acpi_madt_t;
typedef struct acpi_gas acpi_gas_t;
typedef struct acpi_fadt acpi_fadt_t;
typedef struct acpi_hpet acpi_hpet_t;

typedef struct madt_entry madt_entry_t;


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

PACK(struct madt_entry {
    uint8_t type;
    uint8_t size;
    union {
        struct {
            uint8_t acpi_id;
            uint8_t apic_id;
        };
        struct {
            uint8_t io_apic_id;
            uint8_t io_reserved;
        };
        struct {
            uint8_t bus;
            uint8_t irq;
        };
    };
    union {
        struct {
            uint32_t l_flags;
            uint32_t l_reserved;
        };
        struct {
            uint32_t base;
            uint32_t gsi;
        };
        struct {
            uint32_t ogsi;
            uint16_t flags;
            uint16_t ov_reserved;
        };
    };
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


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

acpi_rsdp_t *rsdp;
acpi_rsdt_t *rsdt;

static int acpi_checksum(acpi_head_t *header)
{
    uint8_t *ptr = (uint8_t*)header;
    uint8_t *end = ptr + header->length;
    uint8_t sum = 0;
    while (ptr < end)
        sum += *(ptr++);
    return sum;
}


void acpi_setup()
{
    // Search for 'RSD PTR ' in hardware space, aligned on 16B.
    char *ptr = (char*)0xE0000;
    for (; ptr < (char*)0x100000; ptr += 16) {
        if (memcmp(ptr, "RSD PTR ", 8) != 0)
            continue;
        kprintf(KLOG_DBG, "Found ACPI at %p\n", ptr);
        break;
    }

    if (ptr >= (char*)0x100000) {
        kprintf(KLOG_DBG, "Not found ACPI.\n");
        return;
    }

    // Search RSDP and RSDT
    rsdp = (acpi_rsdp_t*)ptr;
    // TODO - RSDP have checksum
    void *rsdt_pg = kmap(PAGE_SIZE, NULL, ALIGN_DW(rsdp->rsdt, PAGE_SIZE), VMA_PHYSIQ);
    rsdt = (acpi_rsdt_t*)((size_t)rsdt_pg | (rsdp->rsdt & (PAGE_SIZE - 1)));
    if (acpi_checksum(&rsdt->header) != 0) {
        kprintf(KLOG_ERR, "Invalid ACPI RSDT checksum.\n");
        return;
    }

    int i, n = (rsdt->header.length - sizeof(acpi_head_t)) / sizeof(uint32_t);
    char C[5] = { 0 };
    for (i = 0; i < n; ++i) {
        // Read each entry
        // void *rstb_pg = kmap(PAGE_SIZE, NULL, ALIGN_DW(rsdt->tables[i], PAGE_SIZE), VMA_PHYSIQ);
        void *rstb_pg = rsdt_pg;
        acpi_head_t *rstb = (acpi_head_t*)((size_t)rstb_pg | (rsdt->tables[i] & (PAGE_SIZE - 1)));

        memcpy(C, rstb->signature, 4);
        kprintf(KLOG_ERR, "ACPI RSDT entry: %s - %x.\n", C, rstb->length);
        if (acpi_checksum(&rsdt->header) != 0) {
            kprintf(KLOG_ERR, "Invalid ACPI RSDT entry checksum.\n");
            continue;
        }

        switch (*((uint32_t*)rstb)) {
        case 0x50434146: { // FACP
                acpi_fadt_t *fadt = (acpi_fadt_t*)rstb;
                // kdump(rstb, rstb->length);
            }
            break;
        case 0x43495041: { // APIC
                acpi_madt_t *madt = (acpi_madt_t*)rstb;
                kprintf(KLOG_ERR, "Local APIC at %x\n", madt->local_apic);
                // kdump(rstb, rstb->length);
                uint8_t *ptr = madt->records;
                uint8_t *end = ptr + rstb->length - offsetof(acpi_madt_t, records);
                while (ptr < end) {
                    madt_entry_t *en = (madt_entry_t*)ptr;
                    switch (en->type) {
                    case 0:
                        kprintf(KLOG_ERR, " - lapic: acpi:%d apic:%d\n", en->acpi_id, en->apic_id);
                        break;
                    case 1:
                        kprintf(KLOG_ERR, " - ioapic: apic:%d, base:%x, gsi:%d\n", en->io_apic_id, en->base, en->gsi);
                        break;
                    case 2:
                        kprintf(KLOG_ERR, " - override IRQ %d on bus %d, GSI %d, %s %s.\n",
                            en->irq, en->bus, en->ogsi, en->flags & 8 ? "level" : "edge", en->flags & 2 ? "low" : "high");
                        break;
                    case 4:
                        // SIZE 6 -> ff 00 00 01
                        break;
                    }
                    ptr += en->size;
                }
            }
            break;
        case 0x54455048: { // HPET
                acpi_hpet_t *hpet = (acpi_hpet_t*)rstb;
                kprintf(KLOG_ERR, "HPET mmio map physique %x\n", (size_t)hpet->base.base);
                // kdump(rstb, rstb->length);
            }
            break;
        default:
            kprintf(KLOG_ERR, "ACPI RSDT entry unknown.\n");
            break;
        }

        // kdump(rstb, rstb->length);

        // kunmap(rstb_pg, PAGE_SIZE);
        // char signature[4];
        // uint32_t length;
        // uint8_t revision;
        // uint8_t checksum;
        // char oem[6];
        // char oem_tbl[8];
        // uint32_t oem_rev;
        // uint32_t creator_id;
        // uint32_t creator_revision;
    }
}

acpi_head_t *acpi_scan(CSTR name, int idx)
{
    int i, j = 0;
    int n = (rsdt->header.length - sizeof(acpi_head_t)) / sizeof(uint32_t);
    for (i = 0; i < n; ++i) {
        acpi_head_t *head = (acpi_head_t*)rsdt->tables[i];
        if (memcpy(head->signature, name, 4) != 0)
            continue;
        if (j++ == idx) {
            return head;
        }
    }
    return NULL;
}
