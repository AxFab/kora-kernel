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
#ifndef _SRC_CHECK_H
#define _SRC_CHECK_H

#include <kora/mcrs.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>
#include <errno.h>

void nanosleep(struct timespec *, struct timespec *);

#define START_TEST(n) void n() {
#define END_TEST }
#define ck_assert(e) do { if (!(e)) ck_fails(#e, __FILE__, __LINE__); } while(0)
#define ck_ok(e,m)  ck_assert(e)
#define tcase_add_test(t,f)  tcase_add_test_(t,#f,f)

void kprintf(int no, const char *fmt, ...);
void *kalloc(size_t lg);
void kfree(void *ptr);

#if !defined(_WIN32)
// static inline void * _valloc(size_t s) {
//     void *ptr = valloc(s);
//     printf("VALLOC %p (%x)\n",s, ptr);
//     return ptr;
// }
// static inline void _vfree(void*p) {
//     free(p);
//     printf("VFREE %p\n",p);
// }
# define _valloc(s)  valloc(s)
# define _vfree(p)  free(p)
#else
# define _valloc(s)  _aligned_malloc(s, PAGE_SIZE)
# define _vfree(p)  _aligned_free(p)
#endif

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef struct SRunner {
    int count;
    int success;
} SRunner;

typedef struct Suite {
    int count;
    int success;
} Suite;

typedef struct TCase {
    int count;
    int success;
} TCase;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define CK_NORMAL  0

extern jmp_buf __tcase_jump;

static inline SRunner *srunner_create()
{
    SRunner *runner = kalloc(sizeof(SRunner));
    return runner;
}

static inline void srunner_free(SRunner *runner)
{
    kfree(runner);
}

static inline int srunner_ntests_failed(SRunner *runner)
{
    return runner->count - runner->success;
}

static inline void srunner_run_all(SRunner *runner, int flags)
{
    if (runner->count == runner->success)
        kprintf(-1, "  \033[92mTotal of %d unit-tests passed with success.\033[0m\n", runner->count);
    else
        kprintf(-1, "  \033[91mTest failure, only %d of %d tests succeed (%d%%).\033[0m\n", runner->success, runner->count, runner->success * 100 / runner->count);
}

static inline void srunner_add_suite(SRunner *runner, Suite *suite)
{
    runner->count += suite->count;
    runner->success += suite->success;
    kfree(suite);
}


static inline Suite *suite_create(const char *name)
{
    kprintf(-1, "\033[1;36m%s\033[0m\n", name);
    Suite *suite = kalloc(sizeof(Suite));
    return suite;
}

static inline TCase *tcase_create(const char *name)
{
    kprintf(-1, "  \033[1;35m%s\033[0m\n", name);
    TCase *tcase = kalloc(sizeof(TCase));
    return tcase;
}

static inline void tcase_add_test_(TCase *tc, const char *name, void(*test)())
{
    tc->count++;
    if (setjmp(__tcase_jump) == 0) {
        test();
        kprintf(-1, "\033[90m%24s %s%s\033[0m\n", name, "\033[32m", "OK");
        tc->success++;
    } else
        kprintf(-1, "\033[90m%24s %s%s\033[0m\n", name, "\033[31m", "FAILS");
}

static inline void suite_add_tcase(Suite *suite, TCase *tcase)
{
    suite->count += tcase->count;
    suite->success += tcase->success;
    kfree(tcase);
}


static inline void ck_fails(const char *expr, const char *file, int line)
{
    kprintf(-1, "\033[91mCheck fails at %s l.%d: %s\033[0m\n", file, line, expr);
    longjmp(__tcase_jump, 1);
}


#endif  /* _SRC_CHECK_H */
