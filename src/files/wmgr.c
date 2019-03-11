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

int wmgr_window_read(inode_t *ino, char *buf, size_t len, int flags)
{
    return pipe_read(((window_t *) ino->info) ->pipe, buf, len, flags) ;
}

int wmgr_window_resize(inode_t *ino, int width, int height)
{
    window_t *win = (window_t *) ino->info;
    return gfx_resize(win->frame, width, height, NULL);
}

int wmgr_event(window_t *win, int event, int param1, int param2)
{
    event_t ev;
    ev.param1 = param1;
    ev.param2 = param2;
    ev.type = event;
    return pipe_write(win->pipe, &ev, sizeof(ev), 0);
}

ino_ops_t win_ops = {
    // .fcntl = win_fcntl,
    // .close = win_close,
    // .fetch = win_fetch,
    // .release = win_release,
    .read = wmgr_window_read,
    // .reset = win_reset,
    .flip = wmgr_window_flip,
    .resize = wmgr_window_resize,
    // .copy = win_copy,
};


window_t *wmgr_window(desktop_t *desk, int width, int height)
{
    if (desk == NULL)
        desk = kDESK;
    window_t *win = kalloc(sizeof(window_t));

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

    win->frame = gfx_create(width, height, 4, NULL);
    win->color = RGB(rand8(), rand8(), rand8());
    win->desk = desk;
    win->pipe = pipe_create();
    desk->ox += 2 * DESK_PADDING;
    desk->oy += 2 * DESK_PADDING;
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

void wmgr_invalid_rect(screen_t *screen, int x, int y, int w, int h)
{
    screen->queries |= WMGR_RENDER;
}

rect_t wmgr_window_size(window_t *win)
{
    if (win->grid <= 0)
        return win->sz;
    screen_t *screen = win->desk->screen;
    rect_t sz;
    sz.x = win->grid & 1 ? win->ml : screen->sz.w / 2 + win->ml;
    sz.y = win->grid & 2 ? win->mt : screen->sz.h / 2 + win->mt;
    sz.w = (win->grid & 4 ? screen->sz.w  / 2 : screen->sz.w) - win->ml - win->mr;
    sz.h = (win->grid & 8 ? screen->sz.h / 2 : screen->sz.h) - win->mt - win->mb;
    return sz;
}


int wmgr_check_invalid_window(screen_t *screen, window_t *win)
{
    if (win->queries & WMGR_RENDER) {
        win->queries &= ~WMGR_RENDER;
        screen->queries |= WMGR_RENDER;
        return 1;
    }
    if (win->grid == 0 && win->rq_grid == 0) {
        if (win->sz.x != win->rq.x || win->sz.y != win->rq.y || win->sz.w != win->rq.w || win->sz.y != win->rq.y) {
            wmgr_invalid_rect(screen, win->sz.x, win->sz.y, win->sz.w, win->sz.h);
            wmgr_invalid_rect(screen, win->rq.x, win->rq.y, win->rq.w, win->rq.h);
            win->sz = win->rq;
            wmgr_event(win, EV_WIN_RESIZE, win->sz.w, win->sz.h);
        } else {
            // wmgr_noinvalid_rect(screen, win->sz.x, win->sz.y, win->sz.w, win->sz.h);
        }
    } else if (win->grid != win->rq_grid) {
        rect_t sz = wmgr_window_size(win);
        wmgr_invalid_rect(screen, sz.x, sz.y, sz.w, sz.h);
        win->grid = win->rq_grid;
        sz = wmgr_window_size(win);
        wmgr_invalid_rect(screen, sz.x, sz.y, sz.w, sz.h);
        wmgr_event(win, EV_WIN_RESIZE, sz.w, sz.h);
    }
    return 0; // TODO return 1 if there is no other place to check !
}


void wmgr_render(screen_t *screen)
{
    window_t *win;
    rwlock_rdlock(&screen->lock);

    /* Handle screen requests */


    /* Look for pointer position */
    pointer_t mouse = screen->desk->pointer;
    if (mouse.x != screen->pointer.x || mouse.y != screen->pointer.y || mouse.idx != screen->pointer.idx) {
        wmgr_invalid_rect(screen, screen->pointer.x, screen->pointer.y, screen->pointer.size, screen->pointer.size);
        screen->pointer = mouse;
        wmgr_invalid_rect(screen, mouse.x, mouse.y, mouse.size, mouse.size);
    }

    /* Handle windows requests */
    if (screen->fullscreen != NULL)
        wmgr_check_invalid_window(screen, screen->fullscreen);
    else {
        for ll_each_reverse(&screen->desk->windows, win, window_t, node) {
            if (wmgr_check_invalid_window(screen, win) > 0)
                break;
        }
    }

    rwlock_rdunlock(&screen->lock);
    if (!(screen->queries & WMGR_RENDER))
        return;

    // Paint
    screen->queries &= ~WMGR_RENDER;
    gfx_clear(screen->frame, RGB(112, 146, 190));
    window_t *last = ll_last(&screen->desk->windows, window_t, node);
    for ll_each(&screen->desk->windows, win, window_t, node) {
        if (win->grid < 0)
            continue;
        rect_t sz = wmgr_window_size(win);

        // Frame
        uint32_t color = win == last ?  0x8b9d4e : 0xdcdcdc;
        gfx_rect(screen->frame, sz.x - win->ml, sz.y - win->mt, sz.w + win->ml + win->mr, win->mt, color);
        gfx_rect(screen->frame, sz.x - win->ml, sz.y + sz.h, sz.w + win->ml + win->mr, win->mb, color);
        gfx_rect(screen->frame, sz.x - win->ml, sz.y, win->ml, sz.h, color);
        gfx_rect(screen->frame, sz.x + sz.w, sz.y, win->mr, sz.h, color);
        // TODO - shadow

        // Icon
        // gfx_rect(screen->frame, sz.x + 1, sz.y - win->mt + 1, win->mt - 2, win->mt - 2, win->color);

        // Buffer
        // TODO copy or copy blend
        gfx_rect(screen->frame, sz.x, sz.y, sz.w, sz.h, 0x181818); // win->color);
        gfx_copy(screen->frame, win->frame, sz.x, sz.y, sz.w, sz.h);
    }

    // framebuffer_t *pts_sprite = screen->desk->pointer_sprites[screen->pointer.idx];
    // int pts_disp = screen->pointer.size / 2;
    // gfx_copy_blend(screen->frame, pts_sprite, screen->pointer.x - pts_disp, screen->pointer.y - pts_disp);
    screen->ino->ops->flip(screen->ino);
}

int _left_shift_grid[] = {
    WMGR_GRID_LEFT, 0, 0, WMGR_GRID_LEFT, 0, 0,
    WMGR_GRID_LEFT, WMGR_GRID_TOP_LEFT, 0,
    WMGR_GRID_BOTTOM_LEFT, 0, WMGR_GRID_TOP_LEFT,
    WMGR_GRID_BOTTOM_LEFT, WMGR_GRID_TOP_LEFT,
    WMGR_GRID_TOP_LEFT, WMGR_GRID_TOP_LEFT,
};
int _right_shift_grid[] = {
    WMGR_GRID_RIGHT, 0, 0, WMGR_GRID_RIGHT, 0, 0,
    WMGR_GRID_RIGHT, WMGR_GRID_TOP_RIGHT, 0,
    WMGR_GRID_BOTTOM_RIGHT, 0, WMGR_GRID_TOP_RIGHT,
    WMGR_GRID_BOTTOM_RIGHT, WMGR_GRID_TOP_RIGHT,
    WMGR_GRID_TOP_RIGHT, WMGR_GRID_TOP_RIGHT,
};
int _top_shift_grid[] = {
    WMGR_GRID_TOP, 0, 0, WMGR_GRID_TOP, 0, 0,
    WMGR_GRID_TOP_RIGHT, WMGR_GRID_TOP_LEFT, 0,
    WMGR_GRID_TOP, 0, WMGR_GRID_FULL,
    WMGR_GRID_TOP_RIGHT, WMGR_GRID_TOP_LEFT,
    WMGR_GRID_TOP_RIGHT, WMGR_GRID_TOP_LEFT,
};
int _bottom_shift_grid[] = {
    WMGR_GRID_BOTTOM, 0, 0, WMGR_GRID_BOTTOM, 0, 0,
    WMGR_GRID_BOTTOM_RIGHT, WMGR_GRID_BOTTOM_LEFT, 0,
    WMGR_GRID_NORMAL, 0, WMGR_GRID_BOTTOM,
    WMGR_GRID_BOTTOM_RIGHT, WMGR_GRID_BOTTOM_LEFT,
    WMGR_GRID_BOTTOM_RIGHT, WMGR_GRID_BOTTOM_LEFT,
};


void wmgr_keyboard(desktop_t *desk, uint32_t fkey, int state)
{
    window_t *win = ll_last(&desk->windows, window_t, node);
    desk->kbd_status = fkey >> 16;
    uint32_t key = fkey & 0xFFFF;
    if (state) {
        if ((desk->kbd_status & 0770) == 010) {
            kprintf(-1, "SHIFT + %x\n", key); // SHIFT
        } else if ((desk->kbd_status & 0770) == 040) {
            kprintf(-1, "ALT + %x\n", key); // ALT
            if (key == 0xF) {// TAB
                // TODO -- Open small window that leave on ALT-up
                if (win != NULL) {
                    ll_remove(&desk->windows, &win->node);
                    ll_push_front(&desk->windows, &win->node);
                    win = NULL;
                }
            }
        } else if ((desk->kbd_status & 0770) == 0100) {
            kprintf(-1, "CTRL + %x\n", key); // CTRL
        } else if ((desk->kbd_status & 0770) == 0110) {
            kprintf(-1, "CTRL + SHIFT + %x\n", key); // CTRL + SHIFT
        } else if ((desk->kbd_status & 0770) == 0140) {
            kprintf(-1, "CTRL + ALT + %x\n", key); // CTRL + ALT
        } else if ((desk->kbd_status & 0770) == 0050) {
            kprintf(-1, "ALT + SHIFT + %x\n", key); // CTRL + ALT
        } else if ((desk->kbd_status & 0770) == 0200) {
            kprintf(-1, "HOME + %x\n", key); // HOME
            if (key == 0xF) {// TAB
                // TODO -- Open small window that leave on ALT-up
                if (win != NULL) {
                    ll_remove(&desk->windows, &win->node);
                    ll_push_front(&desk->windows, &win->node);
                    win = NULL;
                }
            }
            if (win != NULL) {
                if (key == 0x4B)
                    win->rq_grid = WMGR_GRID_LEFT;
                else if (key == 0x48)
                    win->rq_grid = win->rq_grid == WMGR_GRID_TOP ? WMGR_GRID_FULL : WMGR_GRID_TOP;
                else if (key == 0x4d)
                    win->rq_grid = WMGR_GRID_RIGHT;
                else if (key == 0x50)
                    win->rq_grid = win->rq_grid == WMGR_GRID_BOTTOM ? 0 : WMGR_GRID_BOTTOM;
                win = NULL;
            }
        } else if ((desk->kbd_status & 0770) == 0210) {
            kprintf(-1, "HOME + SHIFT + %x\n", key); // HOME + SHIFT
            if (win != NULL) {
                if (key == 0x4B)
                    win->rq_grid = _left_shift_grid[win->rq_grid];
                else if (key == 0x48)
                    win->rq_grid = _top_shift_grid[win->rq_grid];
                else if (key == 0x4d)
                    win->rq_grid = _right_shift_grid[win->rq_grid];
                else if (key == 0x50)
                    win->rq_grid = _bottom_shift_grid[win->rq_grid];
                win = NULL;
            }
        }
    }

    if (win != NULL) {
        // TODO -- if win is TTY, send to TTY!
        event_t event;
        event.type = state ? 3 : 4;
        event.param1 = key;
        event.param2 = fkey;
        pipe_write(win->pipe, (char *)&event, sizeof(event), 0);
    }
}

window_t *wmgr_over(desktop_t *desk, int *where)
{
    window_t *win;
    for ll_each_reverse(&desk->windows, win, window_t, node) {
        if (win->grid < 0)
            continue;
        rect_t sz = wmgr_window_size(win);
        if (desk->pointer.x < sz.x - win->ml)
            continue;
        if (desk->pointer.y < sz.y - win->mt)
            continue;
        if (desk->pointer.x > sz.x + sz.w + win->ml + win->mr)
            continue;
        if (desk->pointer.y > sz.y + sz.h + win->mt + win->mb)
            continue;
        if (where != NULL) {
            if (desk->pointer.y < sz.y + DESK_BORDER) {
                if (desk->pointer.x < sz.x + DESK_BORDER)
                    *where = DESK_GRAB_TOP | DESK_GRAB_LEFT;
                else if (desk->pointer.x > sz.x + sz.w + win->ml - DESK_BORDER)
                    *where = DESK_GRAB_TOP | DESK_GRAB_RIGHT;
                else
                    *where = DESK_GRAB_TOP;
            } else if (desk->pointer.y > sz.y + win->mt - DESK_BORDER) {
                if (desk->pointer.x < sz.x + DESK_BORDER)
                    *where = DESK_GRAB_BOTTOM | DESK_GRAB_LEFT;
                else if (desk->pointer.x > sz.x + sz.w + win->ml - DESK_BORDER)
                    *where = DESK_GRAB_BOTTOM | DESK_GRAB_RIGHT;
                else
                    *where = DESK_GRAB_BOTTOM;
            } else if (desk->pointer.x < sz.x + DESK_BORDER)
                *where = DESK_GRAB_LEFT;
            else if (desk->pointer.x > sz.x + sz.w + win->ml - DESK_BORDER)
                *where = DESK_GRAB_RIGHT;
            else if (desk->pointer.y < sz.y)
                *where = DESK_GRAB_HEADER;
            else
                *where = DESK_GRAB_WINDOW;
        }
        return win;
    }
    return NULL;
}

void wmgr_pointer(desktop_t *desk, int relx, int rely)
{
    desk->pointer.x += relx;
    desk->pointer.y += rely;
    desk->pointer.size = 64;
    desk->pointer.idx = 0;
    if (desk->grab_win != NULL) {
        if (desk->grab_type == DESK_GRAB_HEADER) {
            if (desk->grab_win->grid == 0) {
                desk->grab_win->rq.x += relx;
                desk->grab_win->rq.y += rely;
            } else if (abs(desk->grab_x - desk->pointer.y) + abs(desk->grab_y - desk->pointer.y)) {
                desk->grab_win->rq_grid = 0;
                // TODO, move window under cursor
            }
        } else if (desk->grab_win->grid == 0) {
            if (desk->grab_type & DESK_GRAB_LEFT) {
                desk->grab_win->rq.x += relx;
                desk->grab_win->rq.w -= relx;
            }
            if (desk->grab_type & DESK_GRAB_TOP) {
                desk->grab_win->rq.y += rely;
                desk->grab_win->rq.h -= rely;
            }
            if (desk->grab_type & DESK_GRAB_RIGHT)
                desk->grab_win->rq.w += relx;
            if (desk->grab_type & DESK_GRAB_BOTTOM)
                desk->grab_win->rq.h += rely;
        }
    }
    window_t *win = ll_last(&desk->windows, window_t, node);
    window_t *over = wmgr_over(desk, NULL);
    if (desk->over_win && over != desk->over_win) {
        kprintf(-1, "MOUSE MOVE %d:  %d, %d\n", desk->over_win->no, relx, rely);
        kprintf(-1, "MOUSE EXIT %d\n", desk->over_win->no);
    }
    if (over && over != desk->over_win) {
        kprintf(-1, "MOUSE ENTER %d\n", over->no);
        kprintf(-1, "MOUSE MOVE %d:  %d, %d\n", over->no, relx, rely);
    }
    if (win && ((win != over && win != desk->over_win) || (win == over && win == desk->over_win)))
        kprintf(-1, "MOUSE MOVE %d:  %d, %d\n", win->no, relx, rely);
    if (desk->over_win && win != desk->over_win && over == desk->over_win)
        kprintf(-1, "MOUSE MOVE %d:  %d, %d\n", over->no, relx, rely);
    desk->over_win = over;
}


void wmgr_button(desktop_t *desk, int btn, int state)
{
    window_t *win = ll_last(&desk->windows, window_t, node);
    if (state)
        desk->btn_status |= btn;
    else
        desk->btn_status &= ~btn;

    int wh = 0;
    window_t *over;
    if (state) {
        over = wmgr_over(desk, &wh);
        if (over != win) {
            if (over != NULL) {
                ll_remove(&desk->windows, &over->node);
                ll_push_back(&desk->windows, &over->node);
            }
            win = over;
        }
        if (over != NULL && btn == 1 && wh != 0) {
            // Grab
            desk->grab_win = over;
            desk->grab_type = wh;
            desk->grab_x = desk->pointer.x;
            desk->grab_y = desk->pointer.y;
            win = NULL;
        }
    } else {
        over = wmgr_over(desk, NULL);
        if (over != win) {
        }
        desk->grab_type = 0; // TODO
    }
    if (win != NULL)
        kprintf(-1, "MOUSE %s %d: 0%o\n", state ? "DOWN" : "UP", win->no, btn);
}


void wmgr_input(inode_t *ino, int type, int param, pipe_t *pipe)
{
    if (kDESK == NULL)
        return;
    // clock64_t now = kclock();
    // if (type == EV_KEY_PRESS) {
    //     if (param == kDESK->last_key) {
    //         clock64_t elapsed = now - kDESK->last_kbd;
    //         if (elapsed < MSEC_TO_KTIME(500))
    //             return;
    //     }
    //     kDESK->last_key = param;
    //     kDESK->last_kbd = now;
    // }
    switch (type) {
    case EV_KEY_PRESS:
        wmgr_keyboard(kDESK, param, 1);
        return;
    case EV_KEY_RELEASE:
        wmgr_keyboard(kDESK, param, 0);
        return;

    }
}

