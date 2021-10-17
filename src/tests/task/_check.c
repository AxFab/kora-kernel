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
#include <kora/mcrs.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "../check.h"

void fixture_starts(Suite *s);

Suite *suite_tasks(void)
{
    Suite *s;
    s = suite_create("Tasks");
    fixture_starts(s);
    return s;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
jmp_buf __tcase_jump;

int main(int argc, char **argv)
{
    // Create suites
    int errors;
    SRunner *sr = srunner_create(NULL);
    srunner_add_suite(sr, suite_tasks());

    // Run test-suites
    srunner_run_all(sr, CK_NORMAL);
    errors = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (errors == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
