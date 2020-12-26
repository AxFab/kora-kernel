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
#include <kernel/mods.h>
#include <kernel/bus/pci.h>
#include <kernel/arch.h>

#define AC97_VENDOR_ID 0x8086
#define AC97_DEVICE_ID 0x2415
#define AC97_NAME "ICH AC97"


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void ac97_startup(struct PCI_device *pci)
{
    // IO region #0: d100..d200
    // IO region #1: d200..d240

}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ac97_match_pci_device(uint16_t vendor, uint32_t class, uint16_t device)
{
    if (vendor != AC97_VENDOR_ID || device != AC97_DEVICE_ID)
        return -1;
    return 0;
}


void ac97_setup()
{
    struct PCI_device *pci = NULL;

    for (;;) {
        pci = PCI_search2(ac97_match_pci_device);
        if (pci == NULL)
            break;
        // kprintf(0, "Found %s (PCI.%02d.%02d)\n", AC97_NAME, pci->bus, pci->slot);
        ac97_startup(pci);
    }
}

void ac97_teardown()
{
}


EXPORT_MODULE(ac97, ac97_setup, ac97_teardown);

