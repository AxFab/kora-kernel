/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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

void csl_early_init();
void com_early_init();

PACK(struct mboot_info {
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
}) *mboot_table;

struct mboot_module {
    void *start;
    void *end;
    char *string;
};

#define GRUB_MEM_BOUND  (1 << 0)
#define GRUB_BOOT_DEVICE  (1 << 1)
#define GRUB_CMDLINE  (1 << 2)
#define GRUB_MODULES  (1 << 3)
#define GRUB_MEMORY  (1 << 6)
#define GRUB_BOOT_LOADER  (1 << 9)
#define GRUB_VGA  (1 << 11)

int mboot_init(void *table)
{
    mboot_table = (struct mboot_info *)table;
    csl_early_init();
    com_early_init();

    // Those call to kprintf crash under VirtualBox! Why!?
#if 0
    if (mboot_table->flags & GRUB_BOOT_LOADER)
        kprintf(KLOG_MSG, "Boot Loader: %s\n", mboot_table->boot_loader);

    if (mboot_table->flags & GRUB_CMDLINE)
        kprintf(KLOG_MSG, "Command line: %s\n", mboot_table->cmdline);

    if (mboot_table->flags & GRUB_BOOT_DEVICE) {
        if (mboot_table->boot_dev == 0x80)   // 1000b
            kprintf(KLOG_MSG, "Booting device: HDD\n");

        else if (mboot_table->boot_dev == 0xe0)   // 1110b
            kprintf(KLOG_MSG, "Booting device: CD\n");

        else
            kprintf(KLOG_MSG, "Booting device: Unknown <%2x>\n", mboot_table->boot_dev);
    }

#endif


    return 0;
}

void mboot_memory()
{
    if (mboot_table->flags & GRUB_MEMORY) {
    }

    uint32_t *ram = mboot_table->mmap_addr;
    // kprintf(KLOG_MSG, "Memory Zones: (at %p)\n", ram);

    for (; *ram == 0x14; ram += 6) {
        int64_t base = (int64_t)ram[1] | ((int64_t)ram[2] << 32);
        int64_t length = (int64_t)ram[3] | ((int64_t)ram[4] << 32);
        // kprintf(-1, " - %08x -%08x -%o:%o\n", (uint32_t)base, (uint32_t)length, ram[0], ram[5]);
        if (base < 4 * _Mib_) { // First 4 Mb are reserved for kernel code
            length -= 4 * _Mib_ - base;
            base = 4 * _Mib_;
        }

        if (length > 0 && ram[5] == 1)
            page_range(base, length);
    }
}

#include <kernel/vfs.h>
#include <kora/llist.h>
// #include <kernel/bio.h>

inode_t *tar_mount(void *base, void *end, CSTR name);
typedef struct dynlib dynlib_t;

struct dynlib {
    char *name;
    int rcu;
    size_t entry;
    size_t init;
    size_t fini;
    size_t base;
    size_t length;
    bio_t *io;
    llnode_t node;
    llhead_t sections;
    llhead_t depends;
    llhead_t intern_symbols;
    llhead_t extern_symbols;
    llhead_t relocations;
};

void mboot_load_modules()
{
    unsigned i;
    if (mboot_table->flags & GRUB_MODULES) {
        struct mboot_module *mods = (struct mboot_module *)mboot_table->mods_addr;
        for (i = 0; i < mboot_table->mods_count; ++i) {
            kprintf(KLOG_MSG, "Module preloaded [%s] %s\n", sztoa(mods->end - mods->start), mods->string);
            inode_t *root = tar_mount(mods->start, mods->end, mods->string);
            kmod_mount(root);
            vfs_close(root);
        }
    }
}
