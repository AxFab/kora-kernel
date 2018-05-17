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
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kora/mcrs.h>

#include <stddef.h>
#include <assert.h>

// #include <stdlib.h>
#include <string.h>

// TODO Need this only for wpen_ -- Access by function!
struct fifo {
    size_t rpen_;
    size_t wpen_;
    size_t avail_;
    size_t size_;
    void *buf_;
    // mutex_t mtx_;
    // void (*wait)(fifo_t*, int);
    // void (*awake)(fifo_t*, int);
};

// typedef struct mutex mutex_t;
// struct mutex {
//   int i;
// };
// void mtx_lock(mutex_t* m) {}
// void mtx_unlock(mutex_t* m) {}

static void fifo_lock(fifo_t *fifo) {}
static void fifo_unlock(fifo_t *fifo) {}
static void fifo_wait(fifo_t *fifo, int flag, int bytes) {}
static void fifo_awake(fifo_t *fifo, int flag, int bytes) {}


/* Instanciate a new FIFO */
fifo_t *fifo_create(void *buf, size_t lg)
{
    fifo_t *fifo = (fifo_t *)kalloc(sizeof(fifo_t));
    memset(fifo, 0, sizeof(fifo_t));
    fifo->buf_ = buf;
    fifo->size_ = lg;
    return fifo;
}


/* Look for a specific bytes in consumable data */
size_t fifo_indexof(fifo_t *fifo, char ch)
{
    size_t i;
    size_t cap;
    size_t max = fifo->size_ - fifo->rpen_;
    char *address = (char *)(fifo->buf_ + fifo->rpen_);

    cap = MIN(max, fifo->avail_);
    for (i = 0; i < cap; ++i)
        if (address[i] == ch) {
            return i + 1;
        }

    if (max > fifo->avail_) {
        return 0;
    }

    address = (char *)fifo->buf_;
    cap = (size_t)MAX(0, (int)fifo->avail_ - (int)max);
    for (i = 0; i < cap; ++i)
        if (address[i] == ch) {
            return i + 1;
        }

    return 0;
}


/* Read some bytes from the FIFO */
size_t fifo_out(fifo_t *fifo, void *buf, size_t lg, int flags)
{
    size_t cap = 0;
    size_t bytes = 0;
    void *address;

    fifo_lock(fifo);
    while (lg > 0) {
        if (fifo->rpen_ >= fifo->size_) {
            fifo->rpen_ = 0;
        }

        address = (void *)(fifo->buf_ + fifo->rpen_);
        cap = fifo->size_ - fifo->rpen_;
        cap = MIN(cap, fifo->avail_);
        if (flags & FP_EOL) {
            cap = MIN(cap, fifo_indexof(fifo, '\n'));
        }
        cap = MIN(cap, lg);
        if (cap == 0) {
            if (flags & FP_NOBLOCK || bytes != 0) {
                break;
            }
            fifo_wait(fifo, flags, -lg);
            continue;
        }

        memcpy(buf, address, cap);
        fifo->rpen_ += cap;
        fifo->avail_ -= cap;
        lg -= cap;
        bytes += cap;
        buf = ((char *)buf) + cap;
    }

    fifo_awake(fifo, flags, -bytes);
    fifo_unlock(fifo);
    return bytes;
}


/* Write some bytes on the FIFO */
size_t fifo_in(fifo_t *fifo, const void *buf, size_t lg, int flags)
{
    size_t cap = 0;
    size_t bytes = 0;
    void *address;

    fifo_lock(fifo);
    while (lg > 0) {
        if (fifo->wpen_ >= fifo->size_) {
            fifo->wpen_ = 0;
        }

        address = (void *)(fifo->buf_ + fifo->wpen_);
        cap = fifo->size_ - fifo->wpen_;
        cap = MIN(cap, fifo->size_ - fifo->avail_);
        cap = MIN(cap, lg);
        if (cap == 0) {
            if (flags & FP_NOBLOCK || bytes != 0) {
                break;
            }
            fifo_wait(fifo, flags, lg);
            continue;
        }

        memcpy(address, buf, cap);
        fifo->wpen_ += cap;
        fifo->avail_ += cap;
        lg -= cap;
        bytes += cap;
        buf = ((char *)buf) + cap;
    }

    fifo_awake(fifo, flags, bytes);
    fifo_unlock(fifo);
    return bytes;
}


/* Reinitialize the queue */
void fifo_reset(fifo_t *fifo)
{
    fifo_lock(fifo);
    fifo->rpen_ = 0;
    fifo->wpen_ = 0;
    fifo->avail_ = 0;
    fifo_unlock(fifo);
}

