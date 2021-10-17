/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#include <kernel/core.h>
#include <kernel/files.h>
#include <threads.h>
#include <assert.h>
#include <string.h>
// #include <fcntl.h>
// #include <unistd.h>
#include "check.h"

long read(int fd, char *buf, size_t lg);
long write(int fd, const char *buf, size_t lg);


void futex_init();


map_cache_t *blk_create(inode_t *ino, void *read, void *write);
void blk_destroy(map_cache_t *cache);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

page_t mmu_read(size_t addr)
{
    return 0;
}

char *vfs_inokey(inode_t *ino, char *buf)
{

}

long bread(int fd, char *buf, size_t lg, off_t off)
{
    return 0;
}

long bwrite(int fd, const char *buf, size_t lg, off_t off)
{
    return 0;
}

TEST_CASE(tst_blk_01)
{
    size_t fd1 = 5;// open()
    blk_cache_t *map1 = blk_create((void *)fd1, bread, bwrite);

    page_t p1 = blk_fetch(map1, 0 * PAGE_SIZE);
    page_t p2 = blk_fetch(map1, 1 * PAGE_SIZE);
    page_t p3 = blk_fetch(map1, 2 * PAGE_SIZE);
    page_t p4 = blk_fetch(map1, 2 * PAGE_SIZE);
    ck_ok(p3 == p4);

    blk_markwr(map1, 0);
    blk_sync(map1, 0, p1);
    blk_release(map1, 0, p1);

    blk_sync(map1, PAGE_SIZE, p2);
    blk_release(map1, PAGE_SIZE, p2);

    blk_release(map1, 2 * PAGE_SIZE, p3);
    blk_markwr(map1, 2 * PAGE_SIZE);
    blk_release(map1, 2 * PAGE_SIZE, p4);

    blk_destroy(map1);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

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
