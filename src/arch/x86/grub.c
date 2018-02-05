#include <kernel/core.h>
#include <kora/iofile.h>
#include <string.h>

uint32_t *grub_table;
FILE grub_file;

#define GRUB_BOOT_DEVICE  (1 << 1)
#define GRUB_MEMORY  (1 << 6)
#define GRUB_BOOT_LOADER  (1 << 9)
#define GRUB_VGA  (1 << 11)

extern FILE *klogs;

int SRL_setup();
void SRL_write(const char *msg, size_t lg);
void TXT_write(const char *msg, size_t lg);
void seat_fb0(int width, int height, int format, int pitch, void* mmio);

int grub_write(FILE *f, const char *s, size_t l)
{
    TXT_write(s, l);
    SRL_write(s, l);
    return 0;
}

int grub_start(uint32_t *table)
{
    grub_table = table;

    SRL_setup();

    grub_file.lbuf_ = EOF;
    grub_file.lock_ = -1;
    grub_file.write = grub_write;
    klogs = &grub_file;

    memset((void *)0x000B8000, 0, 80 * 25 * 2);

    if (grub_table[0] & GRUB_BOOT_LOADER) {
        kprintf(0, (char *)grub_table[16]);
        // kprintf(0, "Boot Loader: %s\n", (char *)grub_table[16]);
    }

    if (grub_table[0] & GRUB_BOOT_DEVICE) {
        if (grub_table[3] >> 24 == 0x80) { // 1000b
            kprintf(0, "Booting device: HDD\n");
        } else if (grub_table[3] >> 24 == 0xe0) { // 1110b
            kprintf(0, "Booting device: CD\n");
        } else {
            kprintf(0, "Booting device: Unknown <%2x>\n", grub_table[3] >> 24);
        }
    }

    if (grub_table[0] & GRUB_VGA && grub_table[22] != 0x000B8000) {
        if (((grub_table[27] >> 8) & 0xFF) != 1) {
            // Unsupported color schemas
        }

        // We are in pixel mode
        // seat_init_framebuffer(0, grub_table[25], grub_table[26],
        //                       (void *)grub_table[22], grub_table[27] & 0xFF);
        seat_fb0(grub_table[25], grub_table[26], grub_table[27] & 0xFF,
                 grub_table[24], (void *)grub_table[22]);

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
