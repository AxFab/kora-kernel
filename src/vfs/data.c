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
#include <kernel/vfs.h>
#include <kernel/device.h>
#include <string.h>
#include <errno.h>



int vfs_read(inode_t *ino, char *buf, size_t size, off_t off, int flags)
{

}

int vfs_write(inode_t *ino, const char *buf, size_t size, off_t off, int flags)
{

}

page_t mem_fetch(inode_t *ino, off_t off)
{
    if (ino->ops->fetch == NULL)
        return 0;
    return ino->ops->fetch(ino, off);
}

void mem_sync(inode_t *ino, off_t off, page_t pg)
{
    if (ino->ops->sync == NULL)
        return;
    ino->ops->sync(ino, off, pg);
}

void mem_release(inode_t *ino, off_t off, page_t pg)
{
    if (ino->ops->release == NULL)
        return;
    ino->ops->release(ino, off, pg);
}

