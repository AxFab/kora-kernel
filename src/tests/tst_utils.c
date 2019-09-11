#include <kernel/core.h>
#include <kora/bbtree.h>
#include <kora/hmap.h>
#include <kora/llist.h>
#include <threads.h>
#include <assert.h>
#include "check.h"

#define INIT_BBNODE(n) { NULL, NULL, NULL, (n), 0 }

bbnode_t **test_bbtree_alloc(int count)
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

void test_bbtree_free(bbnode_t **array)
{
    int i;
    for (i = 0; array[i]; ++i) {
        ck_assert(array[i]->value_ == (size_t)i);
        free(array[i]);
    }
    free(array);
}

TEST_CASE(test_bbtree_001)
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

TEST_CASE(test_bbtree_002)
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

TEST_CASE(test_bbtree_003)
{
    int i, j;
    for (j = 2; j < 99; ++j) {
        for (i = 0; i < j; ++i)
            test_bbtree_003_sub(i, j - 1, j);
        test_bbtree_003_sub(j, j, j);
    }
}

TEST_CASE(test_bbtree_004)
{
    bbtree_t tree;
    bbtree_init(&tree);
    bbnode_t n1 = INIT_BBNODE(1);
    bbtree_insert(&tree, &n1);
    ck_assert(tree.count_ == bbtree_check(tree.root_) && tree.count_ == 1);
    bbtree_remove(&tree, n1.value_);
    ck_assert(tree.root_->level_ == 0 && tree.count_ == 0);
}

TEST_CASE(test_bbtree_005)
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

TEST_CASE(test_bbtree_006)
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

TEST_CASE(test_bbtree_007)
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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

TEST_CASE(test_hmap_001)
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

TEST_CASE(test_hmap_002)
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

TEST_CASE(test_hmap_003)
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


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


struct data {
    int value;
    llnode_t node;
};
#define INIT_DATA(n) {n,{NULL,NULL}}

TEST_CASE(test_llist_001)
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

TEST_CASE(test_llist_002)
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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */



jmp_buf __tcase_jump;
SRunner runner;

int main()
{
    suite_create("Utilities");
    fixture_create("Self-Balanced Binary Tree");
    tcase_create(test_bbtree_001);
    tcase_create(test_bbtree_002);
    tcase_create(test_bbtree_003);
    tcase_create(test_bbtree_004);
    tcase_create(test_bbtree_005);
    tcase_create(test_bbtree_006);
    tcase_create(test_bbtree_007);

    fixture_create("Hash-Map");
    tcase_create(test_hmap_001);
    tcase_create(test_hmap_002);
    tcase_create(test_hmap_003);

    fixture_create("Doubly-linked List");
    tcase_create(test_llist_001);
    tcase_create(test_llist_002);

    return summarize();
}

