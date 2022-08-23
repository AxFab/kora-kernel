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
#include "ext2.h"

int ext2_chmod(inode_t *ino, int mode)
{
    struct bkmap bk_new;
    ext2_volume_t *vol = (ext2_volume_t *)ino->drv_data;
    ext2_ino_t *ino_new = ext2_entry(&bk_new, vol, ino->no, VM_WR);

    ino_new->mode = (ino_new->mode & EXT2_S_IFMT) | (mode & 0x1FF); // TODO -- extra mode bits
    ino->mode = mode & 0x1FF;

    bkunmap(&bk_new);
    return 0;
}

