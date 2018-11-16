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
#include "ps2.h"

#define KEY_CAPS_LOCK  0x3A
#define KEY_NUM_LOCK  0x45
#define KEY_SCROLL_LOCK  0x46
#define KEY_SHIFT_L  0x2A
#define KEY_SHIFT_R  0x36
#define KEY_CTRL_L  0x1D
#define KEY_CTRL_R  0x61
#define KEY_ALT  0x38
#define KEY_ALTGR  0x64
#define KEY_HOST  0x5b

uint8_t kdb_status = 0;


static void PS2_kbd_led(uint8_t status)
{
    while (inb(0x64) & 0x2);
    outb(0x60, 0xed);
    while (inb(0x64) & 0x2);
    outb(0x60, status & 7);
}


void PS2_kdb_handler()
{
    char c = 0;
    for (;;) {
        if (inb(0x60) != c)
            break;
    }

    c = inb(0x60);
    if (c > 0) {
        if (c == KEY_CAPS_LOCK) {
            kdb_status ^= KDB_CAPS_LOCK;
            PS2_kbd_led(kdb_status);
        } else if (c == KEY_NUM_LOCK) {
            kdb_status ^= KDB_NUM_LOCK;
            PS2_kbd_led(kdb_status);
        } else if (c == KEY_SCROLL_LOCK) {
            kdb_status ^= KDB_SCROLL_LOCK;
            PS2_kbd_led(kdb_status);
        } else if (c == KEY_SHIFT_L || c == KEY_SHIFT_R)
            kdb_status |= KDB_SHIFT;

        else if (c == KEY_CTRL_L || c == KEY_CTRL_R)
            kdb_status |= KDB_CTRL;

        else if (c == KEY_ALT)
            kdb_status |= KDB_ALT;

        else if (c == KEY_ALTGR)
            kdb_status |= KDB_ALTGR;

        else if (c == KEY_HOST)
            kdb_status |= KDB_HOST;


        PS2_event(kdb_ino, EV_KEY_PRESS, 0, (kdb_status << 16) | c);

    } else {
        c &= 0x7F;
        if (c == KEY_SHIFT_L || c == KEY_SHIFT_R)
            kdb_status &= ~KDB_SHIFT;

        else if (c == KEY_CTRL_L || c == KEY_CTRL_R)
            kdb_status &= ~KDB_CTRL;

        else if (c == KEY_ALT)
            kdb_status &= ~KDB_ALT;

        else if (c == KEY_ALTGR)
            kdb_status &= ~KDB_ALTGR;

        else if (c == KEY_HOST)
            kdb_status &= ~KDB_HOST;

        PS2_event(kdb_ino, EV_KEY_RELEASE, 0, (kdb_status << 16) | c);
    }
}
