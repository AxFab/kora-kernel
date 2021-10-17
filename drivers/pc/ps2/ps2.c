/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#include <kernel/tasks.h>
#include <kernel/mods.h>

inode_t *mouse_ino;
inode_t *kdb_ino;


// PS/2 Reset
void PS2_reset()
{
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 0x80);
    outb(0x61, tmp & 0x7F);
    inb(0x60);
}


// int ps2_read(inode_t *ino, char *buf, size_t len, int flags)
// {
//     return pipe_read((pipe_t *)ino->info, buf, len, flags);
// }

ino_ops_t ps2_ino_ops = {
    .read = NULL,
    // .read = ps2_read,
};


void PS2_setup()
{
    PS2_mouse_setup();
    PS2_kdb_setup();

    kdb_ino = vfs_inode(1, FL_CHR, NULL, &ps2_ino_ops);
    kdb_ino->dev->block = sizeof(evmsg_t);
    kdb_ino->dev->flags = FD_RDONLY;
    kdb_ino->dev->devclass = strdup("PS/2 Keyboard");
    vfs_mkdev(kdb_ino, "kbd");
    vfs_close_inode(kdb_ino);

    // TODO - Remove this hack!
    itimer_create(kdb_ino, MSEC_TO_USEC(100), MSEC_TO_USEC(40));

    mouse_ino = vfs_inode(2, FL_CHR, NULL, &ps2_ino_ops);
    mouse_ino->dev->block = sizeof(evmsg_t);
    mouse_ino->dev->flags = FD_RDONLY;
    mouse_ino->dev->devclass = strdup("PS/2 Mouse");
    vfs_mkdev(mouse_ino, "mouse");
    vfs_close_inode(mouse_ino);

    irq_register(1, (irq_handler_t)PS2_kdb_handler, NULL);
    irq_register(12, (irq_handler_t)PS2_mouse_handler, NULL);
}

void PS2_teardown()
{
    /*
    vfs_rmdev("kbd");
    vfs_rmdev("mouse");
    */
}


EXPORT_MODULE(ps2, PS2_setup, PS2_teardown);
