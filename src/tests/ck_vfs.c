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
#include <kernel/mods/fs.h>

#define KMODULE(n) int n ## _setup(); int n ## _teardown()

KMODULE(TMPFS);
KMODULE(DEVFS);
KMODULE(ATA);
KMODULE(PCI);
KMODULE(E1000);
KMODULE(PS2);
KMODULE(VBE);
KMODULE(IMG);
KMODULE(ISOFS);
KMODULE(FATFS);

void vfs_sweep(mountfs_t *fs, int max);

void test_01()
{
    kprintf (-1, "\n\e[94m  VFS #1 - Search, unmount and re-search\e[0m\n");

    IMG_setup();
    ISOFS_setup();

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

    ISOFS_teardown();
    IMG_teardown();
}


void test_02()
{
    kprintf (-1, "\n\e[94m  VFS #2 - Read directory\e[0m\n");

    IMG_setup();
    ISOFS_setup();

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

    ISOFS_teardown();
    IMG_teardown();
}

void test_03()
{
    kprintf (-1, "\n\e[94m  VFS #3 - Format, create remove nodes\e[0m\n");
    IMG_setup();
    ISOFS_setup();
    FATFS_setup();

    // ~64Mb 131072 sectors
    long parts[] = {
        256, 2048, 0, 2048, 65536, 0, 65536, -1, 0,
    };
    vfs_fdisk("sdA", 3, parts);

    vfs_parts("sdA");

    // inode_t *root = vfs_mount("sdA", "isofs");

    // inode_t *ino = vfs_search(root, NULL, "/bin/init", NULL);
    // vfs_close(ino);
    // if (root == NULL) {
    //     kprintf(-1, "Expected mount point named 'ISOIMAGE' over 'sdC' !\n");
    //     return;
    // }

    // vfs_mount(root, "dev", NULL, "devfs");
    // vfs_mount(root, "tmp", NULL, "tmpfs");

    // inode_t *ino = vfs_search(root, root, "bin/init");
    // if (ino == NULL) {
    //     kprintf(-1, "Expected file 'bin/init'.\n");
    //     return;
    // }

    // vfs_umount(root); // TODO Lazy or not !?


    FATFS_teardown();
    ISOFS_teardown();
    IMG_teardown();
}

int main ()
{
    kprintf(-1, "Kora VFS check - " __ARCH " - v" _VTAG_ "\n");
    test_01();
    test_02();
    test_03();
    return 0;
}

