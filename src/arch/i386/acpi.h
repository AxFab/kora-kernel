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
#ifndef _SRC_ACPI_H
#define _SRC_ACPI_H 1

#include <kernel/core.h>


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


#endif  /* _SRC_ACPI_H */
