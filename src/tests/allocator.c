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
#include <kora/allocator.h>
#include <kora/mcrs.h>
#include <stdlib.h>
#include <errno.h>
#include <check.h>

START_TEST(test_arena_001)
{
    heap_arena_t arena;
    void *ptr = valloc(4096);
    void *limit = (char *)ptr + 4096;
    setup_arena(&arena, (size_t)ptr, 4096, 4096, HEAP_PARANO | HEAP_CHECK);

    void *p0 = malloc_r(&arena, 128);
    ck_assert(p0 >= ptr && p0 < limit);

    void *p1 = malloc_r(&arena, 128);
    ck_assert(p1 >= ptr && p1 < limit);

    void *p2 = malloc_r(&arena, 128);
    ck_assert(p2 >= ptr && p2 < limit);

    free_r(&arena, p1);

    void *p3 = malloc_r(&arena, 128);
    ck_assert(p3 >= ptr && p3 < limit);

    free_r(&arena, p3);
    free_r(&arena, p0);

    void *p4 = malloc_r(&arena, 128);
    ck_assert(p4 >= ptr && p4 < limit);

    free(ptr);
}
END_TEST

START_TEST(test_arena_002)
{
    heap_arena_t arena;
    void *ptr = valloc(4096);
    void *limit = (char *)ptr + 4096;
    setup_arena(&arena, (size_t)ptr, 4096, 4096, HEAP_PARANO | HEAP_CHECK);

    void *p0 = malloc_r(&arena, 128);
    ck_assert(p0 >= ptr && p0 < limit);

    void *p1 = malloc_r(&arena, 256);
    ck_assert(p1 >= ptr && p1 < limit);

    void *p2 = malloc_r(&arena, 512);
    ck_assert(p2 >= ptr && p2 < limit);

    free_r(&arena, p1);

    void *p3 = malloc_r(&arena, 64);
    ck_assert(p3 >= ptr && p3 < limit);

    void *p4 = malloc_r(&arena, 70);
    ck_assert(p4 >= ptr && p4 < limit);

    void *p5 = malloc_r(&arena, 80);
    ck_assert(p5 >= ptr && p5 < limit);

    free_r(&arena, p5);

    void *p6 = malloc_r(&arena, 70);
    ck_assert(p6 >= ptr && p6 < limit);

    free_r(&arena, p4);
    free_r(&arena, p2);
    free_r(&arena, p0);

    free_r(&arena, p3);
    free_r(&arena, p6);

    free(ptr);
}
END_TEST

START_TEST(test_arena_003)
{
    heap_arena_t arena;
    void *ptr = valloc(4096);
    void *limit = (char *)ptr + 4096;
    setup_arena(&arena, (size_t)ptr, 4096, 4096, HEAP_PARANO | HEAP_CHECK);

    ck_assert(malloc_r(&arena, _Gib_) == NULL);

    void *p0 = malloc_r(&arena, 60);
    ck_assert(p0 >= ptr && p0 < limit);

    void *p1 = malloc_r(&arena, 60);
    ck_assert(p1 >= ptr && p1 < limit);

    void *p2 = malloc_r(&arena, 60);
    ck_assert(p2 >= ptr && p2 < limit);

    void *p3 = malloc_r(&arena, 60);
    ck_assert(p3 >= ptr && p3 < limit);

    void *p4 = malloc_r(&arena, 60);
    ck_assert(p4 >= ptr && p4 < limit);

    void *p5 = malloc_r(&arena, 60);
    ck_assert(p5 >= ptr && p5 < limit);

    free_r(&arena, p0);
    free_r(&arena, p2);
    free_r(&arena, p4);

    void *p6 = malloc_r(&arena, 128);
    ck_assert(p6 >= ptr && p6 < limit);

    free_r(&arena, p3);
    free_r(&arena, p5);
    free_r(&arena, p1);

    void *p7 = malloc_r(&arena, 1024);
    ck_assert(p7 >= ptr && p7 < limit);

    void *p8 = malloc_r(&arena, 1024);
    ck_assert(p8 >= ptr && p8 < limit);

    void *p9 = malloc_r(&arena, 1024);
    ck_assert(p9 >= ptr && p9 < limit);
    ck_assert(errno == 0);

    ck_assert(malloc_r(&arena, 1024) == NULL);
    ck_assert(errno == ENOMEM);


    free_r(&arena, p6);

    free(ptr);
}
END_TEST

START_TEST(test_heap_001)
{
    void *ptr = valloc(4096);
    void *limit = (char *)ptr + 4096;
    setup_allocator(ptr, 4096);

    void *p0 = malloc_p(128);
    ck_assert(p0 >= ptr && p0 < limit);

    void *p1 = malloc_p(128);
    ck_assert(p1 >= ptr && p1 < limit);

    void *p2 = malloc_p(128);
    ck_assert(p2 >= ptr && p2 < limit);

    free_p(p1);

    void *p3 = malloc_p(128);
    ck_assert(p3 >= ptr && p3 < limit);

    free_p(p3);
    free_p(p0);

    void *p4 = calloc_p(64, 2);
    ck_assert(p4 >= ptr && p4 < limit);

    free_p(p4);

    sweep_allocator();
    free(ptr);
}
END_TEST

void fixture_allocator(Suite *s)
{
    TCase *tc;

    tc = tcase_create("Allocator arena");
    tcase_add_test(tc, test_arena_001);
    tcase_add_test(tc, test_arena_002);
    tcase_add_test(tc, test_arena_003);
    suite_add_tcase(s, tc);

    tc = tcase_create("Allocator heap");
    tcase_add_test(tc, test_heap_001);
    suite_add_tcase(s, tc);
}
