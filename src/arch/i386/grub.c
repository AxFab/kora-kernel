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

void csl_early_init();
int csl_write(inode_t *ino, const char *buf, int len);

PACK(struct grub_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint8_t rsvd3a;
    uint8_t rsvd3b;
    uint8_t rsvd3c;
    uint8_t boot_dev;
    char *cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t *mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t rsvd15;
    char *boot_loader;
    uint32_t apm_table; // Flags 10
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
}) *grub_table;

#define GRUB_MEM_BOUND  (1 << 0)
#define GRUB_BOOT_DEVICE  (1 << 1)
#define GRUB_CMDLINE  (1 << 2)
#define GRUB_MODULES  (1 << 3)
#define GRUB_MEMORY  (1 << 6)
#define GRUB_BOOT_LOADER  (1 << 9)
#define GRUB_VGA  (1 << 11)

int grub_init(void *table)
{
    grub_table = (struct grub_info*)table;
    csl_early_init();

    if (grub_table->flags & GRUB_BOOT_LOADER) {
        kprintf(KLOG_MSG, "Boot Loader: %s\n", grub_table->boot_loader);
    }

    if (grub_table->flags & GRUB_CMDLINE) {
        kprintf(KLOG_MSG, "Command line: %s\n", grub_table->cmdline);
    }

    if (grub_table->flags & GRUB_BOOT_DEVICE) {
        if (grub_table->boot_dev == 0x80) { // 1000b
            kprintf(KLOG_MSG, "Booting device: HDD\n");
        } else if (grub_table->boot_dev == 0xe0) { // 1110b
            kprintf(KLOG_MSG, "Booting device: CD\n");
        } else {
            kprintf(KLOG_MSG, "Booting device: Unknown <%2x>\n", grub_table->boot_dev);
        }
    }


    return 0;
}

void grub_memory()
{
    if (grub_table->flags & GRUB_MEMORY) {
    }

    uint32_t *ram = grub_table->mmap_addr;
    // kprintf(KLOG_MSG, "Memory Zones: (at %p)\n", ram);

    for (; *ram == 0x14; ram += 6) {
        int64_t base = (int64_t)ram[1] | ((int64_t)ram[2] << 32);
        int64_t length = (int64_t)ram[3] | ((int64_t)ram[4] << 32);
        // kprintf(-1, " - %08x -%08x -%o:%o\n", (uint32_t)base, (uint32_t)length, ram[0], ram[5]);
        if (base < 2 * _Mib_) { // First 2 Mb are reserved for kernel code
            length -= 2 * _Mib_ - base;
            base = 2 * _Mib_;
        }

        if (length > 0 && ram[5] == 1)
            page_range(base, length);
    }
}
