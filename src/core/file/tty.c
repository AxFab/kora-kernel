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
#include <string.h>

struct line {
    uint32_t fx_color;
    uint32_t bg_color;
    uint32_t fx_color_s;
    uint32_t bg_color_s;
    int row;
    int col;
    int len;
    char *str;
    line_t *prev;
    line_t *next;
};

struct tty {
    surface_t *win;
    const font_t *font;
    int row;
    int line_max;
    int row_max;
    int col_max;
    line_t *first;
    line_t *top;
    line_t *last;
    uint32_t fx_color;
    uint32_t bg_color;
    const uint32_t *colors;
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Paint a glyph into the window attached to the TTY */
static void tty_glyph(tty_t *tty, line_t *line, int col, int ch)
{
    int i, j = 0, l = 0, dp = tty->win->bits;
    uint8_t *px_row;
    int row = line->row - tty->row;
    if (row < 0 || row > tty->row_max || col > tty->col_max)
        return;
    const uint8_t *glyph = &tty->font->glyph[(ch - 0x20) * tty->font->glyph_size];
    int x = col * tty->font->dispx;
    int y = row * tty->font->dispy;
    if (ch >= 0x20 && ch < 0x80) {
        for (; j < tty->font->height; ++j) {
            px_row = &tty->win->pixels[(j + y) * tty->win->pitch];
            for (i = 0; i < tty->font->width; ++i, ++l) {
                if (glyph[l / 8] & (1 << l % 8)) {
                    px_row[(i + x) * dp + 0] = line->fx_color & 0xFF;
                    px_row[(i + x) * dp + 1] = (line->fx_color >> 8) & 0xFF;
                    px_row[(i + x) * dp + 2] = (line->fx_color >> 16) & 0xFF;
                } else {
                    px_row[(i + x) * dp + 0] = line->bg_color & 0xFF;
                    px_row[(i + x) * dp + 1] = (line->bg_color >> 8) & 0xFF;
                    px_row[(i + x) * dp + 2] = (line->bg_color >> 16) & 0xFF;
                }
            }
            for (; i < tty->font->dispx; ++i) {
                px_row[(i + x) * dp + 0] = line->bg_color & 0xFF;
                px_row[(i + x) * dp + 1] = (line->bg_color >> 8) & 0xFF;
                px_row[(i + x) * dp + 2] = (line->bg_color >> 16) & 0xFF;
            }
        }
    }
    for (; j < tty->font->dispy; ++j) {
        px_row = &tty->win->pixels[(j + y) * tty->win->pitch];
        for (i = 0; i < tty->font->dispx; ++i) {
            px_row[(i + x) * dp + 0] = line->bg_color & 0xFF;
            px_row[(i + x) * dp + 1] = (line->bg_color >> 8) & 0xFF;
            px_row[(i + x) * dp + 2] = (line->bg_color >> 16) & 0xFF;
        }
    }
}

/* Change TTY color at the specified line */
static void tty_chcolor(tty_t *tty, line_t *line, int val)
{
    if (val == 0) {
        line->fx_color = tty->fx_color;
        line->bg_color = tty->bg_color;
    } else if (val < 30) {
    } else if (val < 40) {
        line->fx_color = tty->colors[val - 30];
    } else if (val < 50) {
        line->bg_color = tty->colors[val - 40];
    } else if (val < 90) {
    } else if (val < 100) {
        line->fx_color = tty->colors[val - 90 + 9];
    }
}

/* Parse an ANSI escape command */
static void tty_command(tty_t *tty, line_t *line, const char **str)
{
    int i = 0, val[5];
    if (**str != '[')
        return;
    do {
        (*str)++;
        val[i++] = strtoul(*str, (char**)str, 10);
    } while (i < 5 && **str == ';');

    switch (**str) {
    case 'm':
        while (i > 0)
            tty_chcolor(tty, line, val[--i]);
        break;
    }
}

/* Parse an ANSI escape command */
static void tty_paint_line(tty_t *tty, line_t *line, const char *str)
{
    int x;
    line->fx_color_s = line->fx_color;
    line->bg_color_s = line->bg_color;
    const char *start = str;
    for (x = 0; str - start < line->len; ++str) {
        if (*str == '\e') {
            ++str;
            tty_command(tty, line, &str);
        } else if (*str >= 0x20) {
            tty_glyph(tty, line, x++, *str);
        }
    }
    if (line->next) {
        line->next->fx_color = line->fx_color;
        line->next->bg_color = line->bg_color;
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Re-paint the all TTY */
void tty_paint(tty_t *tty)
{
    if (tty->win == NULL)
        return;
    vds_fill(tty->win, tty->top->bg_color);

    line_t *line = tty->top;
    for (; line; line = line->next) {
        if (line->row - tty->row >= tty->row_max)
            break;
        tty_paint_line(tty, line, line->str);
    }
}

void tty_scroll(tty_t *tty, int count)
{
    while (count > 0) {
        if (tty->top->next == NULL)
            break;
        tty->top = tty->top->next;
        count--;
        tty->row++;
    }
    while (count < 0) {
        if (tty->top->prev == NULL)
            break;
        tty->top = tty->top->prev;
        count++;
        tty->row--;
    }
}

void tty_write(tty_t *tty, const char *str, int len)
{
    while (len > 0) {
        line_t *line = tty->last;
        int x = line->col;
        const char *start = str;
        for (; str - start < len; ++str) {
            if (*str == '\e') {
                ++str;
                tty_command(tty, line, &str);
            } else if (*str == '\n') {
                break;
            } else if (*str >= 0x20) {
                ++x;
            }
            if (x >= tty->col_max) {
                ++str;
                break;
            }
        }
        int sz = line->len + str - start;
        char *buf = kalloc(sz);
        memcpy(buf, line->str, line->len);
        memcpy(&buf[line->len], start, str - start);
        kfree(line->str);
        line->len += str - start;
        line->str = buf;
        line->col = x;
        len -= str - start;
        if (len > 0 && *str == '\n') {
            ++str;
            --len;
            tty->last->next = kalloc(sizeof(line_t));
            tty->last->next->fx_color = tty->last->fx_color;
            tty->last->next->bg_color = tty->last->bg_color;
            tty->last->next->row = tty->last->row + 1;
            tty->last->next->str = kalloc(1);
            tty->last->next->prev = tty->last;
            tty->last = tty->last->next;
        }
    }

    // TODO -- auto-scroll
    tty_paint(tty);
}

// void tty_event(tty_t *tty, event_t *event)
// {
//     // TODO Take only KEYBOARD PRESS !
// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void tty_attach(tty_t *tty, surface_t *win, const font_t *font, const uint32_t *colors, int iv)
{
    tty->win = win;
    tty->font = font;
    tty->colors = colors;
    tty->fx_color = iv ? colors[0] : colors[7];
    tty->bg_color = iv ? colors[7] : colors[0];
    tty->row_max = win ? win->height / font->dispy : 80;
    tty->col_max = win ? win->width / font->dispx : 50;
}

tty_t *tty_create(surface_t *win, const font_t *font, const uint32_t *colors, int iv)
{
    tty_t *tty = (tty_t*)kalloc(sizeof(tty_t));
    tty_attach(tty, win, font, colors, iv);
    tty->top = kalloc(sizeof(line_t));
    tty->first = tty->top;
    tty->top->fx_color = tty->fx_color;
    tty->top->bg_color = tty->bg_color;
    tty->top->row = 0;
    tty->top->str = kalloc(1);
    tty->last = tty->top;
    tty_paint(tty);
    return tty;
}

void tty_destroy(tty_t *tty)
{
    while (tty->first) {
        line_t *line = tty->first;
        tty->first = tty->first->next;
        kfree(line->str);
        kfree(line);
    }
    kfree(tty);
}

