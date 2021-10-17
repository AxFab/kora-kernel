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
#include <kora/hmap.h>
#include <stdlib.h>
#include "../check.h"

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

void fixture_hmap(Suite *s)
{
    TCase *tc = tcase_create("Hash-Map");
    tcase_add_test(tc, test_hmap_001);
    tcase_add_test(tc, test_hmap_002);
    tcase_add_test(tc, test_hmap_003);
    suite_add_tcase(s, tc);
}
