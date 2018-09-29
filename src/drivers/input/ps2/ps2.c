/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <kernel/files.h>

inode_t *mouse_ino;
inode_t *kdb_ino;

device_t ps2_kbd;
device_t ps2_mse;

void PS2_event(inode_t *ino, uint8_t type, int32_t param1, int32_t param2)
{
    // if (seat_event(type, param1, param2) != 0) {
    //     return;
    // }

    event_t ev;
    // ev.time = time(NULL);
    ev.type = type;
    ev.param1 = param1;
    ev.param2 = param2;
    (void)ev;
    wmgr_event(&ev);
    // dev_char_write(ino, &ev, sizeof(ev));
}

// PS/2 Reset
void PS2_reset()
{
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 0x80);
    outb(0x61, tmp & 0x7F);
    inb(0x60);
}

// int PS2_irq()
// {
//     uint8_t r;
//     while ((r = inb(0x64)) & 1) {
//         if (r & 0x20) {
//             PS2_mouse_handler();
//         } else {
//             PS2_kdb_handler();
//         }
//     }
//     return 0;
// }


void PS2_setup()
{
    irq_register(1, (irq_handler_t)PS2_kdb_handler, NULL);
    irq_register(12, (irq_handler_t)PS2_mouse_handler, NULL);
    PS2_mouse_setup();

    kdb_ino = vfs_inode(1, S_IFCHR | 700, NULL, 0);
    ps2_kbd.block = sizeof(event_t);
    ps2_kbd.class = "PS/2 Keyboard";
    vfs_mkdev("kdb", &ps2_kbd, kdb_ino);
    vfs_close(kdb_ino);

    mouse_ino = vfs_inode(2, S_IFCHR | 700, NULL, 0);
    ps2_mse.block = sizeof(event_t);
    ps2_mse.class = "PS/2 Mouse";
    vfs_mkdev("mouse", &ps2_mse, mouse_ino);
    vfs_close(mouse_ino);
}

void PS2_teardown()
{
    vfs_rmdev("kdb");
    vfs_rmdev("mouse");
}


MODULE(ps2, MOD_AGPL, PS2_setup, PS2_teardown);
