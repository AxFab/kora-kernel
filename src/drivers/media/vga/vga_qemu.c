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
#include <kernel/drv/pci.h>

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

void vga_flip(surface_t *screen)
{
    vga_change_offset(screen->pixels > screen->backup ? screen->height : 0);
    uint8_t *tmp = screen->pixels;
    screen->pixels = screen->backup;
    screen->backup = tmp;
}

ino_ops_t vga_ino_ops = {

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


    surface_t *screen = vds_create_empty(size[i * 2], size[i * 2 + 1], 4);
    uint32_t pixels1 = pixels0 + screen->pitch * screen->height;
    screen->pixels = (uint8_t *)pixels1;
    vds_fill(screen, 0xe2e2e2);
    screen->pixels = (uint8_t *)pixels0;
    screen->backup = (uint8_t *)pixels1;
    screen->flip = vga_flip;
    vga_change_offset(screen->height);

    inode_t *ino = vfs_inode(1, FL_VDO, NULL);
    ino->info = screen;
    ino->ops = &vga_ino_ops;
    ino->und.dev->ops = &vga_dev_ops;
    ino->und.dev->model = (char *)info->name;
    ino->und.dev->devclass = "VGA Screen";
    vfs_mkdev(ino, "fb0");
    // wmgr_register_screen(screen);
}
