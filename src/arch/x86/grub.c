#include <kernel/core.h>
#include <kora/iofile.h>
#include <string.h>

uint32_t *grub_table;

#define GRUB_BOOT_DEVICE  (1 << 1)
#define GRUB_MEMORY  (1 << 6)
#define GRUB_BOOT_LOADER  (1 << 9)
#define GRUB_VGA  (1 << 11)


int grub_start(uint32_t *table)
{
    grub_table = table;

    memset((void *)0x000B8000, 0, 80 * 25 * 2);

    if (grub_table[0] & GRUB_BOOT_LOADER) {
        // kprintf(KLOG_MSG, (char *)grub_table[16]);
        // kprintf(0, "Boot Loader: %s\n", (char *)grub_table[16]);
    }

    if (grub_table[0] & GRUB_BOOT_DEVICE) {
        if (grub_table[3] >> 24 == 0x80) { // 1000b
            // kprintf(KLOG_MSG, "Booting device: HDD\n");
        } else if (grub_table[3] >> 24 == 0xe0) { // 1110b
            // kprintf(KLOG_MSG, "Booting device: CD\n");
        } else {
            // kprintf(KLOG_MSG, "Booting device: Unknown <%2x>\n", grub_table[3] >> 24);
        }
    }

    if (grub_table[0] & GRUB_VGA && grub_table[22] != 0x000B8000) {
        if (((grub_table[27] >> 8) & 0xFF) != 1) {
            // Unsupported color schemas
        }

        // We are in pixel mode
        // seat_init_framebuffer(0, grub_table[25], grub_table[26],
        //                       (void *)grub_table[22], grub_table[27] & 0xFF);
        // seat_fb0(grub_table[25], grub_table[26], grub_table[27] & 0xFF,
        //          grub_table[24], (void *)grub_table[22]);

        /*
        If ‘framebuffer_type’ is set to ‘1’ it means direct RGB color will be used. Then color_type is defined as follows:
                     +----------------------------------+
             110     | framebuffer_red_field_position   | 27 + 2
             111     | framebuffer_red_mask_size        | 27 + 3
             112     | framebuffer_green_field_position | 28 + 0
             113     | framebuffer_green_mask_size      | 28 + 1
             114     | framebuffer_blue_field_position  | 28 + 2
             115     | framebuffer_blue_mask_size       | 28 + 3
                     +----------------------------------+
        */
    } else {
        // We are in text mode
    }


    if (!(grub_table[0] & GRUB_MEMORY)) {
        return -1;
    }

    return 0;
}
