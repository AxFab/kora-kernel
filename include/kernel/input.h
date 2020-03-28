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
#ifndef _KERNEL_INPUT_H
#define _KERNEL_INPUT_H 1

#include <kernel/vfs.h>

/*
 * Keyboard state flags
 */
#define KDB_SCROLL_LOCK   (1 << 0)
#define KDB_NUM_LOCK      (1 << 1)
#define KDB_CAPS_LOCK     (1 << 2)
#define KDB_SHIFT         (1 << 3)
#define KDB_ALTGR         (1 << 4)
#define KDB_ALT           (1 << 5)
#define KDB_CTRL          (1 << 6)
#define KDB_HOST          (1 << 7)


/*
 * Event types
 */
enum gfx_event {
    GFX_EV_QUIT = 0,
    GFX_EV_MOUSEMOVE,
    GFX_EV_BTNUP,
    GFX_EV_BTNDOWN,
    GFX_EV_KEYUP,
    GFX_EV_KEYDOWN,
    GFX_EV_MOUSEWHEEL,
    GFX_EV_TIMER,
    GFX_EV_RESIZE,
    GFX_EV_PAINT,
    GFX_EV_UNICODE,
    GFX_EV_DELAY = 128,
};

typedef struct event evmsg_t;

// struct kUsr {
//     /* Keyboard repeat */
//     uint16_t key_last;
//     uint16_t key_ticks;
// };

PACK(struct event {
    uint16_t message;
    uint16_t window;
    uint32_t param1;
    uint32_t param2;
});


// int seat_event(uint8_t type, uint32_t param1, uint16_t param2);


// extern struct kUsr kUSR;


// void surface_copy(surface_t *dest, surface_t *src, int relx, int rely);
// void surface_fill_rect(surface_t *scr, int x, int y, int width, int height, uint32_t rgb);
// void surface_draw(surface_t *scr, int width, int height, uint32_t *color, uint8_t *px);
// void seat_ticks();
// surface_t *seat_screen(int no);
// inode_t *seat_surface(int width, int height, int format, int features, int events);
// inode_t *seat_framebuf(int width, int height, int format, int pitch, void* mmio);
// void seat_fb0(int width, int height, int format, int pitch, void* mmio);
// inode_t *seat_initscreen();


#endif  /* _KERNEL_INPUT_H */
