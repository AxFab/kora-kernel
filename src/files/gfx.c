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


static void *memcpy32(void *dest, void *src, size_t lg)
{
    assert(IS_ALIGNED(lg, 4));
    assert(IS_ALIGNED((size_t)dest, 4));
    assert(IS_ALIGNED((size_t)src, 4));
    register uint32_t *a = (uint32_t *)src;
    register uint32_t *b = (uint32_t *)dest;
    while (lg > 16) {
        b[0] = a[0];
        b[1] = a[1];
        b[2] = a[2];
        b[3] = a[3];
        lg -= 16;
        a += 4;
        b += 4;
    }
    while (lg > 0) {
        b[0] = a[0];
        lg -= 4;
        a++;
        b++;
    }
    return dest;
}

static void *memset32(void *dest, uint32_t val, size_t lg)
{
    assert(IS_ALIGNED(lg, 4));
    assert(IS_ALIGNED((size_t)dest, 4));
    register uint32_t *a = (uint32_t *)dest;
    while (lg > 16) {
        a[0] = val;
        a[1] = val;
        a[2] = val;
        a[3] = val;
        lg -= 16;
        a += 4;
    }
    while (lg > 0) {
        a[0] = val;
        lg -= 4;
        a++;
    }
    return dest;
}


framebuffer_t *gfx_create(int width, int height, int depth, void *pixels)
{
    framebuffer_t *fb = (framebuffer_t *)kalloc(sizeof(framebuffer_t));
    fb->width = width;
    fb->height = height;
    fb->depth = depth;
    fb->pitch = ALIGN_UP(width * depth, 4);
    fb->pixels = pixels != NULL ? pixels : kmap(ALIGN_UP(height * fb->pitch, PAGE_SIZE), NULL, 0, VMA_ANON_RW | VMA_RESOLVE);
    rwlock_init(&fb->lock);
    return fb;
}

void gfx_destroy(framebuffer_t *fb, void *pixels)
{
    if (pixels == NULL)
        kunmap(fb->pixels, ALIGN_UP(fb->height * fb->pitch, PAGE_SIZE));
    kfree(fb);
}

void gfx_resize(framebuffer_t *fb, int w, int h, void *pixels)
{
    // TODO -- in val & copy
    if (fb->width >= w && fb->height >= h)
        return;
    rwlock_wrlock(&fb->lock);
    if (pixels == NULL) {
        kunmap(fb->pixels, ALIGN_UP(fb->height * fb->pitch, PAGE_SIZE));
        fb->width = w;
        fb->height = h;
        fb->pitch = ALIGN_UP(w * fb->depth, 4);
        fb->pixels = kmap(ALIGN_UP(fb->height * fb->pitch, PAGE_SIZE), NULL, 0, VMA_ANON_RW | VMA_RESOLVE);
    } else
        fb->pixels = pixels;
    rwlock_wrunlock(&fb->lock);
}

void gfx_rect(framebuffer_t *fb, int x, int y, int w, int h, uint32_t color)
{
    rwlock_rdlock(&fb->lock);
    int i, j, dp = fb->depth;
    int minx = MAX(0, x);
    int maxx = MIN(fb->width, x + w);
    int miny = MAX(0, y);
    int maxy = MIN(fb->height, y + h);

    if (fb->depth == 4) {
        for (j = miny; j < maxy; ++j)
            memset32(&fb->pixels[j * fb->pitch + minx * 4], color, (maxx - minx) * 4);
        rwlock_rdunlock(&fb->lock);
        return;
    }

    for (j = miny; j < maxy; ++j) {
        for (i = minx; i < maxx; ++i) {
            fb->pixels[(j)*fb->pitch + (i)* dp + 0] = (color >> 0) & 0xFF;
            fb->pixels[(j)*fb->pitch + (i)* dp + 1] = (color >> 8) & 0xFF;
            fb->pixels[(j)*fb->pitch + (i)* dp + 2] = (color >> 16) & 0xFF;
        }
    }
    rwlock_rdunlock(&fb->lock);
}

float sqrtf(float f);
double sqrt(double f);

void gfx_shadow(framebuffer_t *fb, int x, int y, int r, uint32_t color)
{
    rwlock_rdlock(&fb->lock);
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
    rwlock_rdunlock(&fb->lock);
}

void gfx_clear(framebuffer_t *fb, uint32_t color)
{
    rwlock_rdlock(&fb->lock);
    if (fb->depth == 4) {
        memset32(fb->pixels, color, fb->pitch * fb->height);
        rwlock_rdunlock(&fb->lock);
        return;
    }

    uint32_t *pixels = (uint32_t *)fb->pixels;
    uint32_t size = fb->pitch * fb->height / 16;
    while (size-- > 0) {
        pixels[0] = color;
        pixels[1] = color;
        pixels[2] = color;
        pixels[3] = color;
        pixels += 4;
    }
    rwlock_rdunlock(&fb->lock);
}


void gfx_slide(framebuffer_t *fb, int height, uint32_t color)
{
    rwlock_rdlock(&fb->lock);
    int px, py;
    if (height < 0) {
        height = -height;
        memcpy32(fb->pixels, ADDR_OFF(fb->pixels, fb->pitch * height), fb->pitch * (fb->height - height));
        for (py = fb->height - height; py < fb->height; ++py) {
            int pxrow = fb->pitch * py;
            for (px = 0; px < fb->width; ++px) {
                uint32_t *pixel = ADDR_OFF(fb->pixels, pxrow + px * 4);
                *pixel = color;
            }
        }
    }
    rwlock_rdunlock(&fb->lock);
}


void gfx_copy(framebuffer_t *dest, framebuffer_t *src, int x, int y, int w, int h)
{
    rwlock_rdlock(&src->lock);
    rwlock_rdlock(&dest->lock);
    int j, i;
    int minx = MAX(0, -x);
    int maxx = MIN(MIN(w, src->width), dest->width - x);
    int miny = MAX(0, -y);
    int maxy = MIN(MIN(h, src->height), dest->height - y);

    if (dest->depth == src->depth && src->depth == 4) {
        for (i = miny; i < maxy; ++i) {
            int ka = i * src->pitch;
            int kb = (i + y) * dest->pitch;
            memcpy32(&dest->pixels[kb + (minx + x) * 4], &src->pixels[ka + minx * 4], (maxx - minx) * 4);
        }
        rwlock_rdunlock(&dest->lock);
        rwlock_rdunlock(&src->lock);
        return;
    }

    int dd = dest->depth, ds = src->depth;
    for (j = miny; j < maxy; ++j) {
        for (i = minx; i < maxx; ++i) {
            int r, g, b;
            r = src->pixels[(j) * src->pitch + (i) * ds + 2];
            g = src->pixels[(j) * src->pitch + (i) * ds + 1];
            b = src->pixels[(j) * src->pitch + (i) * ds + 0];
            dest->pixels[(j + y)*dest->pitch + (i + x)* dd + 0] = b;
            dest->pixels[(j + y)*dest->pitch + (i + x)* dd + 1] = g;
            dest->pixels[(j + y)*dest->pitch + (i + x)* dd + 2] = r;
        }
    }
    rwlock_rdunlock(&dest->lock);
    rwlock_rdunlock(&src->lock);
}


void gfx_copy_blend(framebuffer_t *dest, framebuffer_t *src, int x, int y)
{
    rwlock_rdlock(&src->lock);
    rwlock_rdlock(&dest->lock);
    int j, i;
    int minx = MAX(0, -x);
    int maxx = MIN(src->width, dest->width - x);
    int miny = MAX(0, -y);
    int maxy = MIN(src->height, dest->height - y);

    int dd = dest->depth, ds = src->depth;
    for (j = miny; j < maxy; ++j) {
        for (i = minx; i < maxx; ++i) {
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
    rwlock_rdunlock(&dest->lock);
    rwlock_rdunlock(&src->lock);
}

