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
#include <kernel/vfs.h>
#include <kernel/files.h>

int pipe_fcntl(inode_t *ino, int cmd, void *flags)
{
    return -1;
}


int pipe_close(inode_t *ino)
{
    pipe_destroy((pipe_t *) ino->info);
    return 0;
}

int pipe_read_ino(inode_t *ino, char *buf, int len, int flags)
{
    return pipe_read((pipe_t *) ino->info, buf, len, flags);
}

int pipe_write_ino(inode_t *ino, const char *buf, int len, int flags)
{
    return pipe_write((pipe_t *) ino->info, buf, len, flags);
}


ino_ops_t pipe_ops = {
    .fcntl = pipe_fcntl,
    .close = pipe_close,
    .read = pipe_read_ino,
    .write = pipe_write_ino,
};

inode_t *pipe_inode()
{
    inode_t *ino = vfs_inode();
    ino->info = pipe_create();
    ino->ops = &pipe_ops;
    // TODO, vfs_config
    return ino;
}
>
