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
#ifndef _KERNEL_INPUT_H
#define _KERNEL_INPUT_H 1


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
enum {
    EV_UNDEF = 0,
    EV_MOUSE_BTN,
    EV_MOUSE_MOTION,
    EV_KEY_PRESS,
    EV_KEY_RELEASE,
    EV_KEY_REPEAT,
};


struct kUsr {
    /* Keyboard repeat */
    uint16_t key_last;
    uint16_t key_ticks;
};

typedef struct event event_t;
struct event {
    uint32_t tm_sec;
    uint32_t tm_nsec;
    uint32_t param1;
    uint16_t param2;
    uint8_t type;
    uint8_t unsued;
};


int seat_event(uint8_t type, uint32_t param1, uint16_t param2);
void seat_ticks();

extern struct kUsr kUSR;



#endif  /* _KERNEL_INPUT_H */
