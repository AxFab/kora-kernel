#include <stdlib.h>
#include <fcntl.h>
#include <tchar.h>
#include <stdint.h>

#define PACK(decl) __pragma(pack(push,1)) decl __pragma(pack(pop))

#define ALIGN_UP(v,a)      (((v)+(a-1))&(~(a-1)))
#define ALIGN_DW(v,a)      ((v)&(~(a-1)))
#define IS_ALIGNED(v,a)      (((v)&((a)-1))==0)

#define ADDR_OFF(a,o)  ((void*)(((char*)a)+(o)))

#define MIN(a,b)    ((a)<=(b)?(a):(b))
#define MAX(a,b)    ((a)>=(b)?(a):(b))
#define MIN3(a,b,c)    MIN(a,MIN(b,c))
#define MAX3(a,b,c)    MAX(a,MAX(b,c))
#define POW2(v)     ((v) != 0 && ((v) & ((v)-1)) == 0)


typedef long off_t;
 /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef struct surface surface_t;
struct surface {
    unsigned width, height, pitch, format;
    void *planes[2];
 };

surface_t *surface_create(int w, int h);
void surface_fill(surface_t *sfc, uint32_t color);
void surface_export(surface_t *sfc, int no);

/* --------------------------------------------------------------------------------*/

surface_t *surface_create(int w, int h)
{
    surface_t *sfc = calloc(1, sizeof(surface_t));
    sfc->width = w;
    sfc->height = h;
    sfc->pitch = 4 * ALIGN_UP(w, 4);
    sfc->planes[0] = calloc(sfc->pitch, sfc->height);
    sfc->planes[1] = calloc(sfc->pitch, sfc->height);
    return sfc;
}

void surface_slide(surface_t *sfc, int height, uint32_t color)
{
    int px, py;
    if (height < 0) {
        height = -height;
        memcpy(sfc->planes[0], ADDR_OFF(sfc->planes[0], sfc->pitch * height), sfc->pitch * (sfc->height - height));
        for (py = sfc->height - height; py < sfc->height; ++py) {
            int pxrow = sfc->pitch * py;
            for (px = 0; px < sfc->width; ++px) {
                uint32_t *pixel = ADDR_OFF(sfc->planes[0], pxrow + px * 4);
                *pixel = color;
            }
        }
    }
}

void surface_fill(surface_t *sfc, uint32_t color)
{
    int px, py;
    for (py = 0; py < sfc->height; ++py) {
        int pxrow = sfc->pitch * py;
        for (px = 0; px < sfc->width; ++px) {
            uint32_t *pixel = ADDR_OFF(sfc->planes[0], pxrow + px * 4);
            *pixel = color;
        }
    }
}

void surface_flip(surface_t *sfc)
{
    static int no = 0;
    surface_export(sfc, ++no);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef struct font_bmp font_bmp_t;
struct font_bmp {
    uint8_t *glyphs;
    char glyph_size;
    char width, height, dispx, dispy;
};

void font_paint(surface_t *sfc, font_bmp_t *data, uint32_t unicode, uint32_t *color, int x, int y);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


typedef struct tty tty_t;
typedef struct tty_cell tty_cell_t;

tty_t *tty_create(surface_t *sfc, font_bmp_t *font);
void tty_window(tty_t *tty, surface_t *sfc, font_bmp_t *font);
void tty_paint(tty_t *tty);
void tty_paint_cell(tty_t *tty, tty_cell_t *cell, int *row, int *col);

#define TF_EOL 1
#define TF_EOF 2

/* --------------------------------------------------------------------------------*/

struct tty {
    surface_t *sfc;
    font_bmp_t *font;
    char w, h;
    int count, top, end;
    int smin, smax;
    uint32_t fg, bg;
    tty_cell_t *cells;
};

struct tty_cell {
    uint32_t fg, bg;
    int8_t row, col, len, sz;
    char str[50];
    uint8_t unused, flags;
};


tty_t *tty_create(int count)
{
    tty_t *tty = calloc(1, sizeof(tty_t));
    tty->count = count;
    tty->cells = calloc(count, sizeof(tty_cell_t));
    tty->cells->fg = 0xa6a6a6;
    tty->cells->bg = 0x121212;
    tty->fg = 0xa6a6a6;
    tty->bg = 0x121212;
    tty->cells->flags = TF_EOF;
    tty->w = 1024;
    tty->h = 1024;
    return tty;
}

void tty_window(tty_t *tty, surface_t *sfc, font_bmp_t *font)
{
    tty->sfc = sfc;
    tty->font = font;
    tty->w = sfc->width / font->dispx;
    tty->h = sfc->height / font->dispy;
    surface_fill(sfc, tty->cells->bg);
    tty_paint(tty);
}

void tty_paint_cell(tty_t *tty, tty_cell_t *cell, int *row, int *col)
{
    int i;
    char *str = cell->str;
    for (i = 0; i < cell->len; ++i) {
        if ((*col) >= tty->w) {
            *col = 0;
            (*row)++;
        }
        if ((*row) >= 0 && (*row) < tty->h && tty->sfc != NULL) {
            uint32_t unicode = *str;
            font_paint(tty->sfc, tty->font, unicode, &cell->fg, (*col) * tty->font->dispx, (*row) * tty->font->dispy);
        }
        str++;
        (*col)++;
    }
    if (cell->flags & TF_EOL) {
        if ((*row) >= 0 && (*row) < tty->h && tty->sfc != NULL) {
            while ((*col) < tty->w) {
                font_paint(tty->sfc, tty->font, ' ', &cell->fg, (*col) * tty->font->dispx, (*row) * tty->font->dispy);
                (*col)++;
            }
        }
        *col = 0;
        (*row)++;
    }
}


void tty_paint(tty_t *tty)
{
    int idx = tty->top;
    int col = 0;
    int row = tty->smax;
    for (; ;) {
        int r = row;
        int c = col;
        tty_paint_cell(tty, &tty->cells[idx], &row, &col);
        if (row >= tty->h && tty->sfc != NULL) { // Go to bottom 
            surface_slide(tty->sfc, tty->font->dispy * (tty->h - row - 1), tty->bg);
            idx--;
            row = r + (tty->h - row - 1);
            col = c;
        }
        if (tty->cells[idx].flags & TF_EOF) {
            if (row >= tty->h && tty->sfc != NULL)  // Go to bottom ?
                tty->smax += tty->h - row - 1;

            if (tty->sfc != NULL)
                surface_flip(tty->sfc);
            return;
        }
        idx = (idx + 1) % tty->count;
    }
}

tty_cell_t *tty_next_cell(tty_t *tty)
{
    tty_cell_t *cell = &tty->cells[tty->end];
    if (cell->sz == 0 && !(cell->flags & TF_EOL))
        return cell;
    int next = (tty->end + 1) % tty->count;
    if (tty->top == next)
        tty->top = (tty->top + 1) % tty->count;
    memset(&tty->cells[next], 0, sizeof(tty_cell_t));
    tty->cells[next].fg = tty->cells[tty->end].fg;
    tty->cells[next].bg = tty->cells[tty->end].bg;
    tty->cells[next].flags = TF_EOF;
    tty->cells[tty->end].flags &= ~TF_EOF;
    tty->end = next;
    return &tty->cells[next];
}

uint32_t consoleDarkColor[] = {
    0x121212, 0xa61010, 0x10a610, 0xa66010,
    0x1010a6, 0xa610a6, 0x10a6a6, 0xa6a6a6,
};
uint32_t consoleLightColor[] = {
    0x323232, 0xd01010, 0x10d010, 0xd0d010,
    0x1060d0, 0xd010d0, 0x10d0d0, 0xd0d0d0,
};


int tty_excape_apply_csi_m(tty_t* tty, int* val, int cnt)
{
    int i;
    tty_cell_t *cell = tty_next_cell(tty);
    for (i = 0; i < cnt; ++i) {
        if (val[i] == 0) { // Reset
            cell->fg = consoleDarkColor[7];
            cell->bg = consoleDarkColor[0];
        } else if (val[i] == 1) { // Bolder
        } else if (val[i] == 2) { // Lighter
        } else if (val[i] == 3) { // Italic
        } else if (val[i] == 4) { // Undeline
        } else if (val[i] == 7) { // Reverse video
            uint32_t tmp = cell->bg;
            cell->bg = cell->fg;
            cell->fg = tmp;
        } else if (val[i] == 8) { // Conceal on
        } else if (val[i] >= 10 && val[i] <= 19) { // Select font
        } else if (val[i] == 22) { // Normal weight
        } else if (val[i] == 23) { // Not italic
        } else if (val[i] == 24) { // No decoration
        } else if (val[i] == 28) { // Conceal off
        } else if (val[i] >= 30 && val[i] <= 37) { // Select foreground
            cell->fg = consoleDarkColor[val[i] - 30];
        } else if (val[i] == 38) {
            if (i + 2 < cnt && val[i+1] == 5) {
                cell->fg = consoleDarkColor[val[i+2] & 7];
                i += 2;
            } else if (i + 4 < cnt && val[i + 1] == 2) {
                cell->fg = ((val[i + 2] & 0xff) << 16) | ((val[i + 3] & 0xff) << 8) | (val[i + 4] & 0xff);
                i += 4;
            }
        } else if (val[i] == 39) {
            cell->fg = consoleDarkColor[7];
        } else if (val[i] >= 40 && val[i] <= 47) { // Select background
            cell->bg = consoleDarkColor[val[i] - 40];
        } else if (val[i] == 48) {
            if (i + 2 < cnt && val[i + 1] == 5) {
                cell->bg = consoleDarkColor[val[i + 2] & 7];
                i += 2;
            } else if (i + 4 < cnt && val[i + 1] == 2) {
                cell->bg = ((val[i + 2] & 0xff) << 16) | ((val[i + 3] & 0xff) << 8) | (val[i + 4] & 0xff);
                i += 4;
            }
        } else if (val[i] == 49) {
            cell->bg = consoleDarkColor[0];
        } else if (val[i] >= 90 && val[i] <= 97) { // Select bright foreground
            cell->fg = consoleLightColor[val[i] - 90];
        } else if (val[i] >= 100 && val[i] <= 107) { // Select bright background
            cell->bg = consoleLightColor[val[i] - 100];
        }
    }
}

int tty_excape_csi(tty_t* tty, const char *buf, int len)
{
    int val[10] = { 0 };
    int sp = 0;
    char *end = buf;
    int s = 2;
    len -= 2;
    while (len > 0 && sp < 10) {
        if (buf[s] >= '0' && buf[s] <= '9')
            val[sp] = val[sp] * 10 + buf[s] - '0';
        else if (buf[s] == ';')
            sp++;
        else
            break;
        s++;
        len--;
    }
    if (len == 0)
        return 1;
    switch (buf[s]) {
    case 'm':
        tty_excape_apply_csi_m(tty, val, sp + 1);
        break;
    default:
        return 1;
    }
    return s + 1;
}

int tty_escape(tty_t* tty, const char *buf, int len)
{
    if (len < 2)
        return -1;

    switch (buf[1]) {
    case '[': // CSI
        return tty_excape_csi(tty, buf, len);
    default:
        return 1;
    }
}

void tty_write(tty_t* tty, const char *buf, int len)
{
    tty_cell_t *cell = &tty->cells[tty->end];
    while (len > 0) {
        if (*buf >= 0x20 && *buf < 0x80) {
            if (cell->sz >= 50)
                cell = tty_next_cell(tty);
            cell->str[cell->sz] = *buf;
            cell->sz++;
            cell->len++;
            len--;
            buf++;
        } else if (*buf >= 0x80) {
            char s = *buf;
            int lg = 1;
            while (lg < 5 && (s >> (7 - lg))) 
                ++lg;
            if (lg < 2 || lg > 4) {
                len--;
                buf++;
                continue;
            }
            if (cell->sz + lg > 50)
                cell = tty_next_cell(tty);
            memcpy(&cell->str[cell->sz], buf, lg);
            cell->sz += lg;
            cell->len++;
            len -= lg;
            buf += lg;
        } else if (*buf == '\n') {
            cell->flags |= TF_EOL;
            cell = tty_next_cell(tty);
            len--;
            buf++;
        } else if (*buf == '\r') {
            cell->flags |= TF_EOL;
            cell = tty_next_cell(tty);
            cell->row = -1;
            len--;
            buf++;
        } else if (*buf == '\033') {
            int lg = tty_escape(tty, buf, len);
            cell = &tty->cells[tty->end];
            len -= lg;
            buf += lg;
        } else {
            len--;
            buf++;
        }
    }

    tty_paint(tty);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef struct bmp_file_header bmp_file_header_t;
typedef struct bmp_imag_header bmp_imag_header_t;

PACK(struct bmp_file_header {
    uint16_t type;
    uint32_t size;
    uint32_t reserved;
    uint32_t offset_bits;
});

PACK(struct bmp_imag_header {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bit_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    uint32_t pixel_per_meter_x;
    uint32_t pixel_per_meter_y;
    uint32_t color_used;
    uint32_t color_important;
});

void surface_export(surface_t *sfc, int no)
{
    char name[16];
    sprintf_s(name, 16, "exp%d.bmp", no);
    int fd = open(name, O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, 0666);
    bmp_file_header_t fh;
    bmp_imag_header_t fi;
    int pitch = ALIGN_UP(sfc->width * 3, 4);
    memset(&fh, 0, sizeof(fh));
    memset(&fi, 0, sizeof(fi));
    fh.type = 0x4D42;
    fh.size = pitch * sfc->height + sizeof(fh) + sizeof(fi);
    fh.offset_bits = sizeof(fh) + sizeof(fi);
    fi.size = 40;
    fi.width = sfc->width;
    fi.height = sfc->height;
    fi.planes = 1;
    fi.bit_per_pixel = 24;
    fi.image_size = pitch * sfc->height;
    fi.pixel_per_meter_x = 960000 / 254;
    fi.pixel_per_meter_y = 960000 / 254;
    uint8_t *line = malloc(pitch);
    write(fd, &fh, sizeof(fh));
    write(fd, &fi, sizeof(fi));
    for (int i = sfc->height; i-- > 0; ) {
        uint32_t *dest = (uint32_t*)line;
        uint32_t *src = (uint32_t*)ADDR_OFF(sfc->planes[0], sfc->pitch * i);
        for (int i = 0; i < sfc->width; ++i) {
            *dest = *src;
            dest = ADDR_OFF(dest, 3);
            src = ADDR_OFF(src, 4);
        }
        write(fd, line, pitch);
    }
    close(fd);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
#define _BUF(n) n, sizeof(n)


extern const font_bmp_t font_8x8;
extern const font_bmp_t font_6x9;
extern const font_bmp_t font_6x10;
extern const font_bmp_t font_7x13;
extern const font_bmp_t font_8x15;


int main()
{
    surface_t *sfc = surface_create(400, 300);
    tty_t *tty = tty_create(64);

    tty_write(tty, _BUF("\033[96mKoraOS v0.0\033[0m x86"));
    tty_write(tty, _BUF(" -- build 2e367b2+ at Oct 13 2018\n\n"));
    tty_write(tty, _BUF("Memory: \033[92m63.9\033[0mMib\n"));
    tty_write(tty, _BUF("Virtual memory intialized\n"));
    tty_write(tty, _BUF("CPU.0 GenuineIntel\n"));
    tty_write(tty, _BUF("APIC intialized\n"));
    tty_write(tty, _BUF("Set periodic ticks: \033[92m100\033[0mHz\n"));
    tty_write(tty, _BUF("CPU.1 GenuineIntel\n"));
    tty_write(tty, _BUF("Found \033[92m2\033[0m CPUs\n"));
    tty_window(tty, sfc, &font_7x13);
    tty_write(tty, _BUF("\n\033[94mGreetings...\033[0m\n\n"));
    tty_write(tty, _BUF("Load module: \033[30;43mx86.miniboot.tar.gz\033[0m 16K\n"));
    tty_write(tty, _BUF("PCI.00.00 :: Intel corporation - Host bridge\n"));
    tty_write(tty, _BUF("PCI.00.01 :: Intel corporation - ISA bridge\n"));
    tty_write(tty, _BUF("PCI.00.02 :: InnoTek - VGA-Compatible controller\n"));
    tty_write(tty, _BUF("PCI.00.03 :: Intel corporation - Ethernet controller\n"));
    tty_write(tty, _BUF("PCI.00.04 :: InnoTek - System peripheral\n"));
    tty_write(tty, _BUF("PCI.00.05 :: Intel corporation - Audio Device\n"));
    tty_write(tty, _BUF("PCI.00.06 :: Apple Inc. - USB (Open Host)\n"));
    tty_write(tty, _BUF("PCI.00.07 :: Intel corporation - Bridge device\n"));
    tty_write(tty, _BUF("Time: Sat Oct  13 13:34:49 2018\n"));
    tty_write(tty, _BUF("Register \033[31mIRQ2\033[0m\n"));
    tty_write(tty, _BUF("Start task 'master' PID.1\n"));
    tty_write(tty, _BUF("Register \033[31mIRQ14\033[0m\n"));
    tty_write(tty, _BUF("Register \033[31mIRQ15\033[0m\n"));
    tty_write(tty, _BUF("IDE ATA  Virtual HDD  <\033[33msdA\033[0m>\n"));
    tty_write(tty, _BUF("IDE ATAPI  Virtual CDROM  <\033[33msdC\033[0m>\n"));
    tty_write(tty, _BUF("Register \033[31mIRQ9\033[0m\n"));
    tty_write(tty, _BUF("Register \033[31mIRQ1\033[0m\n"));
    tty_write(tty, _BUF("Register \033[31mIRQ12\033[0m\n"));
    tty_write(tty, _BUF("Keyboard  PS/2   <\033[93mkdb0\033[0m>\n"));
    tty_write(tty, _BUF("Mouse  PS/2   <\033[93mmse0\033[0m>\n"));
    tty_write(tty, _BUF("Mount sdC as \033[35mKoraOSLive\033[0m (isofs)\n"));
    tty_write(tty, _BUF("VGA Screen  VirtualBox VGA  <\033[36mscr0\033[0m>\n"));
    tty_write(tty, _BUF("Network interface (eth0) - MAC: \033[92m08:00:27:AB:1D:DC\033[0m\n"));
    tty_write(tty, _BUF("Mount sdA as \033[35mHomeDrive\033[0m (fat16)\n"));
    tty_write(tty, _BUF("Start task 'network eth0' PID.2\n"));
    tty_write(tty, _BUF("Start task 'desktop 0' PID.3\n"));
    tty_write(tty, _BUF("Start task 'syslog tty' PID.4\n"));
    tty_write(tty, _BUF("IP4 configured (eth0) - \033[92m192.168.0.19\033[0m/16\n"));
    tty_write(tty, _BUF("Start task 'krish' PID.5\n"));
}