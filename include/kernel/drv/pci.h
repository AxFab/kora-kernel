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
#ifndef __KERNEL_DRV_PCI_H
#define __KERNEL_DRV_PCI_H 1

#include <kernel/core.h>

struct PCI_device {
    uint8_t bus;
    uint8_t slot;
    uint8_t irq;
    uint8_t busy;
    uint16_t vendor_id;
    uint16_t device_id;
    uint32_t class_id;
    struct {
        uint32_t base;
        uint32_t size;
        uint32_t mmio;
    } bar[4];
};

struct PCI_device *PCI_search(uint16_t vendor_id, uint32_t class_id,
                              uint16_t device_id);


__stinline void PCI_wr32(struct PCI_device *pci, char no, uint16_t address,
                         uint32_t value)
{
    if (pci->bar[0].base & 1) {
        int iobase = pci->bar[0].base  & 0xFFFFFFFC;
        outl(iobase, address);
        outl(iobase + 4, value);
    } else {
        *((volatile uint32_t *)(pci->bar[0].mmio + address)) = value;
    }
}

__stinline uint32_t PCI_rd32(struct PCI_device *pci, char no,
                             uint16_t address)
{
    if (pci->bar[0].base & 1) {
        int iobase = pci->bar[0].base  & 0xFFFFFFFC;
        outl(iobase, address);
        return inl(iobase + 4);
    } else {
        return *((volatile uint32_t *)(pci->bar[0].mmio + address));
    }
}

__stinline void PCI_wr16(struct PCI_device *pci, char no, uint16_t address,
                         uint16_t value)
{
    if (pci->bar[0].base & 1) {
        int iobase = pci->bar[0].base  & 0xFFFFFFFC;
        outw(iobase, address);
        outw(iobase + 4, value);
    } else {
        *((volatile uint16_t *)(pci->bar[0].mmio + address)) = value;
    }
}

__stinline uint16_t PCI_rd16(struct PCI_device *pci, char no,
                             uint16_t address)
{
    if (pci->bar[0].base & 1) {
        int iobase = pci->bar[0].base  & 0xFFFFFFFC;
        outw(iobase, address);
        return inw(iobase + 4);
    } else {
        return *((volatile uint16_t *)(pci->bar[0].mmio + address));
    }
}

#define PCI_write(p,a,v) PCI_wr32(p,0,a,v)
#define PCI_read(p,a) PCI_rd32(p,0,a)

#define PCI_write16(p,a,v) PCI_wr16(p,0,a,v)
#define PCI_read16(p,a) PCI_rd16(p,0,a)


#endif  /* __KERNEL_DRV_PCI_H */
