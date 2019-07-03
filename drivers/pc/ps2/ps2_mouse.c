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
#include "ps2.h"

uint8_t mouse_cycle = 0;
uint8_t mouse_byte[3];
int mouse_x = 0;
int mouse_y = 0;
uint8_t mouse_btn = 0;

#define MOUSE_PORT   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_WRITE  0xD4
#define MOUSE_DATA_BIT 1
#define MOUSE_SIG_BIT  2
#define MOUSE_F_BIT  0x20
#define MOUSE_V_BIT  0x08


static inline void PS2_mouse_wait(uint8_t bit)
{
    int timeout = 100000;
    while (timeout--) {
        if (inb(MOUSE_STATUS) & bit)
            break;
    }
}

static inline void PS2_mouse_wait_signal()
{
    int timeout = 100000;
    while (timeout--) {
        if (inb(MOUSE_STATUS) & 2)
            break;
    }
}

static inline void PS2_mouse_write(uint8_t a)
{
    //Wait to be able to send a command
    PS2_mouse_wait(MOUSE_SIG_BIT);
    //Tell the mouse we are sending a command
    outb(MOUSE_STATUS, MOUSE_WRITE);
    //Wait for the final part
    PS2_mouse_wait(MOUSE_SIG_BIT);
    //Finally write
    outb(MOUSE_PORT, a);
}

static inline uint8_t PS2_mouse_read()
{
    //Get's response from mouse
    PS2_mouse_wait(MOUSE_DATA_BIT);
    return inb(MOUSE_PORT);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int mseX = 0;
int mseY = 0;

//Mouse functions
void PS2_mouse_handler()
{
    evmsg_t msg;
    pipe_t *kdb_buffer = (pipe_t *)kdb_ino->info;

    uint8_t status = inb(MOUSE_STATUS);
    while (status & MOUSE_DATA_BIT && status & MOUSE_F_BIT) {
        uint8_t byte = inb(MOUSE_PORT);
        switch (mouse_cycle) {
        case 0:
            mouse_byte[0] = byte;
            if (byte & MOUSE_V_BIT)
                mouse_cycle++;
            break;
        case 1:
            mouse_byte[1] = byte;
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = byte;
            mouse_x = mouse_byte[1];
            mouse_y = mouse_byte[2];
            mouse_cycle = 0;

            if (mouse_byte[0] & 0x10)
                mouse_x = mouse_x - 0x100;
            if (mouse_byte[0] & 0x20)
                mouse_y = mouse_y - 0x100;
            if (mouse_byte[0] & 0x40 || mouse_byte[0] & 0x80)
                mouse_x = mouse_y = 0; // Overflow
            if (mouse_x != 0 || mouse_y != 0) {
                mouse_y = -mouse_y;
                mseX = MIN(1280, MAX(0, mseX + mouse_x));
                mseY = MIN(780, MAX(0, mseY + mouse_y));
                msg.param1 = mouse_x;
                msg.param2 = mouse_y;
                msg.message = EV_MOUSEMOVE;
                pipe_write(kdb_buffer, &msg, sizeof(msg), IO_ATOMIC);

            }
            if (mouse_btn != (mouse_byte[0] & 7)) {
                int diff = mouse_btn ^ mouse_byte[0] & 7;
                mouse_btn = mouse_byte[0] & 7;
                // msg.param1 = diff;
                // msg.param2 = mouse_btn;
                // msg.message = EV_MOUSEBTN;
                // pipe_write(kdb_buffer, &msg, sizeof(msg), IO_ATOMIC);
            }

            return;
        }
    }
}

void PS2_mouse_setup()
{
    uint8_t status;

    //Enable the auxiliary mouse device
    PS2_mouse_wait(MOUSE_SIG_BIT);
    outb(MOUSE_STATUS, 0xA8);
    PS2_mouse_read();  //Acknowledge

    //Enable the interrupts
    PS2_mouse_wait(MOUSE_SIG_BIT);
    outb(MOUSE_STATUS, 0x20);

    status = PS2_mouse_read() | 3;

    PS2_mouse_wait(MOUSE_SIG_BIT);
    outb(MOUSE_STATUS, 0x60);

    PS2_mouse_wait(MOUSE_SIG_BIT);
    outb(MOUSE_PORT, status);

    //Tell the mouse to use default settings
    PS2_mouse_write(0xF6);
    PS2_mouse_read();  //Acknowledge

    //Enable the mouse
    PS2_mouse_write(0xF4);
    PS2_mouse_read();  //Acknowledge

    // TODO hot-plugin -- When a mouse is plugged into a running system it may send a 0xAA, then a 0x00 byte, and then go into default state (see below).
}
