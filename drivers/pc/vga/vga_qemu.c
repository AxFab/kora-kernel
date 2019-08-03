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
#include <kernel/core.h>
#include <kernel/files.h>
#include <kernel/device.h>
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

    kprintf(KLOG_MSG, "VGA Resolution: %dx%dx32 \n", width, height);
}

static void vga_change_offset(uint16_t offset)
{
    outw(VGA_PORT_CMD, VGA_REG_OFF_Y);
    outw(VGA_PORT_DATA, offset);
}

int vga_fcntl(inode_t *ino, int cmd, size_t *params)
{
    if (cmd == 800) {
        vga_flip(ino);
        return 0;
    }
    return -1;
}

page_t vga_fetch(inode_t *ino, off_t off)
{
    framebuffer_t *fb = (framebuffer_t *)ino->info;
    return mmu_read(ADDR_OFF(fb->pixels, off));
}

void vga_flip(inode_t *ino)
{
    framebuffer_t *fb = (framebuffer_t *)ino->info;
    vga_change_offset(fb->pixels > fb->backup ? fb->height : 0);
    uint8_t *tmp = fb->pixels;
    fb->pixels = fb->backup;
    fb->backup = tmp;

    memcpy32(fb->pixels, fb->backup, fb->width * fb->height * 4);
}

ino_ops_t vga_ino_ops = {
    .flip = vga_flip,
    .fcntl = vga_fcntl,
    .fetch = vga_fetch,
};

dev_ops_t vga_dev_ops = {

};

void vga_start_qemu(struct PCI_device *pci, struct device_id *info)
{
    // Tested on QEMU and VirtualBox (should works for BOCHS too)
    // MMIO PREFETCH region #0: fd000000..fe000000
    // MMIO region #2: febf0000..febf1000

    outw(VGA_PORT_CMD, 0x00);
    uint16_t i = inw(VGA_PORT_DATA);
    if (i < 0xB0C0 || i > 0xB0C6)
        return;

    outw(VGA_PORT_DATA, 0xB0C4);
    i = inw(VGA_PORT_DATA);

    outw(VGA_PORT_CMD, VGA_REG_MEMORY);
    i = inw(VGA_PORT_DATA);
    uint32_t mem = i > 1 ? (uint32_t)i * 64 * 1024 : inl(VGA_PORT_DATA);
    kprintf(KLOG_DBG, "VGA Memory size %s \n", sztoa(mem));

    pci->bar[0].mmio = (uint32_t)kmap(pci->bar[0].size, NULL, pci->bar[0].base & ~7,
                                      VMA_PHYSIQ);

    uint32_t pixels0 = pci->bar[0].mmio;
    // kprintf(KLOG_DBG, "%s MMIO mapped at %x\n", info->name, pixels0);

    // Load surface device !
    i = 7;
    while (size[i * 2] * size[i * 2 + 1] * 8U > mem)
        --i;
    vga_change_resol(size[i * 2], size[i * 2 + 1]);

    framebuffer_t *fb = gfx_create(size[i * 2], size[i * 2 + 1], 4, (void *) - 1);
    uint32_t pixels1 = pixels0 + fb->pitch * fb->height;
    fb->pixels = (uint8_t *)pixels0;
    fb->backup = (uint8_t *)pixels1;
    vga_change_offset(fb->height);

    inode_t *ino = vfs_inode(1, FL_VDO, NULL);
    ino->info = fb;
    ino->ops = &vga_ino_ops;
    ino->dev->ops = &vga_dev_ops;
    ino->dev->model = (char *)info->name;
    ino->dev->devclass = "VGA Screen";
    vfs_mkdev(ino, "fb0");
}
