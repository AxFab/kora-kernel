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
#include <kernel/stdc.h>
#include <kernel/vfs.h>
#include <kernel/bus/pci.h>

#define VGA_PORT_CMD 0x1CE
#define VGA_PORT_DATA 0x1CF

#define VGA_REG_RESOL_X 0x01
#define VGA_REG_RESOL_Y 0x02
#define VGA_REG_BPP 0x03
#define VGA_REG_VIRT_Y 0x07
#define VGA_REG_OFF_Y 0x09
#define VGA_REG_MEMORY 0x0A

struct device_id {
    uint16_t vendor;
    uint16_t id;
    const char *name;
    void (*start)(struct PCI_device *pci, struct device_id *info);
};

typedef struct vga_info vga_info_t;
struct vga_info {
    struct PCI_device *pci;
    void *pixels0;
    void *pixels1;
    size_t offset;
    size_t width;
    size_t height;
    size_t pitch;
};

uint16_t size[] = {
    320, 200, // 16:10
    640, 480,  // 4:3
    720, 400, // 9:5
    720, 480, // 3:2
    800, 600, // 4:3
    1024, 768, // 4:3 (3Mb)
    1152, 864, // 4:3
    1280, 720, // 19:9
    1280, 800, // 16:10
    1280, 960, // 4:3
    1280, 1024, // 5:4 (5Mb)
    1366, 768, // 16:9
    1440, 900, // 16:10
    1440, 1080, // 4:3
    1600, 900, // 16:9
    1680, 1050, // 16:10
    1600, 1200, // 4:3
    1920, 1080, // 16:9
    1920, 1200, // 16:10
    1920, 1440, // 4:3
    2880, 1800, // 16:10 (~20Mb)
};

static void vga_change_resol(uint16_t width, uint16_t height)
{
    // kprintf(0, "VGA Resolution requested to %dx%dx32 \n",  width, height);

    outw(VGA_PORT_CMD, 0x04);
    outw(VGA_PORT_DATA, 0x00);

    outw(VGA_PORT_CMD, VGA_REG_RESOL_X);
    outw(VGA_PORT_DATA, width);

    outw(VGA_PORT_CMD, VGA_REG_RESOL_Y);
    outw(VGA_PORT_DATA, height);

    outw(VGA_PORT_CMD, VGA_REG_BPP);
    outw(VGA_PORT_DATA, 32);

    outw(VGA_PORT_CMD, VGA_REG_VIRT_Y);
    outw(VGA_PORT_DATA, height << 1);

    outw(VGA_PORT_CMD, 0x04);
    outw(VGA_PORT_DATA, 0x41);

    /* Read to check we have the correct parameters */
    outw(VGA_PORT_CMD, VGA_REG_RESOL_X);
    width = inw(VGA_PORT_DATA);

    outw(VGA_PORT_CMD, VGA_REG_RESOL_Y);
    height = inw(VGA_PORT_DATA);

    kprintf(KL_MSG, "VGA Resolution: %dx%dx32 \n", width, height);
}

static void vga_change_offset(uint16_t offset)
{
    outw(VGA_PORT_CMD, VGA_REG_OFF_Y);
    outw(VGA_PORT_DATA, offset);
}

page_t vga_fetch(inode_t *ino, xoff_t off)
{
    vga_info_t *info = ino->drv_data;
    size_t base = info->pci->bar[0].base & ~(PAGE_SIZE - 1);

    // framebuffer_t *fb = (framebuffer_t *)ino->info;
    // return mmu_read(ADDR_OFF(fb->pixels, off));
    return base + off;
}

void vga_flip(inode_t *ino, size_t newoffset)
{
    vga_info_t *info = ino->drv_data;
    vga_change_offset(newoffset);
    info->offset = newoffset;
    if (info->offset == 0)
        memcpy32(info->pixels1, info->pixels0, info->pitch * info->height);
    else
        memcpy32(info->pixels0, info->pixels1, info->pitch * info->height);
}

int vga_ioctl(inode_t *ino, int cmd, size_t *params)
{
    vga_info_t *info = ino->drv_data;
    if (cmd == FB_FLIP) {
        vga_flip(ino, params[0]);
        return 0;
    }
    if (cmd == FB_SIZE)
        return info->width | info->height << 16;
    return -1;
}

ino_ops_t vga_ino_ops = {
    .ioctl = vga_ioctl,
    .fetch = vga_fetch,
};

void vga_start_qemu(struct PCI_device *pci, struct device_id *devinfo)
{
    // Tested on QEMU and VirtualBox (should works for BOCHS too)
    // MMIO PREFETCH region #0: fd000000..fe000000
    // MMIO region #2: febf0000..febf1000

    char tmp[20];
    outw(VGA_PORT_CMD, 0x00);
    uint16_t i = inw(VGA_PORT_DATA);
    if (i < 0xB0C0 || i > 0xB0C6)
        return;

    outw(VGA_PORT_DATA, 0xB0C4);
    i = inw(VGA_PORT_DATA);

    // kprintf(KL_MSG, "VGA, PCI device %02x.%02x.%x (%p)\n", pci->bus, pci->slot, pci->func, pci);
    outw(VGA_PORT_CMD, VGA_REG_MEMORY);
    i = inw(VGA_PORT_DATA);
    uint32_t mem = i > 1 ? (uint32_t)i * 64 * 1024 : inl(VGA_PORT_DATA);
    kprintf(KL_MSG, "VGA Memory size %s\n", sztoa_r(mem, tmp));

    pci->bar[0].mmio = (uint32_t)kmap(pci->bar[0].size, NULL, pci->bar[0].base & ~7,
                                      VM_WR | VMA_PHYS | VM_UNCACHABLE);

    uint32_t pixels0 = pci->bar[0].mmio;
    // kprintf(KLOG_DBG, "%s MMIO mapped at %x\n", info->name, pixels0);

    // Load surface device !
    i = 7;
    while (size[i * 2] * size[i * 2 + 1] * 8U > mem)
        --i;
    vga_change_resol(size[i * 2], size[i * 2 + 1]);


    inode_t *ino = vfs_inode(1, FL_FRM, NULL, &vga_ino_ops);
    ino->dev->model = strdup(devinfo->name);
    ino->dev->devclass = strdup("VGA Screen");

    vga_info_t *info = kalloc(sizeof(vga_info_t));
    ino->drv_data = info;
    info->pci = pci;
    info->width = size[i * 2];
    info->height = size[i * 2 + 1];
    info->offset = 0;
    info->pitch = ALIGN_UP(info->width * 4, 4);
    info->pixels0 = pixels0;
    info->pixels1 = pixels0 + info->pitch * info->height;

    vga_change_offset(info->pitch * info->height);
    // vfs_fcntl(ino, FB_RESIZE, &size[i * 2]);
    vfs_mkdev(ino, "fb0");
    vfs_close_inode(ino);
}
