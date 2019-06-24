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
#include <kernel/task.h>
#include <kernel/device.h>
#include <kernel/input.h>
#include <kernel/syscalls.h>
#include <kora/splock.h>
#include <kora/rwlock.h>
#include <kora/llist.h>
#include <kora/mcrs.h>
#include <errno.h>

int abs(int v);


#define WMGR_RENDER  1

#define DESK_GRAB_WINDOW 0
#define DESK_GRAB_HEADER 1
#define DESK_GRAB_LEFT 0x10
#define DESK_GRAB_RIGHT 0x20
#define DESK_GRAB_TOP 0x40
#define DESK_GRAB_BOTTOM 0x80


#define WMGR_GRID_NORMAL 0
#define WMGR_GRID_FULL 3
#define WMGR_GRID_LEFT 7
#define WMGR_GRID_TOP 11
#define WMGR_GRID_RIGHT 6
#define WMGR_GRID_BOTTOM 9
#define WMGR_GRID_TOP_LEFT 15
#define WMGR_GRID_TOP_RIGHT 14
#define WMGR_GRID_BOTTOM_LEFT 13
#define WMGR_GRID_BOTTOM_RIGHT 12


desktop_t *kDESK;


void wmgr_window_flip(inode_t *ino)
{
    window_t *win = (window_t *)ino->info;
    win->queries |= WMGR_RENDER;
}

int win_fcntl(inode_t *ino, int cmd, void *args)
{
    switch (cmd) {
    case 17:
        wmgr_window_flip(ino);
        return 0;
    default:
        errno = ENOSYS;
        return -1;
    }
}

page_t win_fetch(inode_t *ino, off_t off)
{
    window_t *win = (window_t *)ino->info;
    framebuffer_t *frame = win->frame;
    page_t pg = mmu_read((size_t)&frame->pixels[off]);
    return pg;
}


int wmgr_window_read(inode_t *ino, char *buf, size_t len, int flags)
{
    return pipe_read(((window_t *) ino->info)->pipe, buf, len, flags) ;
}

int wmgr_window_resize(inode_t *ino, int width, int height)
{
    window_t *win = (window_t *) ino->info;
    gfx_resize(win->frame, width, height, NULL);
    return 0;
}


ino_ops_t win_ops = {
    .fcntl = win_fcntl,
    // .close = win_close,
    .fetch = win_fetch,
    // .release = win_release,
    .read = (void*)wmgr_window_read,
    // .reset = win_reset,
    .flip = wmgr_window_flip,
    .resize = (void *)wmgr_window_resize,
    // .copy = win_copy,
};

void itimer_create(pipe_t *pipe, long delay, long interval);


window_t *wmgr_window(desktop_t *desk, int width, int height)
{
    if (desk == NULL)
        desk = kDESK;
    window_t *win = kalloc(sizeof(window_t));

    /*
    screen_t *screen = desk->screen;
    if (desk->ox + 2 + DESK_PADDING + width > screen->sz.w)
        desk->ox = DESK_PADDING;
    if (desk->oy + 9 + DESK_PADDING + height > screen->sz.h)
        desk->oy = DESK_PADDING;
    win->no = ++desk->no;
    win->mt = 15;
    win->ml = 1;
    win->mr = 1;
    win->mb = 1;
    win->rq.x = desk->ox + win->ml + screen->sz.x;
    win->rq.y = desk->oy + win->mt + screen->sz.y;
    win->rq.w = MAX(10, width);
    win->rq.h = MAX(10, height);
    */
    win->frame = gfx_create(width, height, 4, NULL);
    win->color = RGB(rand8(), rand8(), rand8());
    win->desk = desk;
    win->pipe = pipe_create();
    itimer_create(win->pipe, MSEC_TO_KTIME(100), MSEC_TO_KTIME(40));
    // desk->ox += 2 * DESK_PADDING;
    // desk->oy += 2 * DESK_PADDING;
    splock_lock(&desk->lock);
    ll_append(&desk->windows, &win->node);
    splock_unlock(&desk->lock);
    return win;
}

inode_t *wmgr_create_window(desktop_t *desk, int width, int height)
{
    window_t *win = wmgr_window(desk, width, height);
    inode_t *ino = vfs_inode(win->no, FL_WIN, NULL);
    ino->info = win;
    ino->ops = &win_ops;
    return ino;
}
