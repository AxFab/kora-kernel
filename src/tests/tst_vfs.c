#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/device.h>
#include <threads.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include "check.h"

void futex_init();

int blk_read(inode_t *ino, char *buf, size_t len, int flags, off_t off)
{
    return -1;
}

int blk_write(inode_t *ino, const char *buf, size_t len, int flags, off_t off)
{
    return -1;
}

void vfs_sweep();
void hostfs_setup();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

TEST_CASE(tst_devfs_01)
{
    int i;
    char buf[10];

    inode_t *zero = vfs_search(kSYS.dev_ino, kSYS.dev_ino, "zero", NULL);
    ck_ok(zero != NULL);
    ck_ok(zero == vfs_search(kSYS.dev_ino, kSYS.dev_ino, "zero", NULL));
    ck_ok(10 == vfs_read(zero, buf, 10, 0, 0));
    ck_ok(((int *)buf)[0] == 0 && ((int *)buf)[1] == 0);
    vfs_close(zero);
    vfs_close(zero);

    inode_t *null = vfs_search(kSYS.dev_ino, kSYS.dev_ino, "null", NULL);
    ck_ok(null != NULL);
    ck_ok(null == vfs_search(kSYS.dev_ino, kSYS.dev_ino, "null", NULL));
    for (i = 0; i < 5000; ++i)
        ck_ok(10 == vfs_write(null, buf, 10, 0, 0));
    vfs_close(null);
    ck_ok(0 == vfs_read(null, buf, 10, 0, VFS_NOBLOCK));
    ck_ok(errno == EWOULDBLOCK);
    vfs_close(null);

    inode_t *rand = vfs_search(kSYS.dev_ino, kSYS.dev_ino, "rand", NULL);
    ck_ok(rand != NULL);
    ck_ok(rand == vfs_search(kSYS.dev_ino, kSYS.dev_ino, "rand", NULL));
    ck_ok(10 == vfs_read(rand, buf, 10, 0, 0));
    ck_ok(-1 == vfs_write(rand, buf, 10, 0, 0));
    vfs_close(rand);

    vfs_sweep();
}

TEST_CASE(tst_devfs_02)
{
    vfs_init();

    inode_t *zero = vfs_search(kSYS.dev_ino, kSYS.dev_ino, "zero", NULL);
    ck_ok(zero != NULL);
    vfs_close(zero);

    inode_t *dir = kSYS.dev_ino;
    inode_t *ino;
    char buf[256];
    char name[256];
    void *dir_ctx = vfs_opendir(dir, NULL);
    ck_ok(dir_ctx != NULL);
    while ((ino = vfs_readdir(dir, name, dir_ctx)) != NULL) {
        snprintf(buf, 256, "  /%s%s \n", name, ino->type == FL_DIR ? "/" : "");
        kprintf(-1, buf);
        vfs_close(ino);
    }
    vfs_closedir(dir, dir_ctx);

    vfs_sweep();
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

TEST_CASE(tst_vfs_01)
{
    vfs_init();
    hostfs_setup();

    // Be sure the folder doesn't exist
    inode_t *dir = vfs_search(kSYS.dev_ino, kSYS.dev_ino, "mnt/boot/blop", NULL);
    ck_ok(dir == NULL && errno == ENOENT);

    // Create a directory
    inode_t *root = vfs_search(kSYS.dev_ino, kSYS.dev_ino, "mnt/boot", NULL);
    ck_ok(root != NULL && errno == 0);
    dir = vfs_create(root, "blop", FL_DIR, NULL, VFS_CREAT);
    ck_ok(dir != NULL && errno == 0);

    // Create a couple of files
    inode_t *file1 = vfs_create(dir, "text", FL_REG, NULL, VFS_CREAT);
    ck_ok(file1 != NULL && errno == 0);

    inode_t *file2 = vfs_create(dir, "block", FL_REG, NULL, VFS_CREAT);
    ck_ok(file2 != NULL && errno == 0);

    // Lookup
    ck_ok(file1 == vfs_lookup(dir, "text"));
    vfs_close(file1);
    ck_ok(NULL == vfs_create(dir, "text", FL_REG, NULL, VFS_CREAT));


    ck_ok(0 == vfs_unlink(dir, "text"));
    ck_ok(-1 == vfs_unlink(dir, "image"));
    ck_ok(0 == vfs_unlink(dir, "block"));
    ck_ok(-1 == vfs_unlink(dir, "text"));

    ck_ok(0 == vfs_unlink(root, "blop"));

    // UNMOUND HOSTFS !
    vfs_sweep();
}

TEST_CASE(tst_vfs_02)
{
    vfs_init();
    hostfs_setup();

    // Be sure the folder doesn't exist
    inode_t *dir = vfs_search(kSYS.dev_ino, kSYS.dev_ino, "mnt/boot/blop", NULL);
    ck_ok(dir == NULL && errno == ENOENT);

    // Create a directory
    inode_t *root = vfs_search(kSYS.dev_ino, kSYS.dev_ino, "mnt/boot", NULL);
    ck_ok(root != NULL && errno == 0);
    dir = vfs_create(root, "blop", FL_DIR, NULL, VFS_CREAT);
    ck_ok(dir != NULL && errno == 0);

    // Create a file
    inode_t *file1 = vfs_create(dir, "zig", FL_REG, NULL, VFS_CREAT);
    ck_ok(file1 != NULL && errno == 0);
    vfs_close(file1);

    // Lookup
    ck_ok(NULL == vfs_search(dir, dir, "zig/dru", NULL));
    ck_ok(NULL == vfs_search(dir, dir, "blaz", NULL));


    ck_ok(0 == vfs_unlink(dir, "zig"));
    ck_ok(0 == vfs_unlink(root, "blop"));

    // UNMOUND HOSTFS !
    vfs_sweep();
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

ino_ops_t img_dev_ops = {
    .close = NULL,
};

TEST_CASE(tst_tar_01)
{
    vfs_init();
    hostfs_setup();

    int fd = open("test.tar", O_RDONLY);
    ck_abort(fd == -1);

    inode_t *dev = vfs_inode(1, FL_BLK, NULL);
    dev->dev->vendor = strdup("myself");
    dev->dev->model = strdup("TAR Image");
    dev->dev->devclass = strdup("Block");
    dev->ops = &img_dev_ops;

    vfs_mkdev(dev, "img");
    vfs_close(dev);

    // UNMOUND HOSTFS !
    vfs_sweep();
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

jmp_buf __tcase_jump;
SRunner runner;

int main()
{
    kSYS.cpus = calloc(sizeof(struct kCpu), 0x20);
    // futex_init();

    suite_create("Virtual file system");

    fixture_create("Devfs tests");
//    tcase_create(tst_devfs_01);
//    tcase_create(tst_devfs_02);

    fixture_create("File system operations");
//    tcase_create(tst_vfs_01);
//    tcase_create(tst_vfs_02);

    fixture_create("TAR Archive");
//    tcase_create(tst_tar_01);

    free(kSYS.cpus);
    return summarize();
}


