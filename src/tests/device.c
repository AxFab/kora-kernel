/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#include <kernel/memory.h>
#include <kernel/sys/inode.h>
#include <kernel/sys/device.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <check.h>

void IMG_setup();
void MBR_setup();
void ISOFS_setup();

START_TEST(test_device_001)
{
    // page_initialize();
    // IMG_setup();
    // MBR_setup();
    // ISOFS_setup();

    // device_t *dev = vfs_lookup_device("sdA");
    // MBR_format(dev->ino);
    // MBR_mount(dev->ino, dev->name);

    // dev = vfs_lookup_device("sdA1");
    // ck_assert(dev != NULL);
    // // FAT_format(dev->ino);
    // // FAT_mount(dev->ino, dev->name);
    // dev = vfs_lookup_device("sdC");
    // inode_t *ino = ISOFS_mount(dev->ino, dev->name);
    // ck_assert(ino != NULL && S_ISDIR(ino->mode));

    // mountpt_t *mnt = vfs_lookup_mountpt("ISOIMAGE");
    // ck_assert(mnt != NULL && mnt->ino == ino);
}
END_TEST

void fixture_device(Suite *s)
{
    TCase *tc;

    tc = tcase_create("Device");
    tcase_add_test(tc, test_device_001);
    suite_add_tcase(s, tc);
}
