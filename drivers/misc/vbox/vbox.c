/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#include <kernel/bus/pci.h>
#include <kernel/arch.h>
#include <kernel/irq.h>

#define VBOX_VENDOR_ID 0x80EE
#define VBOX_DEVICE_ID 0xCAFE
#define VBOX_NAME "VirtualBox Guest Additions"

#define VBOX_REQ_MOUSE_GET 1
#define VBOX_REQ_MOUSE 2
#define VBOX_REQ_EVENTS 41
#define VBOX_REQ_DISPLAY 51
#define VBOX_REQ_GUEST_INFO 50
#define VBOX_REQ_GUEST_CAPS 55

#define VBOX_MOUSE_ON (1 << 0) | (1 << 4)
#define VBOX_MOUSE_OFF (0)
#define VBOX_CAPS_MOUSE (1 << 2)

struct vbox_header {
    uint32_t size;
    uint32_t version;
    uint32_t request_type;
    uint32_t rc;
    uint32_t reserved1;
    uint32_t reserved2;
};

struct vbox_guest_info {
    struct vbox_header head;
    uint32_t version;
    uint32_t ostype;
} *vbox_guest_info;

struct vbox_guest_caps {
    struct vbox_header head;
    uint32_t caps;
} *vbox_guest_caps;

struct vbox_events {
    struct vbox_header head;
    uint32_t events;
} *vbox_events;

struct vbox_display {
    struct vbox_header head;
    uint32_t xres;
    uint32_t yres;
    uint32_t bpp;
    uint32_t eventack;
} *vbox_display;

struct vbox_mouse {
    struct vbox_header head;
    uint32_t features;
    int32_t x;
    int32_t y;
} *vbox_mouse;

struct vbox_mouse *vbox_mouse_get;

uint32_t *vbox_vmmdev = 0;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void vbox_set_header(struct vbox_header *header, int size, int req)
{
    header->size = size;
    header->version = 0x10001; // VBOX_VERSION
    header->request_type = req;
    header->rc = 0;
    header->reserved1 = 0;
    header->reserved2 = 0;
}

void vbox_irq_handler(struct PCI_device *pci)
{
    kprintf(KL_MSG, "IRQ for %s\n", VBOX_NAME);
}

void vbox_startup(struct PCI_device *pci)
{
    // IO region #0: d020..d040
    // MMIO region #1: f0400000..f0800000
    // MMIO PREFETCH region #2: f0800000..f0804000
    irq_register(pci->irq, (irq_handler_t)vbox_irq_handler, pci);

    vbox_guest_info = kmap(PAGE_SIZE, NULL, 0, VM_RW | VM_RESOLVE);
    vbox_set_header(&vbox_guest_info->head, sizeof(*vbox_guest_info),
                    VBOX_REQ_GUEST_INFO);
    vbox_guest_info->version = 0x00010003;
    vbox_guest_info->ostype = 0;
    PCI_wr32(pci, 0, 0, mmu_read((size_t)vbox_guest_info));

    vbox_guest_caps = kmap(PAGE_SIZE, NULL, 0, VM_RW | VM_RESOLVE);
    vbox_set_header(&vbox_guest_caps->head, sizeof(*vbox_guest_caps),
                    VBOX_REQ_GUEST_CAPS);
    vbox_guest_caps->caps = 1;
    PCI_wr32(pci, 0, 0, mmu_read((size_t)vbox_guest_caps));

    vbox_events = kmap(PAGE_SIZE, NULL, 0, VM_RW | VM_RESOLVE);
    vbox_set_header(&vbox_events->head, sizeof(*vbox_events), VBOX_REQ_EVENTS);
    vbox_events->events = 0;
    PCI_wr32(pci, 0, 0, mmu_read((size_t)vbox_events));

    vbox_display = kmap(PAGE_SIZE, NULL, 0, VM_RW | VM_RESOLVE);
    vbox_set_header(&vbox_display->head, sizeof(*vbox_display), VBOX_REQ_DISPLAY);
    vbox_display->eventack = 1;
    PCI_wr32(pci, 0, 0, mmu_read((size_t)vbox_display));

    vbox_mouse = kmap(PAGE_SIZE, NULL, 0, VM_RW | VM_RESOLVE);
    vbox_set_header(&vbox_mouse->head, sizeof(*vbox_mouse), VBOX_REQ_MOUSE);
    vbox_mouse->features = VBOX_MOUSE_ON;
    PCI_wr32(pci, 0, 0, mmu_read((size_t)vbox_mouse));

    vbox_mouse_get = kmap(PAGE_SIZE, NULL, 0, VM_RW | VM_RESOLVE);
    vbox_set_header(&vbox_mouse_get->head, sizeof(*vbox_mouse_get),
                    VBOX_REQ_MOUSE_GET);
    vbox_mouse_get->features = VBOX_MOUSE_ON;
    PCI_wr32(pci, 0, 0, mmu_read((size_t)vbox_mouse));

    pci->bar[1].mmio = (uint32_t)kmap(pci->bar[1].size, NULL, pci->bar[1].base & ~7,
                                      VMA_PHYS | VM_RW);
    kprintf(KL_MSG, "%s MMIO mapped at %x\n", VBOX_NAME, pci->bar[1].mmio);

    vbox_vmmdev = (uint32_t *)pci->bar[1].mmio;
    vbox_vmmdev[3] = 0xFFFFFFFF;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int vbox_match_pci_device(uint16_t vendor, uint32_t class, uint16_t device)
{
    if (vendor != VBOX_VENDOR_ID || device != VBOX_DEVICE_ID)
        return -1;
    return 0;
}


void vbox_setup()
{
    struct PCI_device *pci = NULL;

    for (;;) {
        pci = pci_search(vbox_match_pci_device, NULL);
        if (pci == NULL)
            break;
        vbox_startup(pci);
    }
}

void vbox_teardown()
{
}


MODULE(vbox, vbox_setup, vbox_teardown);
