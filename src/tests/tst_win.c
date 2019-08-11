#include <kernel/core.h>
#include <kernel/files.h>
#include <threads.h>
#include <assert.h>
#include <string.h>
// #include <fcntl.h>
#include "check.h"


void futex_init();
void itimer_init();

page_t mmu_read(size_t addr)
{
    return 0;
}
inode_t *vfs_inode(int ino, int type, device_t *dev)
{
    return NULL;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

TEST_CASE(tst_win_01)
{
    desktop_t *desk = kalloc(sizeof(desktop_t));
    // window_t *win = wmgr_window(desk, 400, 300);

    kfree(desk);
}


jmp_buf __tcase_jump;
SRunner runner;

int main()
{
    kSYS.cpus = calloc(sizeof(struct kCpu), 0x500);
    futex_init();
    itimer_init();

    suite_create("Window");
    // fixture_create("Non blocking operation");
    tcase_create(tst_win_01);

    free(kSYS.cpus);
    return summarize();
}


