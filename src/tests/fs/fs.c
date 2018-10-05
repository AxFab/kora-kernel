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
#include "../check.h"

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void imgdk_setup();
void imgdk_teardown();

inode_t *test_fs_setup(CSTR dev, kmod_t *fsmod, int(*format)(inode_t *))
{
    imgdk_setup();
    fsmod->setup();

    inode_t *disk = vfs_search_device(dev);
    ck_ok(disk != NULL && errno == 0, "Search disk");
    format(disk);

    inode_t *root = vfs_mount(dev, fsmod->name);
    ck_ok(root != NULL  && errno == 0, "Mount newly formed disk");
    return root;
}

void test_fs_teardown(inode_t *root)
{
    int res = vfs_umount(root);
    ck_ok(res == 0 && errno == 0, "Unmount file system");
    vfs_close(root);
    imgdk_teardown();
    vfs_reset();
}


void test_fs_mknod(inode_t *root)
{
    int ret;
    // Create and open file
    inode_t *ino1 = vfs_lookup(root, "EMPTY.TXT");
    ck_ok(ino1 == NULL && errno == ENOENT, "");

    inode_t *ino2 = vfs_create(root, "EMPTY.TXT", S_IFREG, NULL, 0);
    ck_ok(ino2 != NULL && errno == 0, "");

    inode_t *ino3 = vfs_lookup(root, "EMPTY.TXT");
    ck_ok(ino3 != NULL && errno == 0, "");
    ck_ok(ino2 == ino3, "");
    vfs_close(ino2);
    vfs_close(ino3);

    // Create and open directory
    inode_t *ino4 = vfs_create(root, "FOLDER", S_IFDIR, NULL, 0);
    ck_ok(ino4 != NULL && errno == 0, "");

    inode_t *ino5 = vfs_create(ino4, "FILE.O", S_IFREG, NULL, 0);
    ck_ok(ino5 != NULL && errno == 0, "");

    inode_t *ino6 = vfs_search(root, root, "FOLDER/FILE.O", NULL);
    ck_ok(ino6 != NULL && errno == 0, "");
    ck_ok(ino6 == ino5, "");
    vfs_close(ino5);
    vfs_close(ino6);

    // Browse directory
    inode_t *ino7;
    char filename[256];
    void *ctx = vfs_opendir(ino4, NULL);
    ck_ok(ctx != NULL && errno == 0, "");
    while ((ino7 = vfs_readdir(ino4, filename, ctx)) != NULL) {
        ck_ok(ino7 != NULL && errno == 0, "");
        // ck_ok(ino7 == ino5);
        ino5 = NULL;
        vfs_close(ino7);
    }
    vfs_closedir(ino4, ctx);
    ck_ok(errno == 0, "");

    // Delete files and directories
    // ret = vfs_unlink(root, "FOLDER");
    // ck_ok(ret == -1 && errno == ENOTEMPTY);

    vfs_close(ino4);
    ino4 = vfs_lookup(root, "FOLDER");
    ck_ok(ino4 != NULL && errno == 0, "");

    ret = vfs_unlink(ino4, "FILE.O");
    ck_ok(ret == 0 && errno == 0, "");
    inode_t *ino8 = vfs_lookup(ino4, "FILE.O");
    ck_ok(ino8 == NULL && errno == ENOENT, "");

    ret = vfs_unlink(root, "FOLDER");
    ck_ok(ret == 0 && errno == 0, "");
    inode_t *ino9 = vfs_lookup(root, "FOLDER");
    ck_ok(ino9 == NULL && errno == ENOENT, "");
    vfs_close(ino4);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

extern kmod_t kmod_info_fatfs;
int fatfs_format(inode_t *blk);

START_TEST(test_fat16_mknod)
{
    inode_t *ino = test_fs_setup("sdA", &kmod_info_fatfs, fatfs_format);
    test_fs_mknod(ino);
    test_fs_teardown(ino);
}
END_TEST

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void fixture_rwfs(Suite *s)
{
    TCase *tc;

    tc = tcase_create("FAT16");
    tcase_add_test(tc, test_fat16_mknod);
    suite_add_tcase(s, tc);
}