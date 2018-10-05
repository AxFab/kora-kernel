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
#include <errno.h>
#include <kernel/vfs.h>
#include "../check.h"

int vfs_readlink(inode_t *ino, char *buf, int len, int flags)
{
	return -1;
}

inode_t *vfs_open(inode_t *ino)
{
    return ino;
}

void vfs_close(inode_t *ino)
{
}

int vfs_read(inode_t *ino, void *buf, size_t lg, off_t off)
{
    errno = EIO;
    return -1;
}

int vfs_write(inode_t *ino, const void *buf, size_t lg, off_t off)
{
    errno = EIO;
    return -1;
}
