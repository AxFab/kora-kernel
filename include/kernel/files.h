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
#ifndef _KERNEL_FILES_H
#define _KERNEL_FILES_H 1

#include <kernel/core.h>
#include <kora/llist.h>



typedef struct surface surface_t;
typedef struct line line_t;
typedef struct tty tty_t;
typedef struct bitmap_font font_t;
typedef struct desktop desktop_t;
typedef struct display display_t;
typedef struct pipe pipe_t;

struct surface {
    int width;
    int height;
    int pitch;
    int depth;
    uint8_t *pixels;
    uint8_t *backup;
    llnode_t node;
    void (*flip)(surface_t *screen);
};

struct bitmap_font {
    uint8_t width;
    uint8_t height;
    uint8_t dispx;
    uint8_t dispy;
    uint8_t glyph_size;
    const uint8_t *glyph;
};


void tty_write(tty_t *tty, const char *str, int lg);
void tty_attach(tty_t *tty, surface_t *win, const font_t *font, const uint32_t *colors, int iv);
tty_t *tty_create(surface_t *win, const font_t *font, const uint32_t *colors, int iv);
void tty_destroy(tty_t *tty);
// void tty_paint(tty_t *tty);
// void tty_scroll(tty_t *tty, int count);


void vds_fill(surface_t *win, uint32_t color);
void vds_copy(surface_t *dest, surface_t *src, int x, int y);
surface_t *vds_create_empty(int width, int height, int depth);
surface_t *vds_create(int width, int height, int depth);
void vds_destroy(surface_t *srf);
void vds_mouse(surface_t *scr, int x, int y);


desktop_t *wmgr_desktop();
void wmgr_register_screen(surface_t *screen);
surface_t *wmgr_window(desktop_t *desktop, int width, int height);




pipe_t *pipe_create();
void pipe_destroy(pipe_t *pipe);
int pipe_resize(pipe_t *pipe, int size);
int pipe_erase(pipe_t *pipe, int len);
int pipe_write(pipe_t *pipe, const char *buf, int len, int flags);
int pipe_read(pipe_t *pipe, char *buf, int len, int flags);


#endif /* _KERNEL_FILES_H */
