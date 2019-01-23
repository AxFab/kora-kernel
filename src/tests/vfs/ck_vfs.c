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
#include <kernel/device.h>

KMODULE(imgdk);
KMODULE(isofs);
KMODULE(fatfs);

void vfs_sweep(mountfs_t *fs, int max);

void test_01()
{
    kprintf(-1, "\n\e[94m  VFS #1 - Search, unmount and re-search\e[0m\n");

    KSETUP(imgdk);
    KSETUP(isofs);

    inode_t *root = vfs_mount("sdC", "isofs");

    inode_t *ino = vfs_search(root, NULL, "/bin/init", NULL);
    assert(ino != NULL);
    vfs_close(ino);

    vfs_umount(root);

    ino = vfs_search(root, NULL, "/bin/init", NULL);
    assert(ino == NULL);

    mountfs_t *fs = root->fs;
    vfs_close(root);

    vfs_sweep(fs, 5);

    KTEARDOWN(isofs);
    KTEARDOWN(imgdk);
}


void test_02()
{
    kprintf(-1, "\n\e[94m  VFS #2 - Read directory\e[0m\n");

    KSETUP(imgdk);
    KSETUP(isofs);

    char name[256];
    inode_t *root = vfs_mount("sdC", "isofs");

    void *dir = vfs_opendir(root, NULL);
    for (;;) {
        inode_t *ino = vfs_readdir(root, name, dir);
        if (ino == NULL)
            break;
        kprintf(0, "isofs - '%s'\n", name);
        vfs_close(ino);
    }
    vfs_closedir(root, dir);

    mountfs_t *fs = root->fs;
    vfs_close(root);

    vfs_sweep(fs, 15);
    vfs_umount(root);
    // vfs_sweep(fs, 15);

    KTEARDOWN(isofs);
    KTEARDOWN(imgdk);
}

void test_03()
{
    kprintf(-1, "\n\e[94m  VFS #3 - Format, create remove nodes\e[0m\n");

    KSETUP(imgdk);
    KSETUP(isofs);
    KSETUP(fatfs);


    // ~64Mb 131072 sectors
    long parts[] = {
        256, 4096, 0, 4096, -1, 0
    };
    vfs_fdisk("sdA", 2, parts);

    vfs_parts("sdA");


    inode_t *root = vfs_mount("sdB", "fatfs");


    // ....


    mountfs_t *fs = root->fs;
    vfs_close(root);

    vfs_sweep(fs, 15);
    vfs_umount(root);

    vfs_rmdev("sdA0");
    vfs_rmdev("sdA1");
    vfs_rmdev("sdA2");

    KTEARDOWN(fatfs);
    KTEARDOWN(isofs);
    KTEARDOWN(imgdk);
}

int main()
{
    kprintf(-1, "Kora VFS check - " __ARCH " - v" _VTAG_ "\n");
    test_01();
    test_02();
    test_03();
    return 0;
}

