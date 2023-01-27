#include <stdint.h>
#include <kernel/memory.h>
#include <kernel/vfs.h>


extern PACK(struct mboot_info {
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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Load every chunk of available memory from the multiboot table */
void mboot_memory()
{
    uint32_t *ram = mboot_table->mmap_addr;
    for (; *ram == 0x14; ram += 6) {
        int64_t base = (int64_t)ram[1] | ((int64_t)ram[2] << 32);
        int64_t length = (int64_t)ram[3] | ((int64_t)ram[4] << 32);
        if (base < 4 * _Mib_) { // First 4 Mb are reserved for kernel code
            length -= 4 * _Mib_ - base;
            base = 4 * _Mib_;
        }

        if (length > 0 && ram[5] == 1)
            page_range(base, length);
    }
}


void mboot_modules()
{
    unsigned i;
    char tmp [12];
    if (mboot_table->flags & GRUB_MODULES) {
        struct mboot_module *mods = (struct mboot_module *)mboot_table->mods_addr;
        for (i = 0; i < mboot_table->mods_count; ++i) {
            size_t len = (size_t)(((char *)mods->end) - ((char *)mods->start));
            kprintf(-1, "Module preloaded [%s] '%s'\n", sztoa(len), mods->string);
            inode_t *ino = tar_mount(mods->start, len, mods->string);
            snprintf(tmp, 12, "boot%d", i);
            int ret = vfs_early_mount(ino, tmp);
            vfs_close_inode(ino);
        }
    }
}

