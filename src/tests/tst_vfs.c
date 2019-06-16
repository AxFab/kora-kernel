#include <kernel/core.h>
#include <kernel/vfs.h>
#include <threads.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "check.h"

void futex_init();

const char *ksymbol(void *ip, char *buf, int lg) { return NULL; }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

TEST_CASE(tst_vfs_01)
{
    int i;
    char buf[10];
    devfs_mount();
    // vfs_init();
    futex_init();

    inode_t *zero = vfs_search(kSYS.dev_ino, kSYS.dev_ino, "zero", NULL);
    ck_ok(zero != NULL);
    ck_ok(zero == vfs_search(kSYS.dev_ino, kSYS.dev_ino, "zero", NULL));
    ck_ok(10 == vfs_read(zero, buf, 10, 0, 0));
    ck_ok(((int*)buf)[0] == 0 && ((int*)buf)[1] == 0);
    vfs_close(zero);

    inode_t *null = vfs_search(kSYS.dev_ino, kSYS.dev_ino, "null", NULL);
    ck_ok(null != NULL);
    ck_ok(null == vfs_search(kSYS.dev_ino, kSYS.dev_ino, "null", NULL));
    for (i = 0; i < 5000; ++i)
        ck_ok(10 == vfs_write(null, buf, 10, 0, 0));
    vfs_close(null);

    devfs_sweep();
    // vfs_sweep();
}

TEST_CASE(tst_vfs_02)
{
    devfs_mount();
    // vfs_init();
    futex_init();
    hostfs_setup();

    inode_t *dir = vfs_search(kSYS.dev_ino, kSYS.dev_ino, "boot/mnt/blop", NULL);

    // UNMOUND HOSTFS !
    devfs_sweep();
    // vfs_sweep();
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

jmp_buf __tcase_jump;
SRunner runner;

int main()
{
    kSYS.cpus = calloc(sizeof(struct kCpu), 0x500);
    // futex_init();

    suite_create("Virtual file system");
    // fixture_create("Non blocking operation");
    tcase_create(tst_vfs_01);
    tcase_create(tst_vfs_02);

    free(kSYS.cpus);
    return summarize();
}


