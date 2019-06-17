#include <kernel/core.h>
#include <kernel/files.h>
#include <threads.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
// #include <unistd.h>
#include "check.h"

long read(int fd, char *buf, size_t lg);
long write(int fd, const char *buf, size_t lg);


void futex_init();


page_t mmu_read(size_t address)
{
    return address;
}


map_cache_t *map_create(inode_t *ino, void *read, void *write);
void map_destroy(map_cache_t *cache);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

TEST_CASE(tst_blk_01)
{
    int fd1 = 5;// open()
    map_cache_t *map1 = map_create((void*)fd1, read, write);

    map_destroy(map1);
}


jmp_buf __tcase_jump;
SRunner runner;

int main()
{
    kSYS.cpus = calloc(sizeof(struct kCpu), 0x500);
    // futex_init();

    suite_create("IO Block");
    // fixture_create("Non blocking operation");
    tcase_create(tst_blk_01);

    free(kSYS.cpus);
    return summarize();
}


