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
#include <kernel/core.h>
#include <kernel/sys/inode.h>
#include <kernel/sys/window.h>
#include <kora/mcrs.h>

typedef struct tm_line tm_line_t;

struct tm_line {
    long offset;
    uint32_t tx_color;
    uint32_t bg_color;
    tm_line_t *prev;
    tm_line_t *next;
};

struct terminal {
    fifo_t *out;
    fifo_t *in;
    tm_line_t *top;
    tm_line_t *last;
    int row, col, max_row, max_col;
    uint32_t tx_color;
    uint32_t bg_color;
    char *base;
    surface_t *surface;
};

uint32_t consoleColor[] = {
    0xff181818, 0xffa61010, 0xff10a610, 0xffa66010,
    0xff1010a6, 0xffa610a6, 0xff10a6a6, 0xffa6a6a6,
    0xffffffff
};
uint32_t consoleSecColor[] = {
    0xff323232, 0xffd01010, 0xff10d010, 0xffd0d010,
    0xff1060d0, 0xffd010d0, 0xff10d0d0, 0xffd0d0d0,
    0xffffffff
};
uint32_t consoleBgColor[] = {
    0xff181818, 0xffa61010, 0xff10a610, 0xffa66010,
    0xff1010a6, 0xffa610a6, 0xff10a6a6, 0xffa6a6a6,
    0xffffffff
};

terminal_t *term_create(surface_t *surface)
{
    terminal_t *term = (terminal_t *)kalloc(sizeof(terminal_t));
    size_t size = 64 * _Kib_;
    term->base = (char *)kmap(size, NULL, 0, VMA_FG_FIFO | VMA_RESOLVE);
    term->out = fifo_create(term->base, size);
    term->in = fifo_create(kmap(4 * _Kib_, NULL, 0, VMA_FG_FIFO | VMA_RESOLVE),
                           4 * _Kib_);

    term->tx_color = 0xffa6a6a6;
    term->bg_color = 0xff181818;
    term->row = 1;

    tm_line_t *line = (tm_line_t *)kalloc(sizeof(tm_line_t));
    line->tx_color = 0xffa6a6a6;
    line->bg_color = 0xff181818;
    term->last = line;
    term->top = line;
    term->surface = surface;

    term->max_col = 80;
    term->max_row = 25;

    return term;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void term_change_color(terminal_t *term, tm_line_t *line, int value)
{
    if (value == 0) {
        line->tx_color = term->tx_color;
        line->bg_color = term->bg_color;
    } else if (value < 30) {
    } else if (value < 40)
        line->tx_color = consoleColor[value - 30];

    else if (value < 50)
        line->bg_color = consoleBgColor[value - 40];

    else if (value < 90) {
    } else if (value < 100)
        line->tx_color = consoleSecColor[value - 90];

    else {
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static int term_readchar(terminal_t *term, tm_line_t *line, const char **str)
{
    if (**str < 0)
        return 0;
    else if (**str >= 0x20)
        return *(*str)++;
    else if (**str == '\033' || **str == '\0')
        return *(*str)++;
    (*str)++;
    return 0x7F;
}

static void term_readcmd(terminal_t *term, tm_line_t *line, const char **str)
{
    int i = 0, val[5];
    if (**str != '[')
        return;
    do {
        (*str)++;
        val[i++] = (int)strtoul(*str, (char **)str, 10);
    } while (i < 5 && **str == ';');

    switch (**str) {
    case 'm':
        while (i > 0)
            term_change_color(term, line, val[--i]);
        break;
    }

    (*str)++;
}

static int term_paint(terminal_t *term, tm_line_t *line, int row)
{
    int ch, col = 0;
    const char *str = &term->base[line->offset];

    while (*str) {
        ch = term_readchar(term, line, &str);
        if (ch == '\0')
            return -1;
        else if (ch == '\033')
            term_readcmd(term, line, &str);

        // TODO -- PAINT
        if (col >= term->max_col)
            return str - term->base;
        // if (line->offset + col >= (int)term->wpen)
        //     return 0;
    }
    return 0;
}

static void term_redraw(terminal_t *term)
{
    int i, pen;
    // TODO - CLEAN SCREEN !?
    tm_line_t *line = term->top;
    for (i = 1; i <= term->max_row; ++i) {
        tm_line_t copy = *line;
        pen = term_paint(term, &copy, i);
        if (pen == 0)
            break;
        line = line->next;
    }
}

static void term_scroll(terminal_t *term, int count)
{
    while (count > 0) {
        if (term->top->next == NULL)
            break;
        term->top = term->top->next;
        count--;
        term->row--;
    }

    while (count < 0) {
        if (term->top->prev == NULL)
            break;
        term->top = term->top->prev;
        count++;
        term->row++;
    }

    term_redraw(term);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int term_write(terminal_t *term, const char *buffer, int length)
{
    fifo_in(term->out, buffer, length, 0);
    for (;;) {
        if (term->row > term->max_row)
            term_scroll(term, term->row - term->max_row);

        int pen = term_paint(term, term->last, term->row);
        if (pen == 0)
            break;
        tm_line_t *line = (tm_line_t *)kalloc(sizeof(tm_line_t));
        line->offset = pen;
        line->tx_color = term->last->tx_color;
        line->bg_color = term->last->bg_color;
        // line->flags = term->last->flags
        term->last->next = line;
        line->prev = term->last;
        term->last = line;
        term->row++;
    }
}
