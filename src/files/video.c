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


// static void vds_rect(surface_t *win, int x, int y, int w, int h, uint32_t color)
// {
//     int i, j, dp = win->depth;
//     for (j = 0; j < h; ++j) {
//         for (i = 0; i < w; ++i) {
//             win->pixels[(j+y)*win->pitch+(i+x)* dp + 0] = (color >> 0) & 0xFF;
//             win->pixels[(j+y)*win->pitch+(i+x)* dp + 1] = (color >> 8) & 0xFF;
//             win->pixels[(j+y)*win->pitch+(i+x)* dp + 2] = (color >> 16) & 0xFF;
//         }
//     }
// }

// void vds_fill(surface_t *win, uint32_t color)
// {
//     vds_rect(win, 0, 0, win->width, win->height, color);
// }

void vds_fill(surface_t *win, uint32_t color)
{
    uint32_t *pixels = (uint32_t*)win->pixels;
    uint32_t size = win->pitch * win->height / 16;
    while (size-- > 0) {
        pixels[0] = color;
        pixels[1] = color;
        pixels[2] = color;
        pixels[3] = color;
        pixels += 4;
    }
}

void vds_copy(surface_t *dest, surface_t *src, int x, int y)
{
    int j, i, dd = dest->depth, ds = src->depth;
    for (j = 0; j < src->height; ++j) {
        if (j + y < 0 || j + y >= dest->height)
            continue;
        for (i = 0; i < src->width; ++i) {
            if (i + x < 0 || i + x >= dest->width)
                continue;
            int r, g, b;
            r = src->pixels[(j)*src->pitch+(i)* ds + 2];
            g = src->pixels[(j)*src->pitch+(i)* ds + 1];
            b = src->pixels[(j)*src->pitch+(i)* ds + 0];
            dest->pixels[(j+y)*dest->pitch+(i+x)* dd + 0] = b;
            dest->pixels[(j+y)*dest->pitch+(i+x)* dd + 1] = g;
            dest->pixels[(j+y)*dest->pitch+(i+x)* dd + 2] = r;
        }
    }
}

static void vds_array(surface_t *win, int x, int y, int sz, uint32_t color)
{
    int j, i, dp = win->depth;
    for (j = 0; j < sz; ++j) {
        if (j + y < 0 || j + y >= win->height)
            continue;
        int n = j - (j < (sz-6) ? 0 : j - (sz-6)) * 3;
        for (i = 0; i < n; ++i) {
            if (i + x < 0 || i + x >= win->width)
                continue;
            win->pixels[(j+y)*win->pitch+(i+x)* dp + 0] = (color >> 0) & 0xFF;
            win->pixels[(j+y)*win->pitch+(i+x)* dp + 1] = (color >> 8) & 0xFF;
            win->pixels[(j+y)*win->pitch+(i+x)* dp + 2] = (color >> 16) & 0xFF;
        }
    }
}

void vds_mouse(surface_t *win, int x, int y)
{
    vds_array(win, x, y, 16, 0x000000);
    vds_array(win, x+1, y+2, 14, 0xFFFFFF);
}

void vds_flip(surface_t *surface)
{
    if (surface->flip)
        surface->flip(surface);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

surface_t *vds_create_empty(int width, int height, int depth)
{
    surface_t *win = (surface_t*)kalloc(sizeof(surface_t));
    win->width = width;
    win->height = height;
    win->depth = depth;
    win->pitch = ALIGN_UP(width * depth, 4);
    return win;
}

surface_t *vds_create(int width, int height, int depth)
{
    surface_t *win = (surface_t*)kalloc(sizeof(surface_t));
    win->width = width;
    win->height = height;
    win->depth = depth;
    win->pitch = ALIGN_UP(width * depth, 4);
    win->pixels = kalloc(height * win->pitch);
    return win;
}

void vds_destroy(surface_t *srf)
{
    kfree(srf->pixels);
    kfree(srf);
}

