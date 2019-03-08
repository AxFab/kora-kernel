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
#include <kernel/files.h>
#include <string.h>

framebuffer_t *gfx_create(int width, int height, int depth, void *pixels)
{
    framebuffer_t *fb = (framebuffer_t *)kalloc(sizeof(framebuffer_t));
    fb->width = width;
    fb->height = height;
    fb->depth = depth;
    fb->pitch = ALIGN_UP(width * depth, 4);
    fb->pixels = pixels != NULL ? pixels : kmap(ALIGN_UP(height * fb->pitch, PAGE_SIZE), NULL, 0, VMA_ANON_RW | VMA_RESOLVE);
    return fb;
}

void gfx_resize(framebuffer_t *fb, int w, int h, void *pixels)
{
    // TODO -- rwlock pixels, inval, copy
    if (fb->width >= w && fb->height >= h)
        return;
    if (pixels == NULL) {
        kunmap(fb->pixels, ALIGN_UP(fb->height * fb->pitch, PAGE_SIZE));
        fb->width = w;
        fb->height = h;
        fb->pitch = ALIGN_UP(w * fb->depth, 4);
        fb->pixels = kmap(ALIGN_UP(fb->height * fb->pitch, PAGE_SIZE), NULL, 0, VMA_ANON_RW | VMA_RESOLVE);
    } else
        fb->pixels = pixels;
}

void gfx_rect(framebuffer_t *fb, int x, int y, int w, int h, uint32_t color)
{
    int i, j, dp = fb->depth;
    int minx = MAX(0, x);
    int maxx = MIN(fb->width, x + w);
    int miny = MAX(0, y);
    int maxy = MIN(fb->height, y + h);
    for (j = miny; j < maxy; ++j) {
        for (i = minx; i < maxx; ++i) {
            fb->pixels[(j)*fb->pitch + (i)* dp + 0] = (color >> 0) & 0xFF;
            fb->pixels[(j)*fb->pitch + (i)* dp + 1] = (color >> 8) & 0xFF;
            fb->pixels[(j)*fb->pitch + (i)* dp + 2] = (color >> 16) & 0xFF;
        }
    }
}

float sqrtf(float f);
double sqrt(double f);

void gfx_shadow(framebuffer_t *fb, int x, int y, int r, uint32_t color)
{
    int i, j, dp = fb->depth;
    int minx = MAX(0, x - r);
    int maxx = MIN(fb->width, x + r);
    int miny = MAX(0, y - r);
    int maxy = MIN(fb->height, y + r);
    for (j = miny; j < maxy; ++j) {
        for (i = minx; i < maxx; ++i) {
            int d = (j - y) * (j - y) + (i - x) * (i - x);
            float ds = 1.0f - (/*sqrtf*/((float)d) / (float)r);
            if (ds <= 0.0f)
                continue;
            float a = ds * ((color >> 24) / 255.0f);
            fb->pixels[(j)*fb->pitch + (i)* dp + 0] = (color >> 0) & 0xFF;
            fb->pixels[(j)*fb->pitch + (i)* dp + 1] = (color >> 8) & 0xFF;
            fb->pixels[(j)*fb->pitch + (i)* dp + 2] = (color >> 16) & 0xFF;
            fb->pixels[(j)*fb->pitch + (i)* dp + 3] = (uint8_t)(255 * a);
        }
    }
}

void gfx_clear(framebuffer_t *fb, uint32_t color)
{
    uint32_t *pixels = (uint32_t *)fb->pixels;
    uint32_t size = fb->pitch * fb->height / 16;
    while (size-- > 0) {
        pixels[0] = color;
        pixels[1] = color;
        pixels[2] = color;
        pixels[3] = color;
        pixels += 4;
    }
}


void gfx_slide(framebuffer_t *sfc, int height, uint32_t color)
{
    int px, py;
    if (height < 0) {
        height = -height;
        memcpy(sfc->pixels, ADDR_OFF(sfc->pixels, sfc->pitch * height), sfc->pitch * (sfc->height - height));
        for (py = sfc->height - height; py < sfc->height; ++py) {
            int pxrow = sfc->pitch * py;
            for (px = 0; px < sfc->width; ++px) {
                uint32_t *pixel = ADDR_OFF(sfc->pixels, pxrow + px * 4);
                *pixel = color;
            }
        }
    }
}


void gfx_copy(framebuffer_t *dest, framebuffer_t *src, int x, int y, int w, int h)
{
    int j, i, dd = dest->depth, ds = src->depth;
    for (j = 0; j < MIN(h, src->height); ++j) {
        if (j + y < 0 || j + y >= dest->height)
            continue;
        for (i = 0; i < MIN(w, src->width); ++i) {
            if (i + x < 0 || i + x >= dest->width)
                continue;
            int r, g, b;
            r = src->pixels[(j) * src->pitch + (i) * ds + 2];
            g = src->pixels[(j) * src->pitch + (i) * ds + 1];
            b = src->pixels[(j) * src->pitch + (i) * ds + 0];
            dest->pixels[(j + y)*dest->pitch + (i + x)* dd + 0] = b;
            dest->pixels[(j + y)*dest->pitch + (i + x)* dd + 1] = g;
            dest->pixels[(j + y)*dest->pitch + (i + x)* dd + 2] = r;
        }
    }
}


void gfx_copy_blend(framebuffer_t *dest, framebuffer_t *src, int x, int y)
{
    int j, i, dd = dest->depth, ds = src->depth;
    for (j = 0; j < src->height; ++j) {
        if (j + y < 0 || j + y >= dest->height)
            continue;
        for (i = 0; i < src->width; ++i) {
            if (i + x < 0 || i + x >= dest->width)
                continue;
            int r, g, b;
            int a = src->pixels[(j) * src->pitch + (i) * ds + 3];
            if (a == 0)
                continue;
            r = src->pixels[(j) * src->pitch + (i) * ds + 2];
            g = src->pixels[(j) * src->pitch + (i) * ds + 1];
            b = src->pixels[(j) * src->pitch + (i) * ds + 0];
            if (a != 255) {
                r = r * 255 / a + dest->pixels[(j + y) * dest->pitch + (i + x) * dd + 2] * 255 / (255 - a);
                g = g * 255 / a + dest->pixels[(j + y) * dest->pitch + (i + x) * dd + 1] * 255 / (255 - a);
                b = b * 255 / a + dest->pixels[(j + y) * dest->pitch + (i + x) * dd + 0] * 255 / (255 - a);
            }
            dest->pixels[(j + y)*dest->pitch + (i + x)* dd + 0] = b;
            dest->pixels[(j + y)*dest->pitch + (i + x)* dd + 1] = g;
            dest->pixels[(j + y)*dest->pitch + (i + x)* dd + 2] = r;
        }
    }
}


void gfx_flip(framebuffer_t *fb)
{

}
