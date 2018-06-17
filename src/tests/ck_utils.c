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
// #include <kora/format.h>
#include <kora/bbtree.h>
#include <kora/llist.h>
#include <kora/hmap.h>
#include <kora/splock.h>
#include <kora/rwlock.h>
#include <kora/mcrs.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
// #include <check.h>




#define START_TEST(n) void n() {
#define END_TEST }
#define CK_CASE(n) CK_CASE_(#n, n)

void kprintf(int no, const char *fmt, ...);


int cases_count = 0;

void CK_END()
{
    kprintf(-1, "  Total of %d tests passed with success.\n", cases_count);
}

void CK_SUITE(const char *name)
{
    kprintf(-1, "\033[1;36m%s\033[0m\n", name);
}
void CK_FIXTURE(const char *name)
{
    kprintf(-1, "  \033[1;37m%s\033[0m\n", name);
}
void CK_CASE_(const char *name, void(*func)())
{
    cases_count++;
    func();
    kprintf(-1, "\033[90m%24s %s%s\033[0m\n", name, "\033[32m", "OK");
}

#define ck_assert(e) do { if (!(e)) ck_fails(#e, __FILE__, __LINE__); } while(0)
static inline void ck_fails(const char *expr, const char *file, int line)
{
    kprintf(-1, "Check fails at %s l.%d: %s\n", file, line, expr);
    abort();
}



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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static bbnode_t **test_bbtree_alloc(int count)
{
    int i;
    bbnode_t **array = (bbnode_t **)malloc(sizeof(bbnode_t *) * (count + 1));
    for (i = 0; i < count; ++i) {
        array[i] = (bbnode_t *)malloc(sizeof(bbnode_t));
        array[i]->value_ = i;
    }
    array[count] = NULL;
    return array;
}

static void test_bbtree_free(bbnode_t **array)
{
    int i;
    for (i = 0; array[i]; ++i) {
        ck_assert(array[i]->value_ == (size_t)i);
        free(array[i]);
    }
    free(array);
}

START_TEST(test_bbtree_001)
{
    bbtree_t tree;
    bbtree_init(&tree);
    bbnode_t n1 = INIT_BBNODE(1);
    bbnode_t n2 = INIT_BBNODE(2);
    bbnode_t n3 = INIT_BBNODE(3);
    bbnode_t n5 = INIT_BBNODE(5);
    bbnode_t n8 = INIT_BBNODE(8);
    bbnode_t n9 = INIT_BBNODE(9);
    bbnode_t n10 = INIT_BBNODE(10);
    bbnode_t n11 = INIT_BBNODE(11);
    bbnode_t n13 = INIT_BBNODE(13);
    bbtree_insert(&tree, &n1);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_insert(&tree, &n2);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_insert(&tree, &n3);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_insert(&tree, &n5);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_insert(&tree, &n8);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_insert(&tree, &n9);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_remove(&tree, n3.value_);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_remove(&tree, n8.value_);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_insert(&tree, &n3);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_insert(&tree, &n8);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_insert(&tree, &n10);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_insert(&tree, &n11);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_insert(&tree, &n13);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
    bbtree_remove(&tree, n1.value_);
    ck_assert(tree.count_ == bbtree_check(tree.root_));
}
END_TEST

START_TEST(test_bbtree_002)
{
    bbtree_t tree;
    bbtree_init(&tree);
    bbnode_t n1 = INIT_BBNODE(1);
    bbnode_t n2 = INIT_BBNODE(2);
    bbnode_t n3 = INIT_BBNODE(3);
    bbnode_t n4 = INIT_BBNODE(4);
    bbnode_t n5 = INIT_BBNODE(5);
    bbnode_t n6 = INIT_BBNODE(6);
    bbnode_t n7 = INIT_BBNODE(7);
    bbnode_t n8 = INIT_BBNODE(8);
    bbnode_t n9 = INIT_BBNODE(9);
    bbnode_t n10 = INIT_BBNODE(10);
    bbtree_insert(&tree, &n1);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 1);
    bbtree_insert(&tree, &n5);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 2);
    bbtree_insert(&tree, &n6);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 3);
    bbtree_insert(&tree, &n8);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 4);
    bbtree_insert(&tree, &n2);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 5);
    bbtree_insert(&tree, &n3);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 6);
    bbtree_insert(&tree, &n7);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 7);
    bbtree_insert(&tree, &n9);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 8);
    bbtree_insert(&tree, &n4);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 9);
    bbtree_insert(&tree, &n10);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 10);

    bbtree_remove(&tree, n2.value_);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 9);
    bbtree_remove(&tree, n4.value_);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 8);
    bbtree_remove(&tree, n5.value_);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 7);
    bbtree_remove(&tree, n6.value_);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 6);
    bbtree_remove(&tree, n7.value_);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 5);
    bbtree_remove(&tree, n8.value_);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 4);
}
END_TEST


void test_bbtree_003_sub(int v, int f, int s)
{
    int i;
    bbtree_t tree;
    bbtree_init(&tree);
    bbnode_t **nodes = test_bbtree_alloc(s);
    for (i = 0; i < s; ++i) {
        bbtree_insert(&tree, nodes[i]);
        ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == (i + 1));
    }

    bbtree_remove(&tree, v);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == f);
    test_bbtree_free(nodes);
}

START_TEST(test_bbtree_003)
{
    int i, j;
    for (j = 2; j < 99; ++j) {
        for (i = 0; i < j; ++i)
            test_bbtree_003_sub(i, j - 1, j);
        test_bbtree_003_sub(j, j, j);
    }
}
END_TEST

START_TEST(test_bbtree_004)
{
    bbtree_t tree;
    bbtree_init(&tree);
    bbnode_t n1 = INIT_BBNODE(1);
    bbtree_insert(&tree, &n1);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 1);
    bbtree_remove(&tree, n1.value_);
    ck_assert(tree.root_->level_ == 0 && tree.count_ == 0);
}
END_TEST

START_TEST(test_bbtree_005)
{
    int ret;
    bbtree_t tree;
    bbtree_init(&tree);
    bbnode_t n1 = INIT_BBNODE(1);
    bbnode_t n2 = INIT_BBNODE(1);
    ret = bbtree_insert(&tree, &n1);
    ck_assert(ret == 0);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 1);
    ret = bbtree_insert(&tree, &n2);
    ck_assert(ret < 0);
    ret = bbtree_remove(&tree, n1.value_);
    ck_assert(ret == 0);
    ck_assert(tree.root_->level_ == 0 && tree.count_ == 0);
    ret = bbtree_remove(&tree, n2.value_);
    ck_assert(ret < 0);

}
END_TEST

START_TEST(test_bbtree_006)
{
    bbtree_t tree;
    bbtree_init(&tree);
    bbnode_t n1 = INIT_BBNODE(1);
    bbnode_t n2 = INIT_BBNODE(2);
    bbnode_t n3 = INIT_BBNODE(3);
    bbnode_t n4 = INIT_BBNODE(4);
    bbnode_t n5 = INIT_BBNODE(5);
    bbnode_t *r;

    ck_assert(bbtree_insert(&tree, &n3) == 0);
    ck_assert(bbtree_insert(&tree, &n2) == 0);
    ck_assert(bbtree_insert(&tree, &n5) == 0);
    ck_assert(bbtree_insert(&tree, &n4) == 0);
    ck_assert(bbtree_insert(&tree, &n1) == 0);

    r = bbtree_search_(tree.root_, 3, 0);
    ck_assert(r && r->value_ == 3);

    r = bbtree_search_(tree.root_, 5, 0);
    ck_assert(r && r->value_ == 5);

    r = bbtree_search_(tree.root_, 2, 0);
    ck_assert(r && r->value_ == 2);

    r = bbtree_search_(tree.root_, 1, 0);
    ck_assert(r && r->value_ == 1);

    r = bbtree_search_(tree.root_, 4, 0);
    ck_assert(r && r->value_ == 4);

    r = bbtree_search_(tree.root_, 6, 0);
    ck_assert(r == NULL);

    r = bbtree_search_(tree.root_, 0, 0);
    ck_assert(r == NULL);

    r = bbtree_left_(tree.root_);
    ck_assert(r && r->value_ == 1);
    r = bbtree_next_(r);
    ck_assert(r && r->value_ == 2);
    r = bbtree_next_(r);
    ck_assert(r && r->value_ == 3);
    r = bbtree_next_(r);
    ck_assert(r && r->value_ == 4);
    r = bbtree_next_(r);
    ck_assert(r && r->value_ == 5);
    r = bbtree_next_(r);
    ck_assert(r == NULL);

    r = bbtree_right_(tree.root_);
    ck_assert(r && r->value_ == 5);
    r = bbtree_previous_(r);
    ck_assert(r && r->value_ == 4);
    r = bbtree_previous_(r);
    ck_assert(r && r->value_ == 3);
    r = bbtree_previous_(r);
    ck_assert(r && r->value_ == 2);
    r = bbtree_previous_(r);
    ck_assert(r && r->value_ == 1);
    r = bbtree_previous_(r);
    ck_assert(r == NULL);

}
END_TEST

START_TEST(test_bbtree_007)
{
    bbnode_t *r;
    bbtree_t tree;
    bbtree_init(&tree);

    r = bbtree_search_(tree.root_, 1, 0);
    ck_assert(r == NULL);

    r = bbtree_left_(tree.root_);
    ck_assert(r == NULL);

    r = bbtree_right_(tree.root_);
    ck_assert(r == NULL);

}
END_TEST


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


// /* Generic type tests */
// START_TEST(test_print_001)
// {
//     char *buffer = malloc(1024);
//     sprintf_p(buffer, "Hello %s!\n", "World");
//     ck_assert(strcmp(buffer, "Hello World!\n") == 0);

//     snprintf_p(buffer, 1024, "Hello %x!\n", 0x15ac);
//     ck_assert(strcmp(buffer, "Hello 15ac!\n") == 0);

//     snprintf_p(buffer, 1024, "Hello %X!\n", 0x15ac);
//     ck_assert(strcmp(buffer, "Hello 15AC!\n") == 0);

//     snprintf_p(buffer, 1024, "Hello %d!\n", 1564);
//     ck_assert(strcmp(buffer, "Hello 1564!\n") == 0);

//     snprintf_p(buffer, 1024, "Hello %o!\n", 0375);
//     ck_assert(strcmp(buffer, "Hello 375!\n") == 0);

//     free(buffer);

// }
// END_TEST

// /* Format arguments tests */
// START_TEST(test_print_002)
// {
//     char *buffer = malloc(1024);
//     sprintf_p(buffer, "Bytes: %#02x %02x %02x %02x", 0x89, 0x4, 0, 0xf6);
//     ck_assert(strcmp(buffer, "Bytes: 0x89 04 00 f6") == 0);

//     // sprintf_p(buffer, "Align: %20s", "left");
//     // ck_assert(strcmp(buffer, "Align: left                ") == 0);

//     // sprintf_p(buffer, "Align: %20s", "right");
//     // ck_assert(strcmp(buffer, "Align:                right") == 0);

//     free(buffer);

// }
// END_TEST

// /* Corner case tests */
// START_TEST(test_print_003)
// {
//     char *buffer = malloc(1024);
//     sprintf_p(buffer, "Pourcentage: %d%%", 10);
//     ck_assert(strcmp(buffer, "Pourcentage: 10%") == 0);

//     sprintf_p(buffer, "Null string are: %s", NULL);
//     ck_assert(strcmp(buffer, "Null string are: (null)") == 0);

//     free(buffer);

// }
// END_TEST

// void fixture_format(Suite *s)
// {
//     TCase *tc;

//     tc = tcase_create("Format print");
//     tcase_add_test(tc, test_print_001);
//     tcase_add_test(tc, test_print_002);
//     tcase_add_test(tc, test_print_003);
//     suite_add_tcase(s, tc);
// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


START_TEST(test_hmap_001)
{
    HMP_map map;
    hmp_init(&map, 4);
    hmp_put(&map, "banana", 6, (void *)0x45);
    hmp_put(&map, "oignon", 6, (void *)0x78);
    ck_assert(hmp_get(&map, "patato", 6) == NULL);
    ck_assert(hmp_get(&map, "oignon", 6) == (void *)0x78);
    hmp_remove(&map, "oignon", 6);
    ck_assert(hmp_get(&map, "oignon", 6) == NULL);
    ck_assert(hmp_get(&map, "banana", 6) == (void *)0x45);
    hmp_remove(&map, "banana", 6);
    hmp_destroy(&map, 1);

}
END_TEST

START_TEST(test_hmap_002)
{
    HMP_map map;
    hmp_init(&map, 4);
    ck_assert(map.mask_ + 1 == 4);
    hmp_put(&map, "#01", 3, (void *)0x45);
    hmp_put(&map, "#02", 3, (void *)0x78);
    hmp_put(&map, "#03", 3, (void *)0x28);
    hmp_put(&map, "#04", 3, (void *)0x17);
    hmp_put(&map, "#05", 3, (void *)0x93);
    hmp_put(&map, "#06", 3, (void *)0x75);
    hmp_put(&map, "#07", 3, (void *)0x32);
    hmp_put(&map, "#08", 3, (void *)0x71);
    hmp_put(&map, "#09", 3, (void *)0x20);
    hmp_put(&map, "#10", 3, (void *)0x19);
    hmp_put(&map, "#11", 3, (void *)0x96);
    hmp_put(&map, "#12", 3, (void *)0x24);
    hmp_put(&map, "#13", 3, (void *)0x37);
    hmp_put(&map, "#14", 3, (void *)0x89);
    hmp_put(&map, "#15", 3, (void *)0x66);
    hmp_put(&map, "#16", 3, (void *)0x11);
    ck_assert(map.mask_ + 1 == 8);
    hmp_remove(&map, "#09", 3);
    ck_assert(hmp_get(&map, "#09", 3) == NULL);
    ck_assert(hmp_get(&map, "#16", 3) == (void *)0x11);
    hmp_remove(&map, "#07", 3);
    hmp_destroy(&map, 1);

}
END_TEST

START_TEST(test_hmap_003)
{
    HMP_map map;
    hmp_init(&map, 4);
    hmp_remove(&map, "#AA", 3);
    hmp_put(&map, "#01", 3, (void *)0x45);
    hmp_put(&map, "#02", 3, (void *)0x78);
    ck_assert(hmp_get(&map, "#02", 3) == (void *)0x78);
    hmp_put(&map, "#02", 3, (void *)0x77);
    ck_assert(hmp_get(&map, "#02", 3) == (void *)0x77);
    hmp_put(&map, "#03", 3, (void *)0x28);
    hmp_put(&map, "#04", 3, (void *)0x17);
    hmp_put(&map, "#05", 3, (void *)0x93);
    hmp_put(&map, "#06", 3, (void *)0x75);
    hmp_put(&map, "#07", 3, (void *)0x32);
    hmp_remove(&map, "#03", 3);
    hmp_destroy(&map, 1);

}
END_TEST



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

START_TEST(test_integer_001)
{
    char *ptr;

    ck_assert(strtol("157", &ptr, 10) == 157);
    ck_assert(strcmp(ptr, "") == 0);
    ck_assert(errno == 0);

    ck_assert(strtol("-983", &ptr, 10) == -983);
    ck_assert(strcmp(ptr, "") == 0);
    ck_assert(errno == 0);

    ck_assert(strtol("20", &ptr, 16) == 32);
    ck_assert(strcmp(ptr, "") == 0);
    ck_assert(errno == 0);

    ck_assert(strtol("42A", &ptr, 5) == 22);
    ck_assert(strcmp(ptr, "A") == 0);
    ck_assert(errno == 0);

    ck_assert(strtol("96!", &ptr, 12) == 114);
    ck_assert(strcmp(ptr, "!") == 0);
    ck_assert(errno == 0);

    ck_assert(strtol("Hello", &ptr, 5) == 0);
    ck_assert(strcmp(ptr, "Hello") == 0);
    ck_assert(errno == EINVAL);

    ck_assert(strtol("@78", &ptr, 10) == 0);
    ck_assert(strcmp(ptr, "@78") == 0);
    ck_assert(errno == EINVAL);

    ck_assert(strtol("5123456789", &ptr, 0) == 0);
    ck_assert(errno == EOVERFLOW);

    ck_assert(strtoul("01244", &ptr, 0) == 01244);
    ck_assert(errno == 0);

    ck_assert(strtoul("0x16EFac8", &ptr, 0) == 0x16EFac8);
    ck_assert(errno == 0);

    ck_assert(strtol("14", &ptr, 1) == 0);
    ck_assert(errno == EINVAL);

    ck_assert(strtol("96", &ptr, 87) == 0);
    ck_assert(errno == EINVAL);

    ck_assert(strtoul("14", &ptr, 1) == 0);
    ck_assert(errno == EINVAL);

    ck_assert(strtoul("96", &ptr, 87) == 0);
    ck_assert(errno == EINVAL);

    ck_assert(strtoul("-879", &ptr, 10) == 0);
    ck_assert(errno == EINVAL);

}
END_TEST

START_TEST(test_integer_002)
{
    ck_assert(atoi("1244") == 1244);
    ck_assert(atoi("+546") == 546);
    ck_assert(atoi("   -864") == -864);
    ck_assert(atoi(" \n\t 989") == 989);

    ck_assert(atol("-984") == -984);
    ck_assert(atol("7876") == 7876);
    ck_assert(atol(" +8465") == 8465);

}
END_TEST


char *itoa(int, char *, int);

START_TEST(test_integer_003)
{
    char buf[50];

    ck_assert(strcmp(itoa(546, buf, 10), "546") == 0);
    ck_assert(strcmp(itoa(144, buf, 12), "100") == 0);
    ck_assert(strcmp(itoa(0, buf, 7), "0") == 0);
    ck_assert(strcmp(itoa(0, buf, 10), "0") == 0);
    ck_assert(strcmp(itoa(-9464, buf, 10), "-9464") == 0);
    ck_assert(itoa(464, buf, 0) == NULL);
    ck_assert(itoa(798654, buf, 765) == NULL);

}
END_TEST


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


struct data {
    int value;
    llnode_t node;
};
#define INIT_DATA(n) {n,{NULL,NULL}}

START_TEST(test_llist_001)
{
    struct data l1 = INIT_DATA(1);
    struct data l2 = INIT_DATA(2);
    struct data l3 = INIT_DATA(3);
    struct data l4 = INIT_DATA(4);
    struct data *r;
    llhead_t head = INIT_LLHEAD;

    ll_push_back(&head, &l1.node);
    ll_push_back(&head, &l2.node);
    ll_push_back(&head, &l3.node);
    ll_push_back(&head, &l4.node);

    r = itemof(ll_pop_front(&head), struct data, node);
    ck_assert(r && r->value == 1);
    r = itemof(ll_pop_front(&head), struct data, node);
    ck_assert(r && r->value == 2);
    ll_push_front(&head, &l1.node);
    r = itemof(ll_pop_back(&head), struct data, node);
    ck_assert(r && r->value == 4);
    ll_push_front(&head, &l4.node);
    ll_remove(&head, &l3.node);
    r = itemof(ll_pop_front(&head), struct data, node);
    ck_assert(r && r->value == 4);
    ll_remove(&head, &l1.node);
    r = itemof(ll_pop_front(&head), struct data, node);
    ck_assert(r == NULL);
    ll_push_back(&head, &l3.node);
    r = itemof(ll_pop_front(&head), struct data, node);
    ck_assert(r && r->value == 3);

}
END_TEST

START_TEST(test_llist_002)
{
    struct data l1 = INIT_DATA(1);
    struct data l2 = INIT_DATA(2);
    struct data l3 = INIT_DATA(3);
    struct data l4 = INIT_DATA(4);
    struct data *r;
    llhead_t head = INIT_LLHEAD;

    ll_push_front(&head, &l1.node);
    ll_push_front(&head, &l2.node);
    ll_push_front(&head, &l3.node);
    ll_push_front(&head, &l4.node);

    r = itemof(ll_pop_back(&head), struct data, node);
    ck_assert(r && r->value == 1);
    r = itemof(ll_pop_back(&head), struct data, node);
    ck_assert(r && r->value == 2);
    ll_push_back(&head, &l1.node);
    r = itemof(ll_pop_front(&head), struct data, node);
    ck_assert(r && r->value == 4);
    ll_push_back(&head, &l4.node);
    ll_remove(&head, &l3.node);
    r = itemof(ll_pop_back(&head), struct data, node);
    ck_assert(r && r->value == 4);
    ll_remove(&head, &l1.node);
    r = itemof(ll_pop_back(&head), struct data, node);
    ck_assert(r == NULL);
    ll_push_front(&head, &l3.node);
    r = itemof(ll_pop_back(&head), struct data, node);
    ck_assert(r && r->value == 3);

}
END_TEST


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

START_TEST(test_splock_001)
{
    splock_t lock;
    splock_init(&lock);
    splock_lock(&lock);
    ck_assert(splock_locked(&lock));
    splock_unlock(&lock);
    ck_assert(!splock_locked(&lock));

}
END_TEST

START_TEST(test_splock_002)
{
    splock_t lock;
    splock_init(&lock);
    ck_assert(splock_trylock(&lock) == true);
    ck_assert(splock_locked(&lock));
    ck_assert(splock_trylock(&lock) == false);
    splock_unlock(&lock);
    ck_assert(!splock_locked(&lock));
    cpu_relax();

}
END_TEST


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

START_TEST(test_rwlock_001)
{
    rwlock_t lock;
    rwlock_init(&lock);
    ck_assert(rwlock_rdtrylock(&lock) == true);//should ok
    ck_assert(rwlock_wrtrylock(&lock) == false);//should fail
    ck_assert(!rwlock_wrlocked(&lock));
    rwlock_rdunlock(&lock);
    ck_assert(rwlock_wrtrylock(&lock) == true);//should ok
    ck_assert(rwlock_wrlocked(&lock));
    ck_assert(rwlock_rdtrylock(&lock) == false);//should fail
    ck_assert(rwlock_wrtrylock(&lock) == false);//should fail
    rwlock_wrunlock(&lock);
    ck_assert(rwlock_wrtrylock(&lock) == true);//should ok
    ck_assert(rwlock_wrlocked(&lock));
    // ck_assert(rwlock_wrlock(&lock) == false);//should wait
    rwlock_wrunlock(&lock);

}
END_TEST

START_TEST(test_rwlock_002)
{
    rwlock_t lock;
    rwlock_init(&lock);
    rwlock_wrlock(&lock);
    ck_assert(rwlock_wrlocked(&lock));
    ck_assert(rwlock_rdtrylock(&lock) == false);
    rwlock_wrunlock(&lock);
    ck_assert(!rwlock_wrlocked(&lock));
    rwlock_rdlock(&lock);
    rwlock_rdlock(&lock);
    ck_assert(rwlock_wrtrylock(&lock) == false);//should fail
    ck_assert(!rwlock_wrlocked(&lock));
    rwlock_rdunlock(&lock);
    ck_assert(rwlock_wrtrylock(&lock) == false);//should fail
    ck_assert(!rwlock_wrlocked(&lock));
    rwlock_rdunlock(&lock);
    ck_assert(rwlock_wrtrylock(&lock) == true);//should ok
    ck_assert(rwlock_wrlocked(&lock));
    rwlock_wrunlock(&lock);

}
END_TEST

START_TEST(test_rwlock_003)
{
    rwlock_t lock;
    rwlock_init(&lock);
    rwlock_rdlock(&lock);
    ck_assert(!rwlock_wrlocked(&lock));
    ck_assert(rwlock_wrtrylock(&lock) == false);//should fail
    ck_assert(!rwlock_wrlocked(&lock));
    ck_assert(rwlock_upgrade(&lock) == true);
    ck_assert(rwlock_wrlocked(&lock));
    ck_assert(rwlock_wrtrylock(&lock) == false);//should fail
    rwlock_wrunlock(&lock);
    ck_assert(rwlock_wrtrylock(&lock) == true);
    ck_assert(rwlock_upgrade(&lock) == false);
    rwlock_wrunlock(&lock);

}
END_TEST


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

START_TEST(test_memop_001)
{
    const char *msg = "ABCDEFGHIJKLMNOP";
    char msg0[6];
    memset(msg0, 0x55, 6);
    ck_assert(msg0[0] == 0x55);
    ck_assert(msg0[5] == 0x55);

    memcpy(msg0, msg, 6);
    ck_assert(msg0[0] == 'A');
    ck_assert(msg0[5] == 'F');
    ck_assert(memcmp(msg0, msg, 6) == 0);

    memset(&msg0[3], 0xAA, 3);
    ck_assert(memcmp(msg0, msg, 6) != 0);

}
END_TEST





/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void test_time_convert(time_t time, const char *value)
{
    const char *fmt;
    struct tm datetime;
    gmtime_r(&time, &datetime);
    ck_assert(time == mktime(&datetime));
    ck_assert(time == mktime(gmtime(&time)));
    fmt = asctime(&datetime);
    ck_assert(strcmp(fmt, value) == 0);
}

START_TEST(test_time_001)
{
    test_time_convert(0xbeaf007, "Mon May  3 04:37:27 1976\n");
    test_time_convert(1, "Thu Jan  1 00:00:01 1970\n");
    test_time_convert(790526, "Sat Jan 10 03:35:26 1970\n");
    test_time_convert(0x7fffffff, "Tue Jan 19 03:14:07 2038\n");
    test_time_convert(1221253494, "Fri Sep 12 21:04:54 2008\n");
    test_time_convert(951876312, "Wed Mar  1 02:05:12 2000\n");
    test_time_convert(951811944, "Tue Feb 29 08:12:24 2000\n");
    if (sizeof(time_t) == 4)
        test_time_convert(0x80000000, "Fri Dec 13 20:45:52 1901\n");
}
END_TEST

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


int main(int argc, char **argv)
{
    CK_SUITE("Basics");
    CK_FIXTURE("Doubly-linked List");
    CK_CASE(test_llist_001);
    CK_CASE(test_llist_002);

    CK_FIXTURE("Self-Balanced Binary Tree");
    CK_CASE(test_bbtree_001);
    CK_CASE(test_bbtree_002);
    CK_CASE(test_bbtree_003);
    CK_CASE(test_bbtree_004);
    CK_CASE(test_bbtree_005);
    CK_CASE(test_bbtree_006);
    CK_CASE(test_bbtree_007);

    CK_FIXTURE("Spinlocks");
    CK_CASE(test_splock_001);
    CK_CASE(test_splock_002);

    CK_FIXTURE("Read-write Locks");
    CK_CASE(test_rwlock_001);
    CK_CASE(test_rwlock_002);
    CK_CASE(test_rwlock_003);

    CK_FIXTURE("Hash-Map");
    CK_CASE(test_hmap_001);
    CK_CASE(test_hmap_002);
    CK_CASE(test_hmap_003);

    CK_SUITE("Standard");
    CK_FIXTURE("Allocator arena");
    CK_CASE(test_arena_001);
    CK_CASE(test_arena_002);
    CK_CASE(test_arena_003);

    // kprintf(-1, "\033[1;37mAllocator heap\033[0m\n");
    // CK_CASE(test_heap_001);

    // kprintf(-1, "\033[1;37mString\033[0m\n");
    CK_FIXTURE("Memory operations");
    CK_CASE(test_memop_001);

    CK_FIXTURE("Format integer");
    CK_CASE(test_integer_001);
    CK_CASE(test_integer_002);
    CK_CASE(test_integer_003);

    kprintf(-1, "\033[1;37mFormat string\033[0m\n");
    CK_FIXTURE("Time format");
    CK_CASE(test_time_001);
    CK_END();


    //     // Create suites
    //     int errors;
    //     SRunner *sr = srunner_create(NULL);
    //     srunner_add_suite (sr, suite_basics());
    //     srunner_add_suite (sr, suite_standard());

    //     // Run test-suites
    //     srunner_run_all(sr, CK_NORMAL);
    //     errors = srunner_ntests_failed(sr);
    //     srunner_free(sr);
    //     return (errors == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

void *heap_map(size_t length)
{
    return NULL;
}
void *heap_unmap(void *address, size_t length)
{
    return NULL;
}
void vfs_read() {}
int cpu_no()
{
    return 0;
}