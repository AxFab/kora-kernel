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
#include <kora/mcrs.h>
#include <string.h>
#include <errno.h>
#include <threads.h>


struct pipe {
    char *base;
    char *rpen;
    char *wpen;
    char *end;
    size_t size;
    size_t max_size;
    size_t avail;
    bool hangup;

    mtx_t mutex;
    cnd_t rd_cond;
    cnd_t wr_cond;
};

static int pipe_resize_unlock_(pipe_t *pipe, size_t size)
{
    if (size < pipe->avail || size > pipe->max_size)
        return -1;
    char *remap = kmap(size, NULL, 0, VMA_PIPE_RW);

    if (size > pipe->size) {
        // TODO - Move pages will be faster!
        memcpy(remap, pipe->base, pipe->size);
        if (pipe->rpen < pipe->wpen) {

        } else if ((size_t)(pipe->wpen - pipe->base) < size - pipe->size) {
            memcpy(remap + pipe->size, pipe->base, pipe->wpen - pipe->base);
            pipe->wpen += pipe->size;
        } else {
            memcpy(remap + pipe->size, pipe->base, size - pipe->size);
            memcpy(remap, pipe->base + size - pipe->size,
                   pipe->wpen - pipe->base - (size - pipe->size));
            pipe->wpen = 0;// TODO - !?
            return -1;
        }
        pipe->rpen = remap + (pipe->rpen - pipe->base);
        pipe->wpen = remap + (pipe->wpen - pipe->base);
    } else if (size > pipe->avail) {
        memcpy(remap, pipe->rpen, pipe->avail);
        pipe->rpen = remap;
        pipe->wpen = remap + pipe->avail;
    } else
        return -1;

    kunmap(pipe->base, pipe->size);
    pipe->base = remap;
    pipe->size = size;
    pipe->end = pipe->base + size;
    return 0;
}

static int pipe_erase_unlock_(pipe_t *pipe, size_t len)
{
    int bytes = 0;
    while (len > 0) {
        size_t cap = MIN3(len, (size_t)(pipe->end - pipe->rpen), pipe->avail);
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
    pipe_t *pipe = (pipe_t *)kalloc(sizeof(pipe_t));
    pipe->size = PAGE_SIZE; // TODO -- Read config!
    pipe->max_size = 64 * PAGE_SIZE;
    pipe->base = kmap(pipe->size, NULL, 0, VMA_PIPE_RW);
    pipe->avail = 0;
    pipe->rpen = pipe->base;
    pipe->wpen = pipe->base;
    pipe->end = pipe->base + pipe->size;
    mtx_init(&pipe->mutex, mtx_plain);
    return pipe;
}

void pipe_destroy(pipe_t *pipe)
{
    mtx_destroy(&pipe->mutex);
    kunmap(pipe->base, pipe->size);
    kfree(pipe);
}


int pipe_resize(pipe_t *pipe, size_t size)
{
    mtx_lock(&pipe->mutex);
    int ret = pipe_resize_unlock_(pipe, size);
    mtx_unlock(&pipe->mutex);
    return ret;
}

int pipe_erase(pipe_t *pipe, size_t len)
{
    mtx_lock(&pipe->mutex);
    int ret = pipe_erase_unlock_(pipe, len);
    mtx_unlock(&pipe->mutex);
    return ret;
}

int pipe_reset(pipe_t *pipe)
{
    return -1;
}


void pipe_hangup(pipe_t* pipe)
{
    pipe->hangup = true;
    cnd_broadcast(&pipe->rd_cond);
}

int pipe_write(pipe_t *pipe, const char *buf, size_t len, int flags)
{
    if (pipe->hangup) {
        errno = EPIPE;
        return -1;
    }
    int bytes = 0;
    mtx_lock(&pipe->mutex);
    if (flags & IO_NO_BLOCK && len > pipe->size - pipe->avail) {
        while (len > pipe->size - pipe->avail && pipe->size * 2 <= pipe->max_size)
            pipe_resize_unlock_(pipe, MIN(pipe->size * 2, pipe->max_size));
        if (len > pipe->size - pipe->avail) {
            mtx_unlock(&pipe->mutex);
            errno = EWOULDBLOCK;
            return -1;
        }
    } else if (flags & IO_ATOMIC) {
        if (len > pipe->max_size) {
            errno = E2BIG;
            return -1;
        }
        while (len > pipe->size - pipe->avail && pipe->size * 2 <= pipe->max_size)
            pipe_resize_unlock_(pipe, MIN(pipe->size * 2, pipe->max_size));
        while (len > pipe->size - pipe->avail)
            cnd_wait(&pipe->wr_cond, &pipe->mutex);
    }
    while (len > 0) {
        int cap = MIN3(len, (size_t)(pipe->end - pipe->wpen), pipe->size - pipe->avail);
        if (cap == 0) {
            if (pipe->size < pipe->max_size) {

                pipe_resize_unlock_(pipe, MIN(pipe->size * 2, pipe->max_size));
                continue;
            }
            // cnd_signal(&pipe->rd_cond);
            cnd_broadcast(&pipe->rd_cond);
            if (flags & IO_NO_BLOCK)
                break;
            if (flags & IO_CONSUME) {
                pipe_erase_unlock_(pipe, len);
                continue;
            }
            cnd_wait(&pipe->wr_cond, &pipe->mutex);
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
    // cnd_signal(&pipe->rd_cond);
    cnd_broadcast(&pipe->rd_cond);
    mtx_unlock(&pipe->mutex);
    errno = 0;
    return bytes;
}

int pipe_read(pipe_t *pipe, char *buf, size_t len, int flags)
{
    int bytes = 0;
    mtx_lock(&pipe->mutex);
    if (flags & IO_NO_BLOCK && len > pipe->avail) {
        mtx_unlock(&pipe->mutex);
        errno = EWOULDBLOCK;
        return -1;
    } else if (flags & IO_ATOMIC) {
        if (len > pipe->max_size) {
            errno = E2BIG;
            return -1;
        }
        while (len > pipe->avail)
            cnd_wait(&pipe->rd_cond, &pipe->mutex);
    }
    while (len > 0) {
        int cap = MIN3(len, (size_t)(pipe->end - pipe->rpen), pipe->avail);
        if (cap == 0) {
            if (bytes > 0) // TODO -- flags !?
                break;
            // cnd_signal(&pipe->wr_cond);
            if (pipe->hangup && pipe->avail == 0)
                break;
            if (flags & IO_NO_BLOCK)
                break;
            cnd_broadcast(&pipe->wr_cond);
            cnd_wait(&pipe->rd_cond, &pipe->mutex);
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
    // cnd_signal(&pipe->wr_cond);
    cnd_broadcast(&pipe->wr_cond);
    mtx_unlock(&pipe->mutex);
    errno = 0;
    return bytes;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


