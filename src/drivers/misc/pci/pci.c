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
#include <kernel/drv/pci.h>
#include <kora/atomic.h>
#include <string.h>

#ifdef K_MODULE
#  define PCI_setup setup
#  define PCI_teardown teardown
#endif

#define PCI_IO_CFG_ADDRESS 0xCF8
#define PCI_IO_CFG_DATA 0xCFC

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct PCI_header {
    uint16_t vendor_; // 0
    uint16_t device_; // 2
    uint16_t command_; // 4
    uint16_t status_; // 6
    uint8_t revision_; // 8
    uint8_t prog_IF_; // 9
    uint8_t subclass_; // 10
    uint8_t class_; // 11
    uint8_t cache_line_size_; // 12
    uint8_t latency_timer_; // 13
    uint8_t header_type_; // 14
    uint8_t bist_;  // 15
    uint32_t bar_[0];  // 16
};

/* Read 8 bits from the PCI configuration */
uint8_t PCI_config_read8(uint8_t bus, uint8_t slot, uint8_t func,
                                  uint8_t offset)
{
    uint32_t address = 0;

    /* Create configuration address */
    address |= (uint32_t)bus << 16;
    address |= (uint32_t)(slot & 0x1f) << 11;
    address |= (uint32_t)(func & 0x7) << 8;
    address |= (uint32_t)(offset & 0xfc);
    address |= 0x80000000;
    outl(PCI_IO_CFG_ADDRESS, address);

    /* Read the data */
    return (inl(PCI_IO_CFG_DATA) >> ((offset & 2) << 3)) & 0xff;
}

/* Read 16 bits from the PCI configuration */
uint16_t PCI_config_read16(uint8_t bus, uint8_t slot, uint8_t func,
                                  uint8_t offset)
{
    uint32_t address = 0;

    /* Create configuration address */
    address |= (uint32_t)bus << 16;
    address |= (uint32_t)(slot & 0x1f) << 11;
    address |= (uint32_t)(func & 0x7) << 8;
    address |= (uint32_t)(offset & 0xfc);
    address |= 0x80000000;
    outl(PCI_IO_CFG_ADDRESS, address);

    /* Read the data */
    return (inl(PCI_IO_CFG_DATA) >> ((offset & 2) << 3)) & 0xffff;
}

/* Read 32 bits from the PCI configuration */
uint32_t PCI_config_read32(uint8_t bus, uint8_t slot, uint8_t func,
                                  uint8_t offset)
{
    uint32_t address = 0;

    /* Create configuration address */
    address |= (uint32_t)bus << 16;
    address |= (uint32_t)(slot & 0x1f) << 11;
    address |= (uint32_t)(func & 0x7) << 8;
    address |= (uint32_t)(offset & 0xfc);
    address |= 0x80000000;
    outl(PCI_IO_CFG_ADDRESS, address);

    /* Read the data */
    return inl(PCI_IO_CFG_DATA);
}

/* Write 32 bits from the PCI configuration */
void PCI_config_write32(uint8_t bus, uint8_t slot, uint8_t func,
                               uint8_t offset, uint32_t value)
{
    uint32_t address = 0;

    /* Create configuration address */
    address |= (uint32_t)bus << 16;
    address |= (uint32_t)(slot & 0x1f) << 11;
    address |= (uint32_t)(func & 0x7) << 8;
    address |= (uint32_t)(offset & 0xfc);
    address |= 0x80000000;
    outl(PCI_IO_CFG_ADDRESS, address);

    /* Write data */
    outl(PCI_IO_CFG_DATA, value);
}
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#ifdef K_MODULE

static const char *PCI_vendor_name(uint32_t vendor_id)
{
    switch (vendor_id) {
#  include "pci_db_vendors.h"
    }
    return NULL;
}

static const char *PCI_class_name(uint32_t class_id)
{
    switch (class_id >> 8) {
    case 0x0101:
        return "IDE Controller";
    case 0x0206:
        return "PICMG 2.14 Multi Computing";
    case 0x0608:
        return "RACEway Bridge";
    case 0x0E00:
        return "I20 Architecture";
    }
    switch (class_id) {
#  include "pci_db_classes.h"
    }
    return NULL;
}

#else

static const char *PCI_vendor_name(uint32_t vendor_id)
{
    switch (vendor_id) {
    case 0x04B3:
        return "IBM";
    case 0x8086:
        return "Intel corporation";
    case 0x1022:
        return "Intel corporation";
    case 0x80ee:
        return "InnoTek";
    case 0x05ac:
        return "Apple Inc.";
    case 0x106b:
        return "Apple Inc.";
    }
    return NULL;
}

static const char *PCI_class_name(uint32_t class_id)
{
    switch (class_id >> 8) {
    case 0x0101:
        return "IDE Controller";
    case 0x0206:
        return "PICMG 2.14 Multi Computing";
    case 0x0608:
        return "RACEway Bridge";
    case 0x0E00:
        return "I20 Architecture";
    }
    switch (class_id) {
    case 0x010000:
        return "SCSI Bus Controller";
    case 0x010200:
        return "Floppy Disk Controller";
    case 0x010300:
        return "IPI Bus Controller";
    case 0x010400:
        return "RAID Controller";
    case 0x010520:
        return "ATA Controller (Single DMA)";
    case 0x010530:
        return "ATA Controller (Chained DMA)";
    case 0x020000:
        return "Ethernet Controller";
    case 0x020100:
        return "Token Ring Controller";
    case 0x030000:
        return "VGA-Compatible Controller";
    case 0x040000:
        return "Video Device";
    case 0x040100:
        return "Audio Device";
    case 0x060000:
        return "Host bridge";
    case 0x060100:
        return "ISA bridge";
    case 0x060200:
        return "EISA bridge";
    case 0x060300:
        return "MCA bridge";
    case 0x060400:
        return "PCI-to-PCI bridge";
    case 0x068000:
        return "Bridge Device";
    case 0x088000:
        return "System Peripheral";
    case 0x0C0300:
        return "USB (Universal Host)";
    case 0x0C0310:
        return "USB (Open Host)";
    case 0x0C0320:
        return "USB2 (Intel Enhanced Host)";
    case 0x0C0330:
        return "USB3 XHCI";
    }
    return NULL;
}

#endif

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct PCI_device device_stack[128];
int dev_sp = 0;

void PCI_check_bus(uint8_t bus);

static void PCI_check_func(uint8_t bus, uint8_t slot, uint8_t func)
{
    uint16_t classes;
    uint8_t secondary;

    classes = PCI_config_read16(bus, slot, func, 10);
    if (classes == 0x0604) {
        secondary = PCI_config_read16(bus, slot, func, 24) >> 8;
        PCI_check_bus(secondary);
    }
}

static void PCI_check_device(uint8_t bus, uint8_t slot)
{
    uint8_t head;
    uint16_t vendor_id;
    vendor_id = PCI_config_read16(bus, slot, 0, 0);
    if (vendor_id == 0xffff) {
        return;
    }

    /* We found a new PCI device */
    device_stack[dev_sp].bus = bus;
    device_stack[dev_sp].slot = slot;
    device_stack[dev_sp].irq = PCI_config_read16(bus, slot, 0, 60) & 0xFF;
    device_stack[dev_sp].busy = 0;
    device_stack[dev_sp].vendor_id = vendor_id;
    device_stack[dev_sp].class_id = PCI_config_read32(bus, slot, 0, 8) >> 8;
    device_stack[dev_sp].device_id = PCI_config_read16(bus, slot, 0, 2);

    const char *vendor_name = PCI_vendor_name(vendor_id);
    const char *class_name = PCI_class_name(device_stack[dev_sp].class_id);

    kprintf(KLOG_MSG, "PCI.%02x.%02x ", bus, slot);
    if (vendor_name != NULL)
        kprintf(KLOG_MSG, " %s", vendor_name);
    if (class_name != NULL)
        kprintf(KLOG_MSG, " %s", class_name);
    kprintf(KLOG_MSG, " (%04x.%05x.%04x) ", vendor_id, device_stack[dev_sp].class_id, device_stack[dev_sp].device_id);

    if (device_stack[dev_sp].irq != 0)
        kprintf(KLOG_MSG, " IRQ%d", device_stack[dev_sp].irq);
    kprintf(KLOG_MSG, "\n");

    int i;
    for (i = 0; i < 4; ++i) {
        int add = 16 + i * 4;
        uint32_t bar = PCI_config_read32(bus, slot, 0, add);
        PCI_config_write32(bus, slot, 0, add, 0xFFFFFFFF);
        uint32_t bar_sz = (~PCI_config_read32(bus, slot, 0, add)) + 1;
        PCI_config_write32(bus, slot, 0, add, bar);

        device_stack[dev_sp].bar[i].base = bar;
        device_stack[dev_sp].bar[i].size = bar_sz;

        if ((bar & 3) != 0) {
            kprintf(KLOG_DBG, "          IO region #%d: %x..%x \n", i, bar & 0xFFFFFFFC,
                    (bar & 0xFFFFFFFC) + bar_sz + 1);
        } else if (bar & 8) {
            kprintf(KLOG_DBG, "          MMIO PREFETCH region #%d: %08x..%08x\n", i, bar & ~15,
                    (bar & ~15) + bar_sz + 8);
        } else if (bar_sz != 0) {
            kprintf(KLOG_DBG, "          MMIO region #%d: %08x..%08x\n", i, bar & ~15,
                    (bar & ~15) + bar_sz);
        }
    }

    dev_sp++;


    head = PCI_config_read16(bus, slot, 0, 14) & 0xff;
    PCI_check_func(bus, slot, 0);
    if ((head & 0x80) == 0) {
        for (head = 1; head < 8; head++) {
            if (PCI_config_read16(bus, slot, 0, 0) != 0xffff) {
                PCI_check_func(bus, slot, head);
            }
        }
    }
}

void PCI_check_bus(uint8_t bus)
{
    uint8_t slot;
    for (slot = 0; slot < 32; slot++) {
        PCI_check_device(bus, slot);
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
atomic_uint PCI_detection = 0;
int PCI_ready = 0;

int PCI_detect()
{
    if (atomic_exchange(&PCI_detection, 1) == 1) {
        while (PCI_ready == 0) {
            cpu_relax(); // Not efficent but mutex might not be up yet!
        }
    } else {

        // Check all buses to find devices
        uint8_t head = PCI_config_read16(0, 0, 0, 14) & 0xff;
        if ((head & 0x80) == 0) {
            PCI_check_bus(0);
            PCI_ready = 1;
            return 0;
        }

        for (head = 0; head < 8; head++) {
            if (PCI_config_read16(0, 0, head, 0) != 0xffff) {
                break;
            }
            PCI_check_bus(head);
        }
        PCI_ready = 1;
    }
    return 0;
}

struct PCI_device *PCI_search2(pci_matcher match)
{
    int i;
    for (i = 0; i < dev_sp; ++i) {
        if (device_stack[i].busy != 0) continue;
        if (match(device_stack[i].vendor_id, device_stack[i].class_id, device_stack[i].device_id) == 0) {
            device_stack[i].busy = 1;
            return &device_stack[i];
        }
    }
    return NULL;
}

struct PCI_device *PCI_search(uint16_t vendor_id, uint32_t class_id,
                              uint16_t device_id)
{
    int i;
    PCI_detect();
    // Spin lock !?
    for (i = 0; i < dev_sp; ++i) {
        if (device_stack[i].vendor_id == vendor_id &&
                device_stack[i].class_id == class_id &&
                device_stack[i].device_id == device_id &&
                device_stack[i].busy == 0) {
            device_stack[i].busy = 1;
            return &device_stack[i];
        }
    }
    return NULL;
}

void PCI_setup()
{
    // TODO Register method PCI_search for other modules
    PCI_detect();
}

void PCI_teardown()
{
}


MODULE(pci, MOD_AGPL, PCI_setup, PCI_teardown);
