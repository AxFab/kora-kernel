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



void page_release_kmap_stub(page_t page);
void page_release(page_t page)
{
    page_release_kmap_stub(page);
}

page_t mmu_read_kmap_stub(size_t address);
page_t mmu_read(size_t vaddr)
{
    return mmu_read_kmap_stub(vaddr);
}


int do_dump(vfs_t **ctx, size_t *param);
int do_umask(vfs_t **ctx, size_t *param);
int do_ls(vfs_t **ctx, size_t *param);
int do_stat(vfs_t **ctx, size_t *param);
int do_lstat(vfs_t **ctx, size_t *param);
int do_link(vfs_t **ctx, size_t *param);
int do_links(vfs_t **ctx, size_t *param);
int do_times(vfs_t **ctx, size_t *param);
int do_cd(vfs_t **ctx, size_t *param);
int do_chroot(vfs_t **ctx, size_t *param);
int do_pwd(vfs_t **ctx, size_t *param);
int do_open(vfs_t **ctx, size_t *param);
int do_close(vfs_t **ctx, size_t *param);
int do_look(vfs_t **ctx, size_t *param);
int do_delay(vfs_t **ctx, size_t *param);
int do_create(vfs_t **ctx, size_t *param);
int do_unlink(vfs_t **ctx, size_t *param);
int do_symlink(vfs_t **ctx, size_t *param);
int do_mkdir(vfs_t **ctx, size_t *param);
int do_rmdir(vfs_t **ctx, size_t *param);
int do_dd(vfs_t **ctx, size_t *param);
int do_clear_cache(vfs_t **ctx, size_t *param);
int do_mount(vfs_t **ctx, size_t *param);
int do_umount(vfs_t **ctx, size_t *param);
int do_extract(vfs_t **ctx, size_t *param);
int do_checksum(vfs_t **ctx, size_t *param);
int do_img_open(vfs_t **ctx, size_t *param);
int do_img_create(vfs_t **ctx, size_t *param);
int do_img_copy(vfs_t **ctx, size_t *param);
int do_img_remove(vfs_t **ctx, size_t *param);
int do_tar_mount(vfs_t **ctx, size_t *param);
int do_format(vfs_t **ctx, size_t *param);

void alloc_check();
void kmap_check();

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

    alloc_check();
    kmap_check();
}

void do_restart(vfs_t *fs, size_t *param)
{
    teardown();
    // TODO -- Check all is clean
    initialize();
}


cli_cmd_t __commands[] = {
    { "RESTART", "", { 0, 0, 0, 0, 0 }, (void*)do_restart, 1 },
    { "DUMP", "", { 0, 0, 0, 0, 0 }, (void*)do_dump, 1 },
    { "UMASK", "", { ARG_INT, 0, 0, 0, 0 }, (void*)do_umask, 1 },
    { "LS", "", { ARG_STR, 0, 0, 0, 0 }, (void*)do_ls, 1 },
    { "STAT", "", { ARG_STR, ARG_STR, ARG_INT, ARG_STR, ARG_STR }, (void*)do_stat, 1 },
    { "LSTAT", "", { ARG_STR, ARG_STR, ARG_INT, ARG_STR, ARG_STR }, (void*)do_lstat, 1 },
    { "LINKS", "", { ARG_STR, ARG_INT, 0, 0, 0 }, (void*)do_links, 1 },
    { "TIMES", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void*)do_times, 2 },
    { "CD", "", { ARG_STR, 0, 0, 0, 0 }, (void*)do_cd, 1 },
    { "CHROOT", "", { ARG_STR, 0, 0, 0, 0 }, (void*)do_chroot, 1 },
    { "PWD", "", { 0, 0, 0, 0, 0 }, (void*)do_pwd, 0 },
    { "OPEN", "", { ARG_STR, ARG_STR, ARG_INT, 0, 0 }, (void*)do_open, 2 },
    // { "CLOSE", "", { ARG_INO, ARG_FLG, 0, 0 }, (void*)do_close, 1 },
    // { "LOOK", "", { ARG_STR, ARG_INO, ARG_FLG, 0, 0 }, (void*)do_look, 1 },
    { "DELAY", "", { ARG_STR, 0, 0, 0, 0 }, (void*)do_delay, 0 },

    { "CREATE", "", { ARG_STR, ARG_INT, 0, 0, 0 }, (void*)do_create, 1 },
    { "UNLINK", "", { ARG_STR, 0, 0, 0, 0 }, (void*)do_unlink, 1 },
    { "SYMLINK", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_symlink, 2 },
    { "LINK", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_link, 2 },

    // LINK, RENAME, SYMLINK, READLINK ...
    { "MKDIR", "", { ARG_STR, ARG_INT, 0, 0, 0 }, (void*)do_mkdir, 1 },
    { "RMDIR", "", { ARG_STR, 0, 0, 0, 0 }, (void*)do_rmdir, 1 },

    { "DD", "", { ARG_STR, ARG_STR, ARG_INT, 0, 0 }, (void*)do_dd, 3 },
    { "CLEAR_CACHE", "", { ARG_STR, 0, 0, 0, 0 }, (void*)do_clear_cache, 1 },


    { "MOUNT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void*)do_mount, 3 },
    { "UMOUNT", "", { ARG_STR, 0, 0, 0, 0 }, (void*)do_umount, 1 },

    // CHMOD, CHOWN, UTIMES, TRUNCATE ...
    // OPEN DIR, READ DIR, CLOSE DIR ...
    // READ, WRITE ...
    // ACCESS ...
    // FDISK !?
    // IO <ino> <lba> [count] [flags] 
    // IOCTL <ino> <cmd> [..?]

    { "EXTRACT", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void*)do_extract, 1 },
    { "CKSUM", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void*)do_checksum, 1},

    { "IMG_OPEN", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void*)do_img_open, 2 },
    { "IMG_CREATE", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void*)do_img_create, 2 },
    { "IMG_COPY", "", { ARG_STR, ARG_STR, ARG_INT, 0, 0 }, (void*)do_img_copy, 2 },
    { "IMG_RM", "",  { ARG_STR, 0, 0, 0, 0 }, (void*)do_img_remove, 1 },

    { "TAR", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void*)do_tar_mount, 2 },

    { "FORMAT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void*)do_format, 2 },
    CLI_END_OF_COMMANDS,
};

int main(int argc, char **argv)
{
    return cli_main(argc, argv, &VFS, initialize, teardown);
}
