/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2018  <Fabien Bavent>
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
#include <kernel/device.h>
#include <kernel/files.h>
#include <errno.h>

struct desktop {
    atomic_t win_no;
    llhead_t list;
};

// Desktop related
surface_t *create_win();

typedef struct window window_t;
struct window {
    surface_t *fb;
    pipe_t *mq;
    task_t *task;
    int status;
    llnode_t node;
};


// All
int win_fcntl(inode_t *ino, int cmd, void *params)
{
    errno = ENOSYS;
    return -1;
}

int win_close(inode_t *ino)
{
    assert(ino->type == FL_WIN);
    window_t *win = (window_t *)ino->info;
    ino->info = NULL;
    vds_destroy(win->fb);
    pipe_destroy(win->mq);
    kfree(win);
    return 0;
}

// Mapping
page_t win_fetch(inode_t *ino, off_t off)
{
    assert(ino->type == FL_WIN);
    window_t *win = (window_t *)ino->info;
    surface_t *fb = win->fb;
    return vds_fetch(fb, off);
}

void win_release(inode_t *ino, off_t off, page_t pg)
{
    assert(ino->type == FL_WIN);
    // window_t *win = (window_t *)ino->info;
    // surface_t *fb = win->fb;
    // vds_release(fb, off, pg);
}

// Fifo
int win_read(inode_t *ino, char *buf, size_t len, int flags)
{
    assert(ino->type == FL_WIN);
    window_t *win = (window_t *)ino->info;
    return pipe_read(win->mq, buf, len, flags);
}

int win_write(inode_t *ino, const char *buf, size_t len, int flags)
{
    assert(ino->type == FL_WIN);
    window_t *win = (window_t *)ino->info;
    return pipe_write(win->mq, buf, len, flags);
}

void win_reset(inode_t *ino)
{
    assert(ino->type == FL_WIN);
    window_t *win = (window_t *)ino->info;
    return pipe_reset(win->mq);
}

// Framebuffer
void win_flip(inode_t *ino)
{

}

void win_resize(inode_t *ino, int width, int height)
{

}

void win_copy(inode_t *dst, inode_t *src, int x, int y, int lf, int tp, int rg, int bt)
{
    // vds_copy(dst, src, x, y, lf, tp, rg, bt);
}


ino_ops_t win_ops = {
    .fcntl = win_fcntl,
    .close = win_close,
    .fetch = win_fetch,
    .release = win_release,
    .read = win_read,
    .write = win_write,
    .reset = win_reset,
    .flip = win_flip,
    .resize = win_resize,
    .copy = win_copy,
};


inode_t *window_open(desktop_t *desk, int width, int height, int flags)
{
    surface_t *fb = create_win(width, height);
    if (fb == NULL)
        return NULL;
    inode_t *ino = vfs_inode(atomic_xadd(desk->win_no, 1), FL_WIN, NULL);

    window_t *win = kalloc(sizeof(window_t));
    ino->ops = &win_ops;
    ino->info = win;
}


void window_close(desktop_t *desk, inode_t *win)
{

}

void *window_map(mspace_t *mspace, inode_t *win)
{
    return NULL;
}

int window_set_features(inode_t *win, int features, int *args)
{
    return -1;
}

int window_get_features(inode_t *win, int features, int *args)
{
    return -1;
}

int window_push_event(inode_t *win, event_t *event)
{
    return -1;
}

int window_poll_push(inode_t *win, event_t *event)
{
    return -1;
}

