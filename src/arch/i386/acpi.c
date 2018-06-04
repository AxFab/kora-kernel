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

typedef struct acpi_rsdp acpi_rsdp_t;
typedef struct acpi_rsdt acpi_rsdt_t;
typedef struct acpi_madt acpi_madt_t;
typedef struct madt_lapic madt_lapic_t;

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

PACK(struct madt_lapic {
    uint8_t type;
    uint8_t size;
    uint8_t acpi_id;
    uint8_t apic_id;
    uint32_t flags;
});

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

acpi_rsdp_t *rsdp;

void acpi_setup()
{
    char *ptr = (char*)0xE0000;
    for (; ptr < (char*)0x100000; ptr += 16) {
        if (memcmp(ptr, "RSD PTR ", 8) != 0)
            continue;
        kprintf(KLOG_DBG, "Found ACPI at %p\n", ptr);
        break;
    }

    if (ptr >= 0x100000) {
        kprintf(KLOG_DBG, "Not found ACPI.\n");
        return;
    }

    rsdp = (acpi_rsdp_t*)ptr;

}
