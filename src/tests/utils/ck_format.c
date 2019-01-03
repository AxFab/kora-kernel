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
#include <errno.h>
#include <kora/format.h>
#include "../check.h"


/* Generic type tests */
START_TEST(test_print_001)
{
    char *buffer = malloc(1024);
    sprintf_p(buffer, "Hello %s!\n", "World");
    ck_assert(strcmp(buffer, "Hello World!\n") == 0);

    snprintf_p(buffer, 1024, "Hello %x!\n", 0x15ac);
    ck_assert(strcmp(buffer, "Hello 15ac!\n") == 0);

    snprintf_p(buffer, 1024, "Hello %X!\n", 0x15ac);
    ck_assert(strcmp(buffer, "Hello 15AC!\n") == 0);

    snprintf_p(buffer, 1024, "Hello %d!\n", 1564);
    ck_assert(strcmp(buffer, "Hello 1564!\n") == 0);

    snprintf_p(buffer, 1024, "Hello %o!\n", 0375);
    ck_assert(strcmp(buffer, "Hello 375!\n") == 0);

    free(buffer);

}
END_TEST

/* Format arguments tests */
START_TEST(test_print_002)
{
    char *buffer = malloc(1024);
    sprintf_p(buffer, "Bytes: %#02x %02x %02x %02x", 0x89, 0x4, 0, 0xf6);
    ck_assert(strcmp(buffer, "Bytes: 0x89 04 00 f6") == 0);

    // sprintf_p(buffer, "Align: %20s", "left");
    // ck_assert(strcmp(buffer, "Align: left                ") == 0);

    // sprintf_p(buffer, "Align: %20s", "right");
    // ck_assert(strcmp(buffer, "Align:                right") == 0);

    free(buffer);

}
END_TEST

/* Corner case tests */
START_TEST(test_print_003)
{
    char *buffer = malloc(1024);
    sprintf_p(buffer, "Pourcentage: %d%%", 10);
    ck_assert(strcmp(buffer, "Pourcentage: 10%") == 0);

    sprintf_p(buffer, "Null string are: %s", NULL);
    ck_assert(strcmp(buffer, "Null string are: (null)") == 0);

    free(buffer);

}
END_TEST

void fixture_format(Suite *s)
{
    TCase *tc;

    tc = tcase_create("Format print");
    tcase_add_test(tc, test_print_001);
    tcase_add_test(tc, test_print_002);
    tcase_add_test(tc, test_print_003);
    suite_add_tcase(s, tc);
}
