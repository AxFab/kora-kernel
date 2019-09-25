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
    pipe_t *pipe = (pipe_t *)ino->info;

    event_t ev;
    // ev.time = time(NULL);
    ev.message = type;
    ev.param1 = param1;
    ev.param2 = param2;
    //(void)ev;
    pipe_write(pipe, (char *)&ev, sizeof(ev), 0);

    //wmgr_event(&ev);
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

int ps2_read(inode_t *ino, char *buf, size_t len, int flags)
{
    return pipe_read((pipe_t *)ino->info, buf, len, flags);
}

dev_ops_t ps2_kdb_dev_ops = {

};

ino_ops_t ps2_ino_ops = {
    .read = ps2_read
};


void itimer_create(pipe_t *pipe, long delay, long interval);

void PS2_setup()
{
    irq_register(1, (irq_handler_t)PS2_kdb_handler, NULL);
    irq_register(12, (irq_handler_t)PS2_mouse_handler, NULL);
    PS2_mouse_setup();
    PS2_kdb_setup();

    kdb_ino = vfs_inode(1, FL_CHR, NULL);
    kdb_ino->dev->block = sizeof(event_t);
    kdb_ino->dev->flags = VFS_RDONLY;
    kdb_ino->dev->ops = &ps2_kdb_dev_ops;
    kdb_ino->ops = &ps2_ino_ops;
    kdb_ino->dev->devclass = (char *)"PS/2 Keyboard";
    kdb_ino->info = pipe_create();
    vfs_mkdev(kdb_ino, "kdb");
    vfs_close(kdb_ino);

    itimer_create(kdb_ino->info, MSEC_TO_KTIME(100), MSEC_TO_KTIME(40));

    mouse_ino = vfs_inode(2, FL_CHR, NULL);
    mouse_ino->dev->block = sizeof(event_t);
    mouse_ino->dev->flags = VFS_RDONLY;
    mouse_ino->dev->ops = &ps2_kdb_dev_ops;
    mouse_ino->ops = &ps2_ino_ops;
    mouse_ino->dev->devclass = (char *)"PS/2 Mouse";
    mouse_ino->info = pipe_create();
    vfs_mkdev(mouse_ino, "mouse");
    vfs_close(mouse_ino);

}

void PS2_teardown()
{
    /*
    vfs_rmdev("kdb");
    vfs_rmdev("mouse");
    */
}


MODULE(ps2, PS2_setup, PS2_teardown);
