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
#include <kernel/mods.h>
#include <kernel/bus/pci.h>
#include <string.h>

struct device_id {
    uint16_t vendor;
    uint16_t id;
    const char *name;
    void (*start)(struct PCI_device *pci, struct device_id *info);
};

void vga_start_qemu(struct PCI_device *pci, struct device_id *info);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


struct device_id __vga_ids[] = {
    { .vendor = 0x1234, .id = 0x1111, .name = "QEMU VGA", .start = vga_start_qemu },
    { .vendor = 0x80EE, .id = 0xBEEF, .name = "VirtualBox VGA", .start = vga_start_qemu },
    // { .vendor = 0x15ad, .id = 0x0405, .name = "VMWare VGA", .start = NULL },
};


// struct device_id *vga_pci_info(struct PCI_device *pci)
// {
//     unsigned i;
//     for (i = 0; i < sizeof(__vga_ids) / sizeof(struct device_id); ++i) {
//         if (__vga_ids[i].id == pci->device_id && __vga_ids[i].vendor == pci->vendor_id)
//             return &__vga_ids[i];
//     }

//     return NULL;
// }

int vga_match_pci_device(uint16_t vendor, uint32_t class, uint16_t device)
{
    unsigned i;
    for (i = 0; i < sizeof(__vga_ids) / sizeof(struct device_id); ++i) {
        if (__vga_ids[i].id == device && __vga_ids[i].vendor == vendor)
            return i;
    }

    return -1;
}

void vga_setup()
{
    struct PCI_device *pci = NULL;
    char name[48];

    int i;
    for (;;) {
        pci = pci_search(vga_match_pci_device, &i);
        if (pci == NULL)
            break;

        struct device_id *info = &__vga_ids[i];
        strcpy(name, info->name);
        info->start(pci, info);

    }
}

void vga_teardown()
{

}


EXPORT_MODULE(vga, vga_setup, vga_teardown);
