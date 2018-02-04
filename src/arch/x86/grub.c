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
void seat_init_framebuffer(int no, int width, int height, void *pixels, uint8_t depth);

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
        // kprintf(0, "\nBoot Loader: %s\n", (char *)grub_table[16]);
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
        // We are in pixel mode
        seat_init_framebuffer(0, grub_table[25], grub_table[26],
                              (void *)grub_table[22], grub_table[27] & 0xFF);
    } else {
        // We are in text mode
    }


    if (!(grub_table[0] & GRUB_MEMORY)) {
        return -1;
    }

    return 0;
}
