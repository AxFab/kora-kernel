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
#include <kernel/files.h>
#include <kernel/input.h>
#include <string.h>

#define TTY_BUF_SIZE (64 - 3 * 4)


typedef struct tty_cell tty_cell_t;

struct tty_cell {
    uint32_t fg, bg;
    int8_t row, col;
    unsigned len, sz;
    char str[TTY_BUF_SIZE];
};

struct tty {
    tty_cell_t prompt;
    tty_cell_t *cells;
    int end, count;
    int rows, cols, scroll;
    inode_t *win;
    framebuffer_t *fb;
    const font_bmp_t *font;
    pipe_t *pipe;
};

extern const font_bmp_t font_6x10;
extern const font_bmp_t font_8x15;
extern const font_bmp_t font_7x13;
extern const font_bmp_t font_6x9;
extern const font_bmp_t font_8x8;


uint32_t colors_std[] = {
    0x000000, 0x800000, 0x008000, 0x808000,
    0x000080, 0x800080, 0x008080, 0x808080,
    0xD0D0D0, 0xFF0000, 0x00FF00, 0xFFFF00,
    0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF,
};

uint32_t colors_kora[] = {
    0x181818, 0xA61010, 0x10A610, 0xA66010,
    0x1010A6, 0xA610A6, 0x10A6A6, 0xA6A6A6,
    0x323232, 0xD01010, 0x10D010, 0xD06010,
    0x1010D0, 0xD010D0, 0x10D0D0, 0xD0D0D0,
};


uint32_t consoleDarkColor[] = {
    0x121212, 0xa61010, 0x10a610, 0xa66010,
    0x1010a6, 0xa610a6, 0x10a6a6, 0xa6a6a6,
};
uint32_t consoleLightColor[] = {
    0x323232, 0xd01010, 0x10d010, 0xd0d010,
    0x1060d0, 0xd010d0, 0x10d0d0, 0xd0d0d0,
};

tty_cell_t *tty_next(tty_t *tty);
void tty_repaint_all(tty_t *tty);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

tty_t *tty_create(int size)
{
    tty_t *tty = kalloc(sizeof(tty_t));
    tty->end = 0;
    int sz = ALIGN_UP(size * sizeof(tty_cell_t), PAGE_SIZE);
    tty->count = sz / sizeof(tty_cell_t);
    tty->cells = kmap(sz, NULL, 0, VMA_ANON_RW | VMA_RESOLVE);
    tty->cells[0].fg = 0xf2f2f2;
    tty->cells[0].bg = 0x181818;
    tty->prompt.fg = 0xf2f2f2;
    tty->prompt.bg = 0x181818;
    tty->rows = 3;
    tty->cols = 4096;
    tty->pipe = pipe_create();
    return tty;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static tty_cell_t *tty_apply_csi_m(tty_t *tty, int *val, int cnt)
{
    int i;
    tty_cell_t *cell = tty_next(tty);
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
        } else if (val[i] >= 30 && val[i] <= 37)   // Select foreground
            cell->fg = consoleDarkColor[val[i] - 30];
        else if (val[i] == 38) {
            if (i + 2 < cnt && val[i + 1] == 5) {
                cell->fg = consoleDarkColor[val[i + 2] & 7];
                i += 2;
            } else if (i + 4 < cnt && val[i + 1] == 2) {
                cell->fg = ((val[i + 2] & 0xff) << 16) | ((val[i + 3] & 0xff) << 8) | (val[i + 4] & 0xff);
                i += 4;
            }
        } else if (val[i] == 39)
            cell->fg = consoleDarkColor[7];
        else if (val[i] >= 40 && val[i] <= 47)   // Select background
            cell->bg = consoleDarkColor[val[i] - 40];
        else if (val[i] == 48) {
            if (i + 2 < cnt && val[i + 1] == 5) {
                cell->bg = consoleDarkColor[val[i + 2] & 7];
                i += 2;
            } else if (i + 4 < cnt && val[i + 1] == 2) {
                cell->bg = ((val[i + 2] & 0xff) << 16) | ((val[i + 3] & 0xff) << 8) | (val[i + 4] & 0xff);
                i += 4;
            }
        } else if (val[i] == 49)
            cell->bg = consoleDarkColor[0];
        else if (val[i] >= 90 && val[i] <= 97)   // Select bright foreground
            cell->fg = consoleLightColor[val[i] - 90];
        else if (val[i] >= 100 && val[i] <= 107)   // Select bright background
            cell->bg = consoleLightColor[val[i] - 100];
    }
    return cell;
}


static tty_cell_t *tty_command_csi(tty_t *tty, const char *buf, int *plen)
{
    int val[10] = { 0 };
    int sp = 0;
    int s = 1;
    int len = *plen - 1;
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
        return tty_next(tty);
    switch (buf[s]) {
    case 'm':
        *plen -= s + 1;
        return tty_apply_csi_m(tty, val, sp + 1);
    default:
        return &tty->cells[tty->end];
    }
}


tty_cell_t *tty_command(tty_t *tty, const char *buf, int *plen)
{
    if (*plen < 2)
        return &tty->cells[tty->end];
    switch (*buf) {
    case '[': // CSI
        return tty_command_csi(tty, buf, plen);
    default:
        return &tty->cells[tty->end];
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void tty_clear_row(framebuffer_t *fb, int row, const font_bmp_t *font, uint32_t color)
{
    gfx_rect(fb, 0, row * font->dispy, fb->width, font->dispy, color);
}

void tty_paint_cell(tty_t *tty, tty_cell_t *cell)
{
    if (tty->fb == NULL)
        return;
    if ((cell->row - tty->scroll) < 0 || (cell->row - tty->scroll) > tty->rows)
        return;
    if ((cell->row - tty->scroll) == tty->rows) {
        tty->scroll++;
        // gfx_slide(tty->fb, -tty->font->dispy, 0x181818);
        // gfx_clear(tty->fb, 0x181818);
        // tty_repaint_all(tty);
        return;
    }
    const font_bmp_t *font = tty->font;
    int c = cell->col;
    int x = cell->col * font->dispx;
    int y = (cell->row - tty->scroll) * font->dispy;
    framebuffer_t *fb = tty->fb;
    unsigned i = 0;
    while (i < cell->sz) {
        int unicode = cell->str[i];
        i++; // TODO - Unicode characters
        if (unicode > 0x20 && unicode < 0x80)
            font_paint(fb, font, unicode, &cell->fg, x, y);
        x += font->dispx;
        if (++c >= tty->cols)
            break;
    }
}


void tty_repaint_all(tty_t *tty)
{
    if (tty->fb == NULL)
        return;
    int i;
    for (i = 0; i < tty->end; ++i)
        tty_paint_cell(tty, &tty->cells[i]);
    tty->win->ops->flip(tty->win);
}

void tty_window(tty_t *tty, inode_t *win, const font_bmp_t *font)
{
    tty->win = win;
    tty->font = font;
    tty->fb = ((window_t *)win->info)->frame;
    gfx_clear(tty->fb, 0x181818);
    tty->rows = tty->fb->height / font->dispy;
    tty->cols = tty->fb->width / font->dispx;
    kprintf(-1, "Tty attached to a window: %dx%d\n", tty->cols, tty->rows);
    tty_repaint_all(tty);
}


tty_cell_t *tty_next(tty_t *tty)
{
    int next_idx = (tty->end + 1) % tty->count;
    tty_cell_t *cell = &tty->cells[tty->end];
    tty_cell_t *next = &tty->cells[next_idx];
    // TODO - If line is on screen, clear row
    next->row = cell->row;
    next->col = cell->col + cell->len;
    next->sz = 0;
    next->len = 0;
    next->fg = cell->fg;
    next->bg = cell->bg;
    if (tty->fb)
        tty_paint_cell(tty, cell);
    tty->end = next_idx;
    return next;
}

tty_cell_t *tty_putchar(tty_t *tty, tty_cell_t *cell, int unicode)
{
    if (cell->sz >= TTY_BUF_SIZE)
        cell = tty_next(tty);
    if (cell->len + cell->col >= (unsigned)tty->cols) {
        cell = tty_next(tty);
        cell->row++;
        cell->col = 0;
    }
    cell->str[cell->sz] = unicode & 0x7F;
    cell->len++;
    cell->sz++;
    return cell;
}

int tty_write(tty_t *tty, const char *buf, int len)
{
    if (len == 0)
        return 0;
    tty_cell_t *cell = &tty->cells[tty->end];
    while (len > 0) {
        int unicode = *buf;
        len--;
        buf++;
        // TODO - Unicode

        if (unicode <= 0)
            continue;

        if (unicode < 0x20) {
            if (unicode == '\n' || unicode == '\r') {
                cell = tty_next(tty);
                cell->row++;
                cell->col = 0;
            } else if (unicode == '\t') {
                cell = tty_next(tty);
                cell->col = ALIGN_UP(cell->col + 1, 8);
            } else if (unicode == '\033') {
                // Command -- Must be send in one block!
                int sv = len;
                cell = tty_command(tty, buf, &sv);
                buf += len - sv;
                len = sv;
            }
            continue;
        }

        cell = tty_putchar(tty, cell, unicode);
    }

    if (tty->fb) {
        tty_paint_cell(tty, cell);
        tty->win->ops->flip(tty->win);
    }
    return 0;
}


void tty_resize(tty_t *tty, int width, int height)
{
    if (tty->win == NULL)
        return;
    tty->win->ops->resize(tty->win, width, height);
    gfx_clear(tty->fb, 0x181818);
    tty->rows = tty->fb->height / tty->font->dispy;
    tty->cols = tty->fb->width / tty->font->dispx;
    kprintf(-1, "Tty window resize %dx%d \n", tty->cols, tty->rows);
    tty_repaint_all(tty);
}


int tty_puts(tty_t *tty, const char *buf)
{
    return tty_write(tty, buf, strlen(buf));
}

void tty_input(tty_t *tty, int unicode)
{
    if (unicode >= 0x20 && unicode < 0x7F) {
        tty->prompt.str[tty->prompt.len] = '\0';
        strcat(tty->prompt.str, (char *)&unicode);
        tty->prompt.len = strlen(tty->prompt.str);
        tty->prompt.sz = tty->prompt.len;
    } else if (unicode == 8 && tty->prompt.len > 0) {
        tty->prompt.len--;
        tty->prompt.sz = tty->prompt.len;
        if (tty->fb)
            tty_clear_row(tty->fb, tty->prompt.row, tty->font, tty->prompt.bg);
    } else if (unicode == 10) {
        if (tty->fb) {
            tty_clear_row(tty->fb, tty->prompt.row, tty->font, tty->prompt.bg);
            tty->prompt.str[tty->prompt.len] = '\n';
            tty_write(tty, tty->prompt.str, tty->prompt.sz + 1);
        }
        pipe_write(tty->pipe, tty->prompt.str, tty->prompt.sz + 1, 0);
        tty->prompt.sz = 0;
        tty->prompt.len = 0;
        return;
    }
    tty->prompt.row = tty->cells[tty->end].row + 1;
    tty->prompt.col = 0;
    if (tty->fb) {
        tty_paint_cell(tty, &tty->prompt);
        tty->win->ops->flip(tty->win);
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int tty_ino_read(inode_t *ino, char *buf, size_t len, int flags) 
{
	return pipe_read(((tty_t*) ino->info) ->pipe, buf, len, flags);
} 

int tty_ino_write(inode_t *ino, const char *buf, size_t len, int flags) 
{
	return tty_write((tty_t*)ino->info, buf, len);
} 

ino_ops_t tty_ino_ops = {
	.read = tty_ino_read,
	.write = tty_ino_write, 
};

inode_t *tty_inode()
{
	tty_t *tty = tty_create(256);
	inode_t *ino = vfs_inode(1, FL_TTY, NULL);
	ino->ops = &tty_ino_ops;
	ino->info = tty;
	return ino;
}