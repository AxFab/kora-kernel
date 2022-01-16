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
#include "../cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <kora/mcrs.h>
#include <kernel/stdc.h>
#include <kernel/vfs.h>

#if !defined(_WIN32) // kernel/src/tests/check.h
# define _valloc(s)  valloc(s)
# define _vfree(p)  free(p)
#else
# define _valloc(s)  _aligned_malloc(s, PAGE_SIZE)
# define _vfree(p)  _aligned_free(p)
#endif


void do_dump(vfs_t **vfs, size_t *param);
void do_umask(vfs_t **vfs, size_t *param);
void do_ls(vfs_t** fs, size_t* param);
void do_stat(vfs_t** fs, size_t* param);
void do_cd(vfs_t** fs, size_t* param);
void do_chroot(vfs_t** fs, size_t* param);
void do_pwd(vfs_t** fs, size_t* param);
void do_open(vfs_t** fs, size_t* param);
void do_close(vfs_t** fs, size_t* param);
void do_look(vfs_t** fs, size_t* param);
void do_create(vfs_t** fs, size_t* param);
void do_unlink(vfs_t** fs, size_t* param);
void do_mkdir(vfs_t** fs, size_t* param);
void do_rmdir(vfs_t** fs, size_t* param);
void do_dd(vfs_t** fs, size_t* param);
void do_mount(vfs_t** fs, size_t* param);
void do_unmount(vfs_t** fs, size_t* param);
void do_extract(vfs_t** fs, size_t* param);
void do_checksum(vfs_t** vfs, size_t* param);
void do_img_open(vfs_t** fs, size_t* param);
void do_img_create(vfs_t** fs, size_t* param);
void do_img_copy(vfs_t** fs, size_t* param);
void do_img_remove(vfs_t** vfs, size_t* param);
void do_tar_mount(vfs_t** vfs, size_t* param);
void do_format(vfs_t** vfs, size_t* param);

#ifdef _EMBEDED_FS
void fat_setup();
void fat_teardown();
void isofs_setup();
void isofs_teardown();
void ext2_setup();
void ext2_teardown();
#endif


vfs_t *VFS;

void initialize()
{
    VFS = vfs_init();
#ifdef _EMBEDED_FS
    fat_setup();
    isofs_setup();
    ext2_setup();
#endif
}

void teardown()
{
#ifdef _EMBEDED_FS
    fat_teardown();
    isofs_teardown();
    ext2_teardown();
#endif
    vfs_sweep(VFS);
}

void do_load(vfs_t* fs, size_t* param)
{

}

void do_restart(vfs_t* fs, size_t* param)
{
    teardown();
    // TODO -- Check all is clean
    initialize();
}


cli_cmd_t __commands[] = {
    { "RESTART", "", { 0, 0, 0, 0, 0 }, do_restart, 1 },
    { "DUMP", "", { 0, 0, 0, 0, 0 }, do_dump, 1 },
    { "UMASK", "", { ARG_INT, 0, 0, 0, 0 }, do_umask, 1 },
    { "LS", "", { ARG_STR, ARG_INO, ARG_FLG, 0, 0 }, do_ls, 1 },
    { "STAT", "", { ARG_STR, ARG_STR, ARG_INT, ARG_STR, ARG_STR }, do_stat, 1 },
    { "CD", "", { ARG_STR, ARG_INO, ARG_FLG, 0, 0 }, do_cd, 1 },
    { "CHROOT", "", { ARG_STR, ARG_INO, ARG_FLG, 0, 0 }, do_chroot, 1 },
    { "PWD", "", { 0, 0, 0, 0, 0 }, do_pwd, 0 },
    { "OPEN", "", { ARG_STR, ARG_INO, ARG_FLG, 0, 0 }, do_open, 1 },
    { "CLOSE", "", { ARG_INO, ARG_FLG, 0, 0 }, do_close, 1 },
    { "LOOK", "", { ARG_STR, ARG_INO, ARG_FLG, 0, 0 }, do_look, 1 },

    { "CREAT", "", { ARG_STR, ARG_INO, ARG_FLG, 0, 0 }, do_create, 1 },
    { "UNLINK", "", { ARG_STR, ARG_INO, ARG_FLG, 0, 0 }, do_unlink, 1 },
    // LINK, RENAME, SYMLINK, READLINK ...
    { "MKDIR", "", { ARG_STR, ARG_INT, 0, 0, 0 }, do_mkdir, 1 },
    { "RMDIR", "", { ARG_STR, ARG_INO, ARG_FLG, 0, 0 }, do_rmdir, 1 },

    { "DD", "", { ARG_STR, ARG_STR, ARG_INT, 0, 0 }, do_dd, 3 },

    { "MOUNT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, do_mount, 3 },
    { "UNMOUNT", "", { ARG_STR, ARG_INO, ARG_FLG, 0, 0 }, do_unmount, 1 },

    // CHMOD, CHOWN, UTIMES, TRUNCATE ...
    // OPEN DIR, READ DIR, CLOSE DIR ...
    // READ, WRITE ...
    // ACCESS ...
    // FDISK !?
    // IO <ino> <lba> [count] [flags] 
    // IOCTL <ino> <cmd> [..?]

    { "EXTRACT", "", { ARG_STR, ARG_STR, ARG_INO, ARG_FLG, 0 }, do_extract, 1 },
    { "CKSUM", "", { ARG_STR, ARG_STR, ARG_INO, ARG_FLG, 0 }, do_checksum, 1},

    { "IMG_OPEN", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, do_img_open, 2 },
    { "IMG_CREATE", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, do_img_create, 2 },
    { "IMG_COPY", "", { ARG_STR, ARG_STR, ARG_INT, 0, 0 }, do_img_copy, 2 },
    { "IMG_RM", "",  { ARG_STR, 0, 0, 0, 0 }, do_img_remove, 1 },

    { "TAR", "", { ARG_STR, ARG_STR, 0, 0, 0 }, do_tar_mount, 2 },

    { "FORMAT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, do_format, 2 },
    CLI_END_OF_COMMANDS,
};

int main(int argc, char **argv)
{
    return cli_main(argc, argv, &VFS, initialize, teardown);
}
