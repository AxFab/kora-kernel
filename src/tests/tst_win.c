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
