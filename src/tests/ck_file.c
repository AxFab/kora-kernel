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
#include <kernel/files.h>
#include <kora/mcrs.h>
#include <string.h>


void vfs_read() {}
void page_range() {}

typedef struct IO_FILE FILE;
FILE *fopen(const char *, const char *);
int fread(void *, size_t, size_t, FILE *);
int fwrite(const void *, size_t, size_t, FILE *);
int fclose(FILE*);


PACK(struct BMP_header {
    uint16_t magic;
    uint32_t length;
    uint32_t reserved;
    uint32_t offset;
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bits;
    uint32_t compress;
    uint32_t image;
    uint32_t resol_x;
    uint32_t resol_y;
    uint32_t colors_map;
    uint32_t colors;
});

void export(const char *name, surface_t *srf)
{
    int i;
    FILE *fp = fopen(name, "w+");
    struct BMP_header header;
    memset(&header, 0, sizeof(header));
    header.magic = 0x4d42;
    header.length = srf->width * srf->height * 3 + sizeof(struct BMP_header);
    header.offset = sizeof(struct BMP_header);
    header.size = 40;
    header.width = srf->width;
    header.height = srf->height;
    header.planes = 1;
    header.bits = 24;
    header.compress = 0;
    header.image = 0;
    fwrite(&header, sizeof(header), 1, fp);
    for (i = 0; i < srf->height; ++i)
        fwrite(&srf->pixels[srf->width * (srf->height-i-1) * 3], srf->width, 3, fp);
    fclose(fp);
}


extern const font_t font_6x10;
extern const font_t font_8x15;
extern const font_t font_7x13;
extern const font_t font_6x9;
extern const font_t font_8x8;


uint32_t colors_std[] = {
    0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0x808080, 0xFFFFFF,
    0xD0D0D0, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF, 0xFFFFFF,
};

uint32_t colors_kora[] = {
    0x181818, 0xA61010, 0x10A610, 0xA66010, 0x1010A6, 0xA610A6, 0x10A6A6, 0xA6A6A6, 0xFFFFFF,
    0x323232, 0xD01010, 0x10D010, 0xD06010, 0x1010D0, 0xD010D0, 0x10D0D0, 0xD0D0D0, 0xFFFFFF,
};

#define TTY(n) tty_write(tty, n, strlen(n))




void test_VDS()
{
    kprintf (-1, "\n\e[94m  IO FILE VDS - <<<>>>\e[0m\n");

    surface_t *screen = vds_create(800, 600, 3);
    surface_t *w0 = vds_create(400, 600, 4);
    surface_t *w1 = vds_create(400, 300, 4);
    surface_t *w2 = vds_create(200, 300, 4);
    surface_t *w3 = vds_create(200, 150, 4);
    surface_t *w4 = vds_create(100, 150, 4);
    // surface_t *w5 = vds_create(100, 75, 4);
    // surface_t *w6 = vds_create(50, 75, 4);
    // surface_t *w7 = vds_create(50, 37, 4);
    // surface_t *w8 = vds_create(25, 37, 4);
    // surface_t *w9 = vds_create(25, 18, 4);

    tty_t *tty = tty_create(w0, &font_6x10, colors_kora, 0);

    TTY("\e[38mKoraOS\e[0m - x86 - v1.0-a0ee43b+\n");
    TTY("Memory available 61.9 Mb over 63.9 Mb\n");
    TTY("\e[90m- Krn :: c0000000-c1000000 HEAP rw-p\e[0m\n");
    TTY("\e[31m#PF\e[0m - c0000000  HEAP rw-p\n");
    TTY("CPU Ticks: 100 Hz\n");
    TTY("Wake up CPU.1: GenuineIntel\n");
    TTY("BSP is CPU.0: GenuineIntel, found 2 CPUs\n");
    TTY("CPU features: SSE3, VMW, MMW, APIC\n");
    TTY("Startup: Thu Apr 19 2:24:46 2018\n");
    TTY("\n  \e[36mGreetings...\e[0m\n\n");
    TTY("\e[90m- Krn :: c11d6000-c11e5000 ANON rw-p\e[0m\n");
    TTY("\e[31m#PF\e[0m - c11d6000  ANON rw-p\n");
    TTY("Special device /dev/null <\e[33mnull\e[0m>\n");
    TTY("Special device /dev/zero <\e[33mzero\e[0m>\n");
    TTY("Special device /dev/ones <\e[33mones\e[0m>\n");
    TTY("Special device /dev/random <\e[33mrandom\e[0m>\n");
    TTY("\e[90m- Krn :: c1001000-c11d6000 PHYS rw-p\e[0m\n");
    TTY("VGA 800x600x24 <\e[33mscr\e[0m>\n");
    TTY("PS/2 Keyboard  <\e[33mkdb\e[0m>\nPS/2 Mouse  <\e[33mmouse\e[0m>\n");
    TTY("IDE ATA VBOX HARDDISK 4.00 Mb <\e[33msdA\e[0m>\nIDE ATA VBOX CD-ROM <\e[33msdC\e[0m>\n");
    // TTY("PCI.00.00 Intel corporation Host bridge\n");
    // TTY("PCI.00.01 Intel corporation ISA bridge\n");
    // TTY("PCI.00.02 InnoTek VGA-Compatible Controller\n");
    // TTY("PCI.00.03 Intel corporation Ethernet Controller\n");
    // TTY("PCI.00.04 InnoTek System Peripheral\n");
    // TTY("PCI.00.05 Intel corporation Audio Device\n");
    // TTY("PCI.00.06 Apple Inc. USB (Open Host)\n");
    // TTY("PCI.00.07 Intel corporation Bridge Device\n");
    TTY("\e[90m- Krn :: c1001000-c11d6000 PHYS rw-p\e[0m\n");
    TTY("E1000i Ethernet controller \e[92m08:00:27:AB:1D:DC\e[0m\n");
    TTY("\e[90m- Krn :: c1001000-c11d6000 BLOCK r--S\e[0m\n");
    TTY("\e[31m#PF\e[0m - c10d1000  BLOCK r--S\n");
    TTY("Logical partition MBR  896 Kb <\e[33msdA0\e[0m>\n");
    TTY("Logical partition MBR 31.0 Mb <\e[33msdA1\e[0m>\n");
    TTY("Logical partition MBR 31.8 Mb <\e[33msdA2\e[0m>\n");
    TTY("\e[90m- Krn :: c1001000-c11d6000 BLOCK r--S\e[0m\n");
    TTY("\e[31m#PF\e[0m - c10d2000  BLOCK r--S\n");
    TTY("\e[31m#PF\e[0m - c10d3000  BLOCK r--S\n");
    TTY("Mount sdC as \e[35mISOIMAGE\e[0m (isofs)\n");
    TTY("\e[90m- Krn :: c11ef000-c11f0000 FILE  r--S\e[0m\n");
    TTY("\e[31m#PF\e[0m - c10d3000  FILE r--S\n");
    TTY("\e[90m- Krn :: c11ef000-c11f0000 ANON  r--S\e[0m\n");
    TTY("\e[90m- Usr :: 08000000-08001000 FILE  r-xc\e[0m\n");
    TTY("\e[90m- Usr :: 08001000-08002000 FILE  rw-c\e[0m\n");
    TTY("\e[90m- Usr :: 00400000-00500000 STACK rw-p\e[0m\n");
    TTY("Create new task PID.1\n");
    TTY("\e[31m#PF\e[0m - c10d3000  FILE r-xc\n");
    TTY("\e[31m#PF\e[0m - c10d3000  STACK rw-p\n");
    TTY("Syscall PID.1 #5\n");
    TTY("\e[38mHello\e[0m\n");
    TTY("\e[96msyslogs(\"Hello\") = -1 [0]\e[0m\n");

    tty_scroll(tty, 4);
    tty_scroll(tty, -2);
    tty_paint(tty);

    tty_destroy(tty);
    tty = tty_create(w1, &font_8x15, colors_kora, 1);

    TTY("Krish - v1.0 \nKora system interactive shell\n\e[32mfabien\e[0m@\e[32mnuc\e[0m:\e[36m~\e[0m/> ");

    tty_destroy(tty);
    tty = tty_create(w2, &font_6x9, colors_std, 1);

    TTY("Kora Test\nVFS #1 - OK\nVFS #2 - OK\n");

    tty_destroy(tty);

    vds_fill(w3, 0xf2f2f2);
    vds_fill(w4, 0xa61010);

    vds_copy(screen, w0, 0, 0);
    vds_copy(screen, w1, 400, 0);
    vds_copy(screen, w2, 400, 300);
    vds_copy(screen, w3, 600, 300);
    vds_copy(screen, w4, 600, 450);

    vds_destroy(w0);
    vds_destroy(w1);
    vds_destroy(w2);
    vds_destroy(w3);
    vds_destroy(w4);

    export("screen.bmp", screen);
    vds_destroy(screen);
}


int main ()
{
    kprintf(-1, "Kora FILE check - " __ARCH " - v" _VTAG_ "\n");

    test_VDS();
    return 0;
}

