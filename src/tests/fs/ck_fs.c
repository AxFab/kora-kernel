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
#include <kernel/device.h>
#include "../check.h"

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void imgdk_setup();
void imgdk_teardown();

inode_t *test_fs_setup(const char *dev, kmod_t *fsmod, int(*format)(inode_t *))
{
    vfs_init();
    imgdk_setup();
    fsmod->setup();

    inode_t *disk = vfs_search_device(dev);
    ck_ok(disk != NULL && errno == 0, "Search disk");
    format(disk);

    inode_t *root = vfs_mount(dev, fsmod->name);
    ck_ok(root != NULL  && errno == 0, "Mount newly formed disk");

    vfs_close_inode(disk);
    vfs_show_devices();
    return root;
}

void test_fs_teardown(inode_t *root, kmod_t *fsmod)
{
    vfs_show_devices();
    // TODO - Release all links !?
    // int res = vfs_umount(root);
    vfs_close_inode(root);
    // ck_ok(res == 0 && errno == 0, "Unmount file system");
    fsmod->teardown();
    // TODO - rmdev
    imgdk_teardown();
    vfs_fini();
}


void test_fs_mknod(inode_t *root)
{
    int ret;
    // Create and open file
    inode_t *ino1 = vfs_lookup(root, "EMPTY.TXT");
    ck_ok(ino1 == NULL && errno == ENOENT, "");

    inode_t *ino2 = vfs_create(root, "EMPTY.TXT", FL_REG, NULL, 0);
    ck_ok(ino2 != NULL && errno == 0, "");

    inode_t *ino3 = vfs_lookup(root, "EMPTY.TXT");
    ck_ok(ino3 != NULL && errno == 0, "");
    ck_ok(ino2 == ino3, "");
    vfs_close_inode(ino2);
    vfs_close_inode(ino3);

    // Create and open directory
    inode_t *ino4 = vfs_create(root, "FOLDER", FL_DIR, NULL, 0);
    ck_ok(ino4 != NULL && errno == 0, "");

    inode_t *ino5 = vfs_create(ino4, "FILE.O", FL_REG, NULL, 0);
    ck_ok(ino5 != NULL && errno == 0, "");

    inode_t *ino6 = vfs_search(root, root, "FOLDER/FILE.O", NULL);
    ck_ok(ino6 != NULL && errno == 0, "");
    ck_ok(ino6 == ino5, "");
    vfs_close_inode(ino5);
    vfs_close_inode(ino6);

    // Browse directory
    inode_t *ino7;
    char filename[256];
    void *ctx = vfs_opendir(ino4, NULL);
    ck_ok(ctx != NULL && errno == 0, "");
    while ((ino7 = vfs_readdir(ino4, filename, ctx)) != NULL) {
        ck_ok(ino7 != NULL && errno == 0, "");
        ck_ok(ino7 == ino5, "");
        ino5 = NULL;
        vfs_close_inode(ino7);
    }
    vfs_closedir(ino4, ctx);
    ck_ok(errno == 0, "");

    // Delete files and directories
    ret = vfs_unlink(root, "FOLDER");
    ck_ok(ret == -1 && errno == ENOTEMPTY, "");

    vfs_close_inode(ino4);
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
    vfs_close_inode(ino4);

    // ROOT & FOLDER are on inode cache, but not used anymore.
}

void test_fs_rdwr(inode_t *root)
{
    int ret;
    char *buf = malloc(100);
    // Create and open file
    inode_t *ino1 = vfs_create(root, "TEXT.TXT", FL_REG, NULL, 0);
    ck_ok(ino1 != NULL && errno == 0, "");

    ret = vfs_read(ino1, buf, 100, 0, 0);
    ck_ok(ret == 0 && errno == 0, "");

    ret = vfs_truncate(ino1, 12);
    ck_ok(ret == 0 && errno == 0, "");

    ret = vfs_write(ino1, "Hello world!", 12, 0, 0);
    ck_ok(ret == 12 && errno == 0, "");

    buf[12] = 'Z';
    buf[13] = '\0';
    ret = vfs_read(ino1, buf, 100, 0, 0);
    ck_ok(ret == 12 && errno == 0, "");
    ck_ok(strncmp("Hello world!Z", buf, 15) == 0, "");
}

void test_fs_truncate(inode_t *root)
{
    int ret;
    inode_t *ino1 = vfs_create(root, "FILE_S.TXT", FL_REG, NULL, 0);
    ck_ok(ino1 != NULL && errno == 0, "");
    ret = vfs_truncate(ino1, 348); // Less than a sector
    ck_ok(ret == 0 && errno == 0, "");

    inode_t *ino2 = vfs_create(root, "FILE_L.TXT", FL_REG, NULL, 0);
    ck_ok(ino2 != NULL && errno == 0, "");
    ret = vfs_truncate(ino2, 75043); // Much larger file
    ck_ok(ret == 0 && errno == 0, "");
    inode_t *ino3 = vfs_create(root, "FILE_XL.TXT", FL_REG, NULL, 0);
    ck_ok(ino3 != NULL && errno == 0, "");
    ret = vfs_truncate(ino3, 2000 * _Mib_); // Much larger file
    ck_ok(ret != 0 && errno == ENOSPC, "");

    vfs_close_inode(ino1);
    vfs_close_inode(ino2);
    vfs_close_inode(ino3);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

extern kmod_t kmod_info_fatfs;
int fatfs_format(inode_t *blk);

START_TEST(test_fat16_mknod)
{
    inode_t *ino = test_fs_setup("sdA", &kmod_info_fatfs, fatfs_format);
    test_fs_mknod(ino);
    test_fs_teardown(ino, &kmod_info_fatfs);
}
END_TEST

START_TEST(test_fat16_rdwr)
{
    inode_t *ino = test_fs_setup("sdA", &kmod_info_fatfs, fatfs_format);
    test_fs_rdwr(ino);
    test_fs_teardown(ino, &kmod_info_fatfs);
}
END_TEST

START_TEST(test_fat16_truncate)
{
    inode_t *ino = test_fs_setup("sdA", &kmod_info_fatfs, fatfs_format);
    test_fs_truncate(ino);
    test_fs_teardown(ino, &kmod_info_fatfs);
}
END_TEST

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#if 0
extern kmod_t kmod_info_ext2;
int ext2_format(inode_t *blk);

START_TEST(test_ext2_mknod)
{
    inode_t *ino = test_fs_setup("sdA", &kmod_info_ext2, ext2_format);
    test_fs_mknod(ino);
    test_fs_teardown(ino, &kmod_info_ext2);
}
END_TEST

START_TEST(test_ext2_rdwr)
{
    inode_t *ino = test_fs_setup("sdA", &kmod_info_ext2, ext2_format);
    test_fs_rdwr(ino);
    test_fs_teardown(ino, &kmod_info_ext2);
}
END_TEST

START_TEST(test_ext2_truncate)
{
    inode_t *ino = test_fs_setup("sdA", &kmod_info_ext2, ext2_format);
    test_fs_truncate(ino);
    test_fs_teardown(ino, &kmod_info_ext2);
}
END_TEST
#endif

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void fixture_rwfs(Suite *s)
{
    TCase *tc;

    tc = tcase_create("FAT16");
    tcase_add_test(tc, test_fat16_mknod);
    // tcase_add_test(tc, test_fat16_rdwr);
    // tcase_add_test(tc, test_fat16_truncate);

    // tc = tcase_create("Ext2");
    // tcase_add_test(tc, test_ext2_mknod);
    // tcase_add_test(tc, test_ext2_rdwr);
    // tcase_add_test(tc, test_ext2_truncate);
    suite_add_tcase(s, tc);
}
