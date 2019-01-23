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
#include <kernel/sys/inode.h>
#include <errno.h>
#include <unistd.h>
#include <check.h>

#define STR_x10(s) s s s s s s s s s s


static inode_t *make_root()
{
    inode_t *root = vfs_initialize();

    // Create a dummy block device
    // inode_t *dev_inode = vfs_mkdev(NULL, 0777 | S_ISBLK, 0, );

    // Request a format of this block using `DumbFS`
    // vfs_format(NULL, dev_inode, "")
    return root;
}

static void sweep_root(inode_t *root)
{
    vfs_sweep(root);
    ck_assert(errno == 0);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void test_mkdir_mode(inode_t *dir, int umask, int rmask, int emask)
{
    const char *name = "ZIG";
    // kTSK.umask = umask;

    inode_t *ino = vfs_mkdir(dir, name, rmask & ~umask);
    ck_assert(ino != NULL && errno == 0);
    ck_assert(ino->mode == (S_IFDIR | emask));
    vfs_close(ino);

    vfs_rmdir(dir, name);
    ck_assert(errno == 0);
}

// static void test_mkdir_uid(inode_t *dir, uid_t ruid, gid_t rgid, )
// {
//   const char *name = "STO";
//   kTSK.uid = 65535;
//   kTSK.gid = 65535;

//   inode_t *ino = vfs_mkdir(dir, name, 0755);
//   ck_assert(ino != NULL && errno == 0);
//   ck_assert(ino->uid == 65535 && ino->gid == 65535);

//   vfs_rmdir(ino);
//   ck_assert(errno == 0);
// }

static void test_filename(inode_t *dir, const char *name, int err)
{
    inode_t *ino;
    ino = vfs_mkdir(dir, name, 0755);
    ck_assert(errno == err);
    if (err == 0) {
        ck_assert(ino != NULL);
        vfs_close(ino);
    } else
        ck_assert(ino == NULL);
    vfs_rmdir(dir, name);
    ck_assert(errno == err);

    ino = vfs_create(dir, name, 0644);
    ck_assert(errno == err);
    if (err == 0) {
        ck_assert(ino != NULL);
        vfs_close(ino);
    } else
        ck_assert(ino == NULL);
    vfs_unlink(dir, name);
    ck_assert(errno == err);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
// Test `mkdir`
START_TEST(test_vfs_001)
{
    inode_t *root = make_root();
    inode_t *dir = vfs_mkdir(root, "KAH", 0755);
    inode_t *ino;

    // Mode
    test_mkdir_mode(dir, 0, 0755, 0755);
    test_mkdir_mode(dir, 077, 0151, 0100);
    test_mkdir_mode(dir, 070, 0345, 0305);
    test_mkdir_mode(dir, 0501, 0345, 0244);
    test_mkdir_mode(dir, 0, 0151, 0151);

    // UID & GID
    // vfs_chown(dir, 65535, 65535);
    // ck_assert(errno == 0);
    // test_mkdir_uid(dir, 65535, 65535, 65535, 65535);
    // test_mkdir_uid(dir, 65535, 65534, 65535, 65534);
    // vfs_chmod(dir, 0777);
    // ck_assert(errno == 0);
    // test_mkdir_uid(dir, 65533, 65534, 65533, 65534);

    // DATES
    ck_assert(errno == 0);
    time_t p_time = dir->ctime.tv_sec;
    sleep(1);
    ino = vfs_mkdir(dir, "BLAZ", 0755);
    ck_assert(ino != NULL && errno == 0);
    vfs_close(ino);
    ck_assert(p_time < ino->atime.tv_sec);
    ck_assert(p_time < ino->mtime.tv_sec);
    ck_assert(p_time < ino->ctime.tv_sec);
    // ck_assert(p_time < dir->mtime.tv_sec);
    // ck_assert(p_time < dir->ctime.tv_sec);
    vfs_rmdir(dir, "BLAZ");
    ck_assert(errno == 0);

    // ENOTDIR
    ino = vfs_create(dir, "DRU", 0644);
    ck_assert(errno == 0);
    vfs_close(ino);
    ck_assert(vfs_mkdir(ino, "GNEE", 0755) == NULL && errno == ENOTDIR);
    vfs_unlink(dir, "DRU");
    ck_assert(errno == 0);

    // ENAMETOOLONG
    char *lgname = STR_x10("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    ck_assert(vfs_mkdir(dir, lgname, 0755) == NULL && errno == ENAMETOOLONG);

    // // EACCES
    // vfs_chown(dir, 65534, 65534);
    // vfs_chmod(dir, 0755);
    // ino = vfs_mkdir(dir, "KAH", 0755);
    // ck_assert(ino != NULL && errno == 0);
    // vfs_close(ino);
    // vfs_rmdir(dir, "KAH");
    // ck_assert(errno == 0);


    // vfs_chmod(dir, 0750);
    // ino = vfs_mkdir(dir, "KAH", 0755);
    // ck_assert(ino != NULL && errno == 0);
    // vfs_close(ino);
    // vfs_rmdir(dir, "KAH");
    // ck_assert(errno == 0);

    // EPERM (immutable flag)

    // EEXIST
    ino = vfs_mkdir(dir, "ZEE", 0755);
    vfs_close(ino);
    ck_assert(vfs_mkdir(dir, "ZEE", 0755) == NULL && errno == EEXIST);
    vfs_rmdir(dir, "ZEE");
    ck_assert(errno == 0);

    ino = vfs_create(dir, "ZEE", 0644);
    vfs_close(ino);
    ck_assert(vfs_mkdir(dir, "ZEE", 0755) == NULL && errno == EEXIST);
    vfs_unlink(dir, "ZEE");
    ck_assert(errno == 0);

#if 0
    ino = vfs_symlink(dir, "ZEE", "NoWhere");
    ck_assert(vfs_mkdir(dir, "ZEE", 0755) == NULL && errno == EEXIST);
    vfs_rmdir(dir, "ZEE");

    ino = vfs_mkfifo(dir, "ZEE", 0644);
    ck_assert(vfs_mkdir(dir, "ZEE", 0755) == NULL && errno == EEXIST);
    vfs_rmdir(dir, "ZEE");
#endif

    vfs_close(dir);
    vfs_rmdir(root, "KAH");
    ck_assert(errno == 0);
    sweep_root(root);

}
END_TEST

// Test `rmdir`
START_TEST(test_vfs_002)
{
    inode_t *root = make_root();
    inode_t *dir = vfs_mkdir(root, "KAH", 0755);
    inode_t *ino;
    inode_t *sub;

    // Check that directory can't be found after deletion
    ino = vfs_mkdir(dir, "ZIG", 0755);
    ck_assert(ino != NULL && errno == 0);
    vfs_close(ino);
    ino = vfs_lookup(dir, "ZIG");
    ck_assert(ino != NULL && ino->mode == (S_IFDIR | 0755));
    vfs_close(ino);
    vfs_rmdir(dir, "ZIG");
    ck_assert(errno == 0);
    ino = vfs_lookup(dir, "ZIG");
    ck_assert(ino == NULL && errno == ENOENT);

    // Successful mkdir updates ctime.

    // Check returns ENOTDIR if a component of the path is not a directory
    ino = vfs_create(dir, "STO", 0755);
    vfs_rmdir(ino, "BLAZ");
    ck_assert(errno == ENOTDIR);
    vfs_rmdir(dir, "STO");
    ck_assert(errno == ENOTDIR);
    vfs_unlink(dir, "STO");
    ck_assert(errno == 0);
    vfs_close(ino);

    // Check returns ENAMETOOLONG if a component of a pathname exceeded 255 characters
    char *lgname = STR_x10("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    vfs_rmdir(dir, lgname);
    ck_assert(errno == ENAMETOOLONG);

    // Returns ENOENT if the named directory does not exist
    ino = vfs_mkdir(dir, "DRU", 0755);
    ck_assert(ino != NULL && errno == 0);
    vfs_close(ino);
    ino = vfs_lookup(dir, "DRU");
    ck_assert(ino->mode == (S_IFDIR | 0755));
    vfs_close(ino);
    vfs_rmdir(dir, "DRU");
    ck_assert(errno == 0);
    vfs_rmdir(dir, "DRU");
    ck_assert(errno == ENOENT);

    // Returns ENOTEMPTY if the directory contains files
    ino = vfs_mkdir(dir, "BLAZ", 0755);
    ck_assert(errno == 0);
    vfs_close(ino);
    sub = vfs_mkdir(ino, "GNEE", 0755);
    ck_assert(errno == 0);
    vfs_close(sub);
    vfs_rmdir(dir, "BLAZ");
    ck_assert(errno == ENOTEMPTY);
    vfs_rmdir(ino, "GNEE");
    ck_assert(errno == 0);
    vfs_rmdir(dir, "BLAZ");
    ck_assert(errno == 0);

    ino = vfs_mkdir(dir, "BLAZ", 0755);
    ck_assert(errno == 0);
    vfs_close(ino);
    sub = vfs_create(ino, "GNEE", 0644);
    ck_assert(errno == 0);
    vfs_close(sub);
    vfs_rmdir(dir, "BLAZ");
    ck_assert(errno == ENOTEMPTY);
    vfs_unlink(ino, "GNEE");
    ck_assert(errno == 0);
    vfs_rmdir(dir, "BLAZ");
    ck_assert(errno == 0);

    // Returns EBUSY if the directory to be removed is the mount point for a mounted file system

    vfs_close(dir);
    vfs_rmdir(root, "KAH");
    ck_assert(errno == 0);
    sweep_root(root);

}
END_TEST

// Test `unlink`
START_TEST(test_vfs_003)
{
    inode_t *root = make_root();
    inode_t *dir = vfs_mkdir(root, "KAH", 0755);

    // Unlink removes regular files, symbolic links, fifos and sockets
    inode_t *ino = vfs_create(dir, "ZIG", 0644);
    ck_assert(ino != NULL && errno == 0);
    vfs_close(ino);
    ino = vfs_lookup(dir, "ZIG");
    ck_assert(ino != NULL && ino->mode == (S_IFREG | 0644) && errno == 0);
    vfs_close(ino);
    vfs_unlink(dir, "ZIG");
    ck_assert(errno == 0);
    ino = vfs_lookup(dir, "ZIG");
    ck_assert(ino == NULL && errno == ENOENT);

    // Successful unlink updates ctime.

    // Unsuccessful unlink does not update ctime.

    // Check returns ENOTDIR if a component of the path is not a directory
    ino = vfs_create(dir, "STO", 0644);
    vfs_close(ino);
    vfs_unlink(ino, "BLAZ");
    ck_assert(errno == ENOTDIR);
    vfs_unlink(dir, "STO");
    ck_assert(errno == 0);

    // Check returns ENAMETOOLONG if a component of a pathname exceeded 255 characters
    char *lgname = STR_x10("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    vfs_unlink(dir, lgname);
    ck_assert(errno == ENAMETOOLONG);

    // Returns ENOENT if the named file does not exist
    ino = vfs_create(dir, "DRU", 0644);
    ck_assert(ino != NULL && errno == 0);
    vfs_close(ino);
    ino = vfs_lookup(dir, "DRU");
    ck_assert(ino != NULL && ino->mode == (S_IFREG | 0644) && errno == 0);
    vfs_close(ino);
    vfs_unlink(dir, "DRU");
    ck_assert(errno == 0);
    vfs_unlink(dir, "DRU");
    ck_assert(errno == ENOENT);
    ino = vfs_lookup(dir, "DRU");
    ck_assert(ino == NULL && errno == ENOENT);

    // Returns EPERM if the named file is a directory
    ino = vfs_mkdir(dir, "ZIG", 0755);
    ck_assert(ino != NULL && ino->mode == (S_IFDIR | 0755) && errno == 0);
    vfs_close(ino);
    vfs_unlink(dir, "ZIG");
    ck_assert(errno == EPERM);
    vfs_rmdir(dir, "ZIG");
    ck_assert(errno == 0);


    vfs_close(dir);
    vfs_rmdir(root, "KAH");
    ck_assert(errno == 0);
    sweep_root(root);

}
END_TEST

// returns EINVAL if filename isn't valid
START_TEST(test_vfs_004)
{
    inode_t *root = make_root();

    // Forbidden characters
    test_filename(root, "/Hello", EINVAL);
    test_filename(root, "C:Hello", EINVAL);
    test_filename(root, "Hello\\World", EINVAL);
    test_filename(root, "Hello\tWorld", EINVAL);
    test_filename(root, "Hello World\n", EINVAL);
    test_filename(root, "Hello World\r\n", EINVAL);

    // Dots
    test_filename(root, ".", EINVAL);
    test_filename(root, "..", EINVAL);
    test_filename(root, ".......", EINVAL);
    test_filename(root, "name.ext", 0);
    test_filename(root, ".hidden", 0);

    // Unicode
    test_filename(root, "Здравствуйте", 0); // Russian
    test_filename(root, "こんにちは", 0); // Japanease
    test_filename(root, "你好", 0); // Chinnease
    test_filename(root, "여보세요", 0); // Korean
    test_filename(root, "नमस्ते", 0); // Indie
    test_filename(root, "ಹಲೋ", 0); // Kannada
    test_filename(root, "สวัสดี", 0); // Thai
    test_filename(root, "🀀", 0); // Majong EAST

    // Bad Unicode chars
    char buf[6];
    strcpy(buf, "🀀");
    test_filename(root, &buf[1], EINVAL);
    strcpy(buf, "🀀");
    buf[1] = 'a';
    test_filename(root, buf, EINVAL);
    strcpy(buf, "🀀");
    buf[2] = 'a';
    test_filename(root, buf, EINVAL);
    strcpy(buf, "🀀");
    buf[3] = 'a';
    test_filename(root, buf, EINVAL);


    sweep_root(root);
}
END_TEST


void fixture_vfs(Suite *s)
{
    TCase *tc = tcase_create("File-Systems");
    tcase_add_test(tc, test_vfs_001);
    tcase_add_test(tc, test_vfs_002);
    tcase_add_test(tc, test_vfs_003);
    tcase_add_test(tc, test_vfs_004);
    suite_add_tcase(s, tc);
}
