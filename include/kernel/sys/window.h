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
#ifndef _KERNEL_SYS_WINDOW_H
#define _KERNEL_SYS_WINDOW_H 1

#include <kernel/sys/inode.h>
#include <kora/llist.h>

struct surface {
    int width;
    int height;
    int pitch;
    // int full_height;
    int format;
    uint8_t *pixels;
    int features;
    int events;
    int x, y;
    int depth;
    size_t size;
    llnode_t node;
    inode_t *ino;
    // Screen... inputs...user...state...desktop...
};




#endif  /* _KERNEL_SYS_WINDOW_H */
