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


#define PCI_VENDOR_ID            0x00 // 2
#define PCI_DEVICE_ID            0x02 // 2
#define PCI_COMMAND              0x04 // 2
#define PCI_STATUS               0x06 // 2
#define PCI_REVISION_ID          0x08 // 1

#define PCI_PROG_IF              0x09 // 1
#define PCI_SUBCLASS             0x0a // 1
#define PCI_CLASS                0x0b // 1
#define PCI_CACHE_LINE_SIZE      0x0c // 1
#define PCI_LATENCY_TIMER        0x0d // 1
#define PCI_HEADER_TYPE          0x0e // 1
#define PCI_BIST                 0x0f // 1
#define PCI_BAR0                 0x10 // 4
#define PCI_BAR1                 0x14 // 4
#define PCI_BAR2                 0x18 // 4
#define PCI_BAR3                 0x1C // 4
#define PCI_BAR4                 0x20 // 4
#define PCI_BAR5                 0x24 // 4

#define PCI_INTERRUPT_LINE       0x3C // 1

#define PCI_SECONDARY_BUS        0x19 // 1

#define PCI_HEADER_TYPE_DEVICE  0
#define PCI_HEADER_TYPE_BRIDGE  1
#define PCI_HEADER_TYPE_CARDBUS 2

#define PCI_TYPE_BRIDGE 0x0604
#define PCI_TYPE_SATA   0x0106

#define PCI_ADDRESS_PORT 0xCF8
#define PCI_VALUE_PORT   0xCFC

#define PCI_NONE 0xFFFF



typedef int (*pci_matcher)(uint16_t vendor, uint32_t class, uint16_t device);

struct PCI_device *PCI_search2(pci_matcher match);

struct PCI_device *PCI_search(uint16_t vendor_id, uint32_t class_id,
                              uint16_t device_id);


static inline void PCI_wr32(struct PCI_device *pci, char no, uint16_t address,
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

static inline uint32_t PCI_rd32(struct PCI_device *pci, char no,
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

static inline void PCI_wr16(struct PCI_device *pci, char no, uint16_t address,
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

static inline uint16_t PCI_rd16(struct PCI_device *pci, char no,
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

/* Read 8 bits from the PCI configuration */
uint8_t PCI_config_read8(uint8_t bus, uint8_t slot, uint8_t func,
                                  uint8_t offset);
/* Read 16 bits from the PCI configuration */
uint16_t PCI_config_read16(uint8_t bus, uint8_t slot, uint8_t func,
                                  uint8_t offset);

/* Read 32 bits from the PCI configuration */
uint32_t PCI_config_read32(uint8_t bus, uint8_t slot, uint8_t func,
                                  uint8_t offset);
/* Write 32 bits from the PCI configuration */
void PCI_config_write32(uint8_t bus, uint8_t slot, uint8_t func,
                               uint8_t offset, uint32_t value);

static inline uint8_t PCI_cfg_rd8(struct PCI_device *pci, uint8_t off) {
    return PCI_config_read8(pci->bus, pci->slot, 0, off);
}

static inline uint16_t PCI_cfg_rd16(struct PCI_device *pci, uint8_t off) {
    return PCI_config_read16(pci->bus, pci->slot, 0, off);
}

static inline uint32_t PCI_cfg_rd32(struct PCI_device *pci, uint8_t off) {
    return PCI_config_read32(pci->bus, pci->slot, 0, off);
}

static inline void PCI_cfg_wr32(struct PCI_device *pci, uint8_t off, uint32_t val) {
    PCI_config_write32(pci->bus, pci->slot, 0, off, val);
}



#define PCI_write(p,a,v) PCI_wr32(p,0,a,v)
#define PCI_read(p,a) PCI_rd32(p,0,a)

#define PCI_write16(p,a,v) PCI_wr16(p,0,a,v)
#define PCI_read16(p,a) PCI_rd16(p,0,a)


#endif  /* __KERNEL_DRV_PCI_H */
