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
#ifndef _SRC_CHECK_H
#define _SRC_CHECK_H

#include <kora/mcrs.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <setjmp.h>

void abort();
void nanosleep(struct timespec *, struct timespec *);

#define START_TEST(n) void n() {
#define END_TEST }
#define ck_case(n) ck_case_(#n, n)
#define ck_assert(e) do { if (!(e)) ck_fails(#e, __FILE__, __LINE__); } while(0)

void kprintf(int no, const char *fmt, ...);

int cases_count = 0;
jmp_buf case_jmp;

static inline void ck_end()
{
    kprintf(-1, "  Total of %d tests passed with success.\n", cases_count);
}

static inline void ck_suite(const char *name)
{
    kprintf(-1, "\033[1;36m%s\033[0m\n", name);
}
static inline void ck_fixture(const char *name)
{
    kprintf(-1, "  \033[1;37m%s\033[0m\n", name);
}

static inline void ck_case_(const char *name, void(*func)())
{
    cases_count++;
    if (setjmp(case_jmp) == 0) {
        func();
        kprintf(-1, "\033[90m%24s %s%s\033[0m\n", name, "\033[32m", "OK");
    } else {
        kprintf(-1, "\033[90m%24s %s%s\033[0m\n", name, "\033[31m", "FAILS");
    }
}

static inline void ck_fails(const char *expr, const char *file, int line)
{
    kprintf(-1, "Check fails at %s l.%d: %s\n", file, line, expr);
    longjmp(case_jmp, 1);
    abort();
}


#endif  /* _SRC_CHECK_H */
