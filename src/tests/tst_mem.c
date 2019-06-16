#include <kernel/core.h>
#include <kernel/memory.h>
#include <threads.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "check.h"

void futex_init();

void vfs_open(inode_t *ino) {}
void vfs_close(inode_t *ino) {}
void vfs_readlink(inode_t *ino) {}

_Noreturn void kpanic(const char *ms, ...)
{
    abort();
}

_Noreturn void task_fatal(CSTR error, unsigned signum)
{
    abort();
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

TEST_CASE(tst_mem_01)
{
    kMMU.max_vma_size = 64 * PAGE_SIZE;
    mspace_t *msp = mspace_create();
    mspace_display(msp);

    void *ptr1 = mspace_map(msp, NULL, PAGE_SIZE, NULL, 0, VMA_ANON_RW);
    ck_ok(ptr1 != NULL);

    void *ptr2 = mspace_map(msp, NULL, PAGE_SIZE, NULL, 0, VMA_ANON_RW);
    ck_ok(ptr2 != NULL);

    mspace_unmap(msp, ptr1, PAGE_SIZE);

    mspace_protect(msp, ptr2, PAGE_SIZE, VMA_RO);

    mspace_display(msp);
    mspace_close(msp);
}


jmp_buf __tcase_jump;
SRunner runner;

int main()
{
    kSYS.cpus = calloc(sizeof(struct kCpu), 0x500);
    // futex_init();

    suite_create("Memory");
    // fixture_create("Non blocking operation");
    tcase_create(tst_mem_01);

    free(kSYS.cpus);
    return summarize();
}


