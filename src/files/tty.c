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
#include <string.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


typedef struct tty_cell tty_cell_t;

void tty_window(tty_t *tty, surface_t *sfc, font_bmp_t *font);
void tty_paint(tty_t *tty);
void tty_paint_cell(tty_t *tty, tty_cell_t *cell, int *row, int *col);

#define TF_EOL 1
#define TF_EOF 2

/* --------------------------------------------------------------------------------*/

struct tty {
    surface_t *sfc;
    font_bmp_t *font;
    short w, h;
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
    tty_t *tty = kalloc(sizeof(tty_t));
    tty->count = count;
    tty->cells = kalloc(count * sizeof(tty_cell_t));
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
    vds_fill(sfc, tty->cells->bg);
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
            vds_slide(tty->sfc, tty->font->dispy * (tty->h - row - 1), tty->bg);
            idx--;
            row = r + (tty->h - row - 1);
            col = c;
        }
        if (tty->cells[idx].flags & TF_EOF) {
            if (row >= tty->h && tty->sfc != NULL)  // Go to bottom ?
                tty->smax += tty->h - row - 1;

            if (tty->sfc != NULL)
                vds_flip(tty->sfc);
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


void tty_excape_apply_csi_m(tty_t *tty, int *val, int cnt)
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
}

int tty_excape_csi(tty_t *tty, const char *buf, int len)
{
    int val[10] = { 0 };
    int sp = 0;
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

int tty_escape(tty_t *tty, const char *buf, int len)
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

void tty_write(tty_t *tty, const char *buf, int len)
{
    tty_cell_t *cell = &tty->cells[tty->end];
    while (len > 0) {
        if (*buf >= 0x20) {
            if (cell->sz >= 50)
                cell = tty_next_cell(tty);
            cell->str[cell->sz] = *buf;
            cell->sz++;
            cell->len++;
            len--;
            buf++;
        } else if (*buf < 0) {
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
