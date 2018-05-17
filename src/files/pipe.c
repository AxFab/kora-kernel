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
#include <kernel/task.h>
#include <kora/mcrs.h>
#include <kora/splock.h>
#include <kora/llist.h>
#include <string.h>
#include <errno.h>


struct pipe {
    char *base;
    char *rpen;
    char *wpen;
    char *end;
    int size;
    int max_size;
    int avail;

    splock_t lock;
    llhead_t rlist;
    llhead_t wlist;
};

static int pipe_resize_unlock_(pipe_t *pipe, int size)
{
    if (size < pipe->avail || size > pipe->max_size)
        return -1;
    char *remap = kalloc(size); //kmap(size, NULL, 0, VMA_PIPE_RW);

    if (size > pipe->size) {
        // TODO - Move pages will be faster!
        memcpy(remap, pipe->base, pipe->size);
        if (pipe->rpen < pipe->wpen) {

        } else if (pipe->wpen - pipe->base < size - pipe->size) {
            memcpy(remap + pipe->size, pipe->base, pipe->wpen - pipe->base);
            pipe->wpen += pipe->size;
        } else {
            memcpy(remap + pipe->size, pipe->base, size - pipe->size);
            memcpy(remap, pipe->base + size - pipe->size, pipe->wpen - pipe->base - (size - pipe->size));
            pipe->wpen = 0;// TODO - !?
            return -1;
        }
        pipe->rpen = remap + (pipe->rpen - pipe->base);
        pipe->wpen = remap + (pipe->wpen - pipe->base);
    } else {
        memcpy(remap, pipe->rpen, pipe->avail);
        pipe->rpen = remap;
        pipe->wpen = remap + pipe->avail;
    }
    kfree(pipe->base); // kunmap(pipe->base, pipe->size);
    pipe->base = remap;
    pipe->size = size;
    pipe->end = pipe->base + size;
    return 0;
}

static int pipe_erase_unlock_(pipe_t *pipe, int len)
{
    int bytes = 0;
    while (len > 0) {
        int cap = MIN3(len, pipe->end - pipe->rpen, pipe->avail);
        if (cap == 0)
            break;
        pipe->rpen += cap;
        pipe->avail += cap;
        len -= cap;
        bytes += cap;
        if (pipe->rpen == pipe->end)
            pipe->rpen = pipe->end;
    }
    return bytes;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

pipe_t *pipe_create()
{
    pipe_t *pipe = (pipe_t*)kalloc(sizeof(pipe_t));
    pipe->size = 16; // PAGE_SIZE; // TODO -- Read config!
    pipe->max_size = 64; // * PAGE_SIZE;
    pipe->base = kalloc(pipe->size);// kmap(pipe->size, NULL, 0, VMA_PIPE_RW);
    pipe->avail = 0;
    pipe->rpen = pipe->base;
    pipe->wpen = pipe->base;
    pipe->end = pipe->base + pipe->size;
    return pipe;
}

void pipe_destroy(pipe_t *pipe)
{
    kfree(pipe->base); // kunmap(pipe->base, pipe->size);
    kfree(pipe);
}


int pipe_resize(pipe_t *pipe, int size)
{
    splock_lock(&pipe->lock);
    int ret = pipe_resize_unlock_(pipe, size);
    splock_unlock(&pipe->lock);
    return ret;
}

int pipe_erase(pipe_t *pipe, int len)
{
    splock_lock(&pipe->lock);
    int ret = pipe_erase_unlock_(pipe, len);
    splock_unlock(&pipe->lock);
    return ret;
}


int pipe_write(pipe_t *pipe, const char *buf, int len, int flags)
{
    int bytes = 0;
    splock_lock(&pipe->lock);
    if (flags & (IO_NO_BLOCK | IO_ATOMIC) && len > pipe->size - pipe->avail) {
        splock_unlock(&pipe->lock);
        errno = EWOULDBLOCK;
        return -1;
    }
    while (len > 0) {
        int cap = MIN3(len, pipe->end - pipe->wpen, pipe->size - pipe->avail);
        if (cap == 0) {
            if (pipe->size < pipe->max_size) {
    // kdump(pipe->base, pipe->size);
    // kprintf(KLOG_DBG, "Bf\n");
                pipe_resize_unlock_(pipe, MIN(pipe->size * 2, pipe->max_size));
    // kdump(pipe->base, pipe->size);
    // kprintf(KLOG_DBG, "Af\n");
                continue;
            }
            advent_awake(&pipe->rlist, 0);
            if (flags & IO_NO_BLOCK)
                break;
            if (flags & IO_CONSUME) {
                pipe_erase_unlock_(pipe, len);
                continue;
            }
            advent_wait(&pipe->lock, &pipe->wlist, -1);
            continue;
        }
        memcpy(pipe->wpen, buf, cap);
        pipe->wpen += cap;
        pipe->avail += cap;
        len -= cap;
        bytes += cap;
        buf += cap;
        if (pipe->wpen == pipe->end)
            pipe->wpen = pipe->base;
    }
    advent_awake(&pipe->rlist, 0);
    splock_unlock(&pipe->lock);
    errno = 0;
    return bytes;
}

int pipe_read(pipe_t *pipe, char *buf, int len, int flags)
{
    int bytes = 0;
    splock_lock(&pipe->lock);
    if (flags & (IO_NO_BLOCK | IO_ATOMIC) && len > pipe->avail) {
        splock_unlock(&pipe->lock);
        errno = EWOULDBLOCK;
        return -1;
    }
    while (len > 0) {
        int cap = MIN3(len, pipe->end - pipe->rpen, pipe->avail);
        if (cap == 0) {
            advent_awake(&pipe->wlist, 0);
            if (flags & IO_NO_BLOCK)
                break;
            advent_wait(&pipe->lock, &pipe->rlist, -1);
            continue;
        }
        memcpy(buf, pipe->rpen, cap);
        pipe->rpen += cap;
        pipe->avail -= cap;
        len -= cap;
        bytes += cap;
        buf += cap;
        if (pipe->rpen == pipe->end)
            pipe->rpen = pipe->base;
    }
    advent_awake(&pipe->wlist, 0);
    splock_unlock(&pipe->lock);
    errno = 0;
    return bytes;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */



// int tty_write(tty_t *tty, char *buf, int len)
// {
//     pipe_write(tty->out, buf, len, IO_CONSUME);
//     tty->last
// }
