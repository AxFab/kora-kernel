/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "../check.h"

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

void fixture_integer(Suite *s)
{
    TCase *tc;

    tc = tcase_create("Format integer");
    tcase_add_test(tc, test_integer_001);
    tcase_add_test(tc, test_integer_002);
    tcase_add_test(tc, test_integer_003);
    suite_add_tcase(s, tc);
}
