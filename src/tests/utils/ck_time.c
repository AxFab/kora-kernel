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
#include <kora/splock.h>
#include <string.h>
#include <time.h>
#include "../check.h"

#undef INT_MAX
#define INT_MAX ((int)2147483647)

#undef INT_MIN
#define INT_MIN ((int)-INT_MAX - 1)

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
    test_time_convert(INT_MIN, "Fri Dec 13 20:45:52 1901\n");
    test_time_convert(INT_MAX, "Tue Jan 19 03:14:07 2038\n");
    test_time_convert(1221253494, "Fri Sep 12 21:04:54 2008\n");
    test_time_convert(951876312, "Wed Mar  1 02:05:12 2000\n");
    test_time_convert(951811944, "Tue Feb 29 08:12:24 2000\n");

}
END_TEST

void fixture_time(Suite *s)
{
    TCase *tc;

    tc = tcase_create("Time format");
    tcase_add_test(tc, test_time_001);
    suite_add_tcase(s, tc);
}
