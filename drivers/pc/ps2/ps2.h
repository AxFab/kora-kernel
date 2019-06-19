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
#ifndef _SRC_DRV_PS2_H
#define _SRC_DRV_PS2_H 1

#include <kernel/core.h>
#include <kernel/input.h>
#include <kernel/files.h>
#include <kernel/device.h>
#include <kernel/cpu.h>

extern inode_t *mouse_ino;
extern inode_t *kdb_ino;

void PS2_event(inode_t *ino, uint8_t type, int32_t param1, int32_t param2);

void PS2_kdb_handler();

void PS2_mouse_handler();
void PS2_mouse_setup();


typedef struct gfx_msg {
    int64_t timestamp;
    int32_t param1;
    int32_t param2;
    uint16_t message;
    uint16_t unsued;
} gfx_msg_t;


#endif  /* _SRC_DRV_PS2_H */
