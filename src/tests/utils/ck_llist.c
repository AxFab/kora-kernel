/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <kora/llist.h>
#include <stdlib.h>
#include "../check.h"

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

void fixture_llist(Suite *s)
{
    TCase *tc = tcase_create("Doubly-linked List");
    tcase_add_test(tc, test_llist_001);
    tcase_add_test(tc, test_llist_002);
    suite_add_tcase(s, tc);
}
