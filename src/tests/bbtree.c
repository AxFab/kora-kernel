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
#include <kora/bbtree.h>
#include <stdlib.h>
#include <check.h>

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

START_TEST(test_bbtree_001)
{
    bbtree_t tree = { &_NIL, 0 };
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
    bbtree_t tree = { &_NIL, 0 };
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
    bbtree_t tree = { &_NIL, 0 };
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
        for (i = 0; i < j; ++i) {
            test_bbtree_003_sub(i, j - 1, j);
        }
        test_bbtree_003_sub(j, j, j);
    }
}
END_TEST

START_TEST(test_bbtree_004)
{
    bbtree_t tree = { &_NIL, 0 };
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
    bbtree_t tree = { &_NIL, 0 };
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
    bbtree_t tree = { &_NIL, 0 };
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
    bbtree_t tree = { &_NIL, 0 };

    r = bbtree_search_(tree.root_, 1, 0);
    ck_assert(r == NULL);

    r = bbtree_left_(tree.root_);
    ck_assert(r == NULL);

    r = bbtree_right_(tree.root_);
    ck_assert(r == NULL);

}
END_TEST

void fixture_bbtree(Suite *s)
{
    TCase *tc = tcase_create("Self-Balanced Binary Tree");
    tcase_add_test(tc, test_bbtree_001);
    tcase_add_test(tc, test_bbtree_002);
    // tcase_add_test(tc, test_bbtree_003);
    tcase_add_test(tc, test_bbtree_004);
    tcase_add_test(tc, test_bbtree_005);
    tcase_add_test(tc, test_bbtree_006);
    tcase_add_test(tc, test_bbtree_007);
    suite_add_tcase(s, tc);
}
