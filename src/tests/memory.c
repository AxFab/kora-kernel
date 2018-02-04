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
#include <kernel/memory.h>
#include <kernel/sys/vma.h>
#include <errno.h>
#include <check.h>

START_TEST(test_memory_001)
{
    int ret;

    page_initialize();

    inode_t *ino1 = (void *)0xbeaf0001;
    inode_t *ino2 = (void *)0xbeaf0002;
    mspace_t *mspace = mspace_create();

    // Simple program simulation
    void *m1 = mspace_map(mspace, 0, 12 * _Kib_, ino1, 4 * _Kib_, VMA_FG_CODE);
    ck_assert(m1 != NULL && errno == 0);

    void *m2 = mspace_map(mspace, 0, 8 * _Kib_, ino1, 4 * _Kib_, VMA_FG_DATAI);
    ck_assert(m2 != NULL && errno == 0);

    void *m3 = mspace_map(mspace, 0, 4 * _Kib_, NULL, 0, VMA_FG_STACK);
    ck_assert(m3 != NULL && errno == 0);

    void *m4 = mspace_map(mspace, 0, 64 * _Kib_, NULL, 0, VMA_FG_HEAP);
    ck_assert(m4 != NULL && errno == 0);

    void *m5 = mspace_map(mspace, 120 * _Mib_, 8 * _Mib_, ino2, 0,
                          VMA_FG_RW_FILE);
    ck_assert(m5 != NULL && errno == 0);

    // Test EINVAL
    void *m6 = mspace_map(mspace, 100 * _Mib_, 0, ino2, 0, VMA_FG_RW_FILE);
    ck_assert(m6 == NULL && errno == EINVAL);

    void *m7 = mspace_map(mspace, 100 * _Mib_, VMA_CFG_MAXSIZE + PAGE_SIZE, ino2,
                          0, VMA_FG_RW_FILE);
    ck_assert(m7 == NULL && errno == EINVAL);

    ret = mspace_protect(mspace, 120 * _Mib_, 2 * _Mib_, VMA_DEAD);
    ck_assert(ret == 0 && errno == 0);

    ret = mspace_protect(mspace, 123 * _Mib_, 2 * _Mib_, VMA_DEAD);
    ck_assert(ret == 0 && errno == 0);

    ret = mspace_protect(mspace, 126 * _Mib_, 2 * _Mib_, VMA_DEAD);
    ck_assert(ret == 0 && errno == 0);

    mspace_scavenge(mspace);

    // mspace_display(0, mspace);
    mspace_sweep(mspace);

}
END_TEST

START_TEST(test_memory_002)
{
    // User space is from 1Mb to 512Mb
    page_initialize();
    kMMU.uspace_lower_bound = 1 * _Mib_;
    kMMU.uspace_upper_bound = 512 * _Mib_;
    mspace_t *mspace = mspace_create();

    void *m;
    // We take [500-512]
    m = mspace_map(mspace, 500 * _Mib_, 12 * _Mib_, NULL, 0, VMA_FG_HEAP);
    ck_assert((size_t)m == 500 * _Mib_ && errno == 0);
    // We try to take [480-504], it should fails
    m = mspace_map(mspace, 480 * _Mib_, 24 * _Mib_, NULL, 0,
                   VMA_FG_HEAP | VMA_MAP_FIXED);
    ck_assert(m == NULL && errno == ERANGE);
    // We try to take 64Mb, should end up at [1-65]
    m = mspace_map(mspace, 0, 64 * _Mib_, NULL, 0, VMA_FG_HEAP);
    ck_assert((size_t)m == 1 * _Mib_ && errno == 0);
    // We grab [264-364]
    m = mspace_map(mspace, 264 * _Mib_, 100 * _Mib_, NULL, 0, VMA_FG_HEAP);
    ck_assert(m != NULL && errno == 0);
    // We grab [364-464]
    m = mspace_map(mspace, 364 * _Mib_, 100 * _Mib_, NULL, 0, VMA_FG_HEAP);
    ck_assert(m != NULL && errno == 0);
    // We grab [164-264]  -- We have taken here [1-65 | 164-464 | 500-512]
    m = mspace_map(mspace, 164 * _Mib_, 100 * _Mib_, NULL, 0, VMA_FG_HEAP);
    ck_assert(m != NULL && errno == 0);
    // We try to grab the full space between first and second [65-164]
    m = mspace_map(mspace, 65 * _Mib_, 99 * _Mib_, NULL, 0, VMA_FG_HEAP);
    ck_assert(m != NULL && errno == 0);
    // Only [464 - 500] sould be free !
    m = mspace_map(mspace, 0, 100 * _Mib_, NULL, 0, VMA_FG_HEAP);
    ck_assert(m == NULL && errno == ENOMEM);

    m = mspace_map(mspace, 500 * _Mib_, 24 * _Mib_, NULL, 0,
                   VMA_FG_HEAP | VMA_MAP_FIXED);
    ck_assert(m == NULL && errno == ERANGE);

    m = mspace_map(mspace, 280 * _Mib_, 2 * _Mib_, NULL, 0,
                   VMA_FG_HEAP | VMA_MAP_FIXED);
    ck_assert(m == NULL && errno == ERANGE);

    // mspace_display(0, mspace); // Only [464 - 500] is free !


    mspace_sweep(mspace);

}
END_TEST

void fixture_memory(Suite *s)
{
    TCase *tc = tcase_create("Memory");
    tcase_add_test(tc, test_memory_001);
    tcase_add_test(tc, test_memory_002);
    suite_add_tcase(s, tc);
}
