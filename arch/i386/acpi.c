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
#include <string.h>
#include <kora/mcrs.h>
#include "acpi.h"
#include "apic.h"

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

acpi_rsdp_t *rsdp;
acpi_rsdt_t *rsdt;
extern size_t hpet_mmio;

static int acpi_checksum(acpi_head_t *header)
{
    uint8_t *ptr = (uint8_t *)header;
    uint8_t *end = ptr + header->length;
    uint8_t sum = 0;
    while (ptr < end)
        sum += *(ptr++);
    return sum;
}

void acpi_fadt_setup(acpi_fadt_t *fadt)
{
    kprintf(KL_DBG, "FADT Table at %p\n", fadt);
}

void acpi_madt_setup(acpi_madt_t *madt)
{
    kprintf(KL_DBG, "MADT Table at %p\n", madt);
    apic_mmio = madt->local_apic;
    kprintf(KL_DBG, "Local APIC at %p\n", apic_mmio);

    int cpus = 0;
    uint8_t *ptr = madt->records;
    uint8_t *end = ptr + madt->header.length - offsetof(acpi_madt_t, records);

    while (ptr < end) {
        madt_lapic_t *lapic = (madt_lapic_t *)ptr;
        switch (lapic->type) {
        case 0:
            cpus++;
            break;
        }
        ptr += lapic->size;
    }

    lapic_setup(cpus);
    ptr = madt->records;
    while (ptr < end) {
        madt_lapic_t *lapic = (madt_lapic_t *)ptr;
        switch (lapic->type) {
        case 0:
            lapic_register(lapic);
            break;
        case 1:
            ioapic_register((madt_ioapic_t *)lapic);
            break;
        case 2:
            apic_override_register((madt_override_t *)lapic);
            break;
        case 4:
            // SIZE 6 -> ff 00 00 01
            break;
        }
        ptr += lapic->size;
    }
}


void acpi_setup()
{
    // Search for 'RSD PTR ' in hardware space, aligned on 16B.
    char *ptr = (char *)0xE0000;
    for (; ptr < (char *)0x100000; ptr += 16) {
        if (memcmp(ptr, "RSD PTR ", 8) != 0)
            continue;
        kprintf(KL_DBG, "Found ACPI at %p\n", ptr);
        break;
    }

    if (ptr >= (char *)0x100000) {
        kprintf(KL_DBG, "Not found ACPI.\n");
        return;
    }

    // Search RSDP and RSDT
    rsdp = (acpi_rsdp_t *)ptr;
    void *rsdt_pg = kmap(PAGE_SIZE, NULL, ALIGN_DW(rsdp->rsdt, PAGE_SIZE),
                         VM_PHYSIQ | VM_UNCACHABLE);
    rsdt = (acpi_rsdt_t *)((size_t)rsdt_pg | (rsdp->rsdt & (PAGE_SIZE - 1)));
    if (acpi_checksum(&rsdt->header) != 0) {
        kprintf(KL_ERR, "Invalid ACPI RSDT checksum.\n");
        return;
    }

    int i, n = (rsdt->header.length - sizeof(acpi_head_t)) / sizeof(uint32_t);
    // char C[5] = { 0 };
    for (i = 0; i < n; ++i) {
        // Read each entry
        // void *rstb_pg = kmap(PAGE_SIZE, NULL, ALIGN_DW(rsdt->tables[i], PAGE_SIZE), VMA_PHYSIQ);
        void *rstb_pg = rsdt_pg;
        acpi_head_t *rstb = (acpi_head_t *)((size_t)rstb_pg | (rsdt->tables[i] &
                                            (PAGE_SIZE - 1)));

        // memcpy(C, rstb->signature, 4);
        // kprintf(KL_ERR, "ACPI RSDT entry: %s - %x.\n", C, rstb->length);
        if (acpi_checksum(rstb) != 0) {
            kprintf(KL_ERR, "Invalid ACPI RSDT entry checksum.\n");
            continue;
        }

        switch (*((uint32_t *)rstb)) {
        case 0x50434146: // FACP
            acpi_fadt_setup((acpi_fadt_t *)rstb);
            break;
        case 0x43495041: // APIC
            acpi_madt_setup((acpi_madt_t *)rstb);
            break;
        case 0x54455048: // HPET
            hpet_mmio = ((acpi_hpet_t *)rstb)->base.base;
            break;
        default:
            kprintf(KL_ERR, "ACPI RSDT entry unknown.\n");
            break;
        }
    }
}

acpi_head_t *acpi_scan(const char *name, int idx)
{
    int i, j = 0;
    int n = (rsdt->header.length - sizeof(acpi_head_t)) / sizeof(uint32_t);
    for (i = 0; i < n; ++i) {
        acpi_head_t *head = (acpi_head_t *)rsdt->tables[i];
        if (memcpy(head->signature, name, 4) != 0)
            continue;
        if (j++ == idx)
            return head;
    }
    return NULL;
}
