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
#include <kora/mcrs.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <check.h>

void fixture_llist(Suite *s);
void fixture_bbtree(Suite *s);
void fixture_splock(Suite *s);
void fixture_rwlock(Suite *s);
void fixture_hmap(Suite *s);

void fixture_allocator(Suite *s);
void fixture_string(Suite *s);
void fixture_integer(Suite *s);
void fixture_format(Suite *s);
void fixture_time(Suite *s);

void fixture_memory(Suite *s);
void fixture_pages(Suite *s);
void fixture_device(Suite *s);
void fixture_vfs(Suite *s);

Suite *suite_basics(void)
{
    Suite *s;
    s = suite_create("Basics");
    fixture_llist(s);
    fixture_bbtree(s);
    fixture_splock(s);
    fixture_rwlock(s);
    fixture_hmap(s);
    return s;
}

Suite *suite_standard(void)
{
    Suite *s;
    s = suite_create("Standard");
    fixture_allocator(s);
    fixture_string(s);
    fixture_integer(s);
    fixture_format(s);
    fixture_time(s);
    return s;
}

Suite *suite_core(void)
{
    Suite *s;
    s = suite_create("Kernel core");
    fixture_memory(s);
    // fixture_pages(s);
    // fixture_device(s);
    // fixture_vfs(s);
    return s;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int main (int argc, char **argv)
{
    // Create suites
    int errors;
    SRunner *sr = srunner_create(NULL);
    srunner_add_suite (sr, suite_core());
    srunner_add_suite (sr, suite_basics());
    srunner_add_suite (sr, suite_standard());

    // Run test-suites
    srunner_run_all(sr, CK_NORMAL);
    errors = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (errors == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
