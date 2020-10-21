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
#include <kernel/stdc.h>
#include <kernel/vfs.h>
#include <assert.h>

typedef struct framebuffer framebuffer_t;

struct framebuffer {
    void *pixels;
    unsigned width;
    unsigned height;
    unsigned pitch;
    // splock_t lock;
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void *memcpy32(void *dest, void *src, size_t lg)
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

void *memset32(void *dest, uint32_t val, size_t lg)
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


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

page_t framebuffer_fetch(inode_t *ino, xoff_t off)
{
    if (ino->ops->fetch != NULL)
        return ino->ops->fetch(ino, off);
    assert(false);
    return 0;
}

void framebuffer_release(inode_t *ino, xoff_t off, page_t pg, bool dirty)
{
    if (ino->ops->release != NULL)
        return ino->ops->release(ino, off, pg, dirty);
    if (ino->ops->fetch != NULL)
        return;
    assert(false);
}

int framebuffer_fcntl(inode_t *ino, int cmd, void **params)
{
    framebuffer_t *fb = ino->drv_data;
    if (cmd == FB_RESIZE) {
        size_t w = (size_t)params[0];
        size_t h = (size_t)params[1];
        if (w > 8196 || h > 8196)
            return -1;
        fb->width = w;
        fb->height = h;
        fb->pitch = w * 4;
        // TODO - Send event to those who listen ?
    }
    return -1;
}

void framebuffer_destroy(inode_t *ino)
{
    framebuffer_t *fb = ino->drv_data;
    kfree(fb);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

fl_ops_t framebuffer_ops = {
    .fetch = framebuffer_fetch,
    .release = framebuffer_release,
    .fcntl = framebuffer_fcntl,
    .destroy = framebuffer_destroy
};

framebuffer_t *framebuffer_create()
{
    framebuffer_t *fb = (framebuffer_t *)kalloc(sizeof(framebuffer_t));
    fb->width = 0;
    fb->height = 0;
    // fb->depth = 32;
    fb->pitch = 0;
    fb->pixels = NULL;
    // splock_init(&fb->lock);
    return fb;
}

