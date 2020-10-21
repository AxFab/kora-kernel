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
#include <kernel/stdc.h>
#include <kernel/vfs.h>


typedef struct socket socket_t;


int socket_read(inode_t *ino, char *buf, size_t len, xoff_t off, int flags)
{
    return -1;
}

int socket_write(inode_t *ino, const char *buf, size_t len, xoff_t off, int flags)
{
    return -1;
}

void socket_destroy(inode_t *ino)
{
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

fl_ops_t socket_ops = {
    .read = socket_read,
    .write = socket_write,
    .destroy = socket_destroy,
};

socket_t *socket_create()
{
    return NULL;
}