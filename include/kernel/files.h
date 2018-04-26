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
#include <stdint.h>


typedef struct surface surface_t;
typedef struct line line_t;
typedef struct tty tty_t;
typedef struct bitmap_font font_t;

struct surface {
    int width;
    int height;
    int pitch;
    int bits;
    uint8_t *pixels;
    uint8_t *backup;
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
tty_t *tty_create(surface_t *win, const font_t *font, const uint32_t *colors, int iv);
void tty_destroy(tty_t *tty);
void tty_paint(tty_t *tty);
void tty_scroll(tty_t *tty, int count);


void vds_fill(surface_t *win, uint32_t color);
void vds_copy(surface_t *dest, surface_t *src, int x, int y);
surface_t *vds_create(int width, int height, int depth);
void vds_destroy(surface_t *srf);


#endif /* _KERNEL_FILES_H */
