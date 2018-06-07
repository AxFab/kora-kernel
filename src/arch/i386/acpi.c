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
    acpi_head_t header;
    uint32_t tables[0];
});

PACK(struct acpi_madt {
    acpi_head_t header;
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
    for (i = 0; i < n; ++i) {
        // Read each entry
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
