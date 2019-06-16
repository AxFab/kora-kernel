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
#include <time.h>
#include <setjmp.h>
#include <errno.h>
#include <stddef.h>
#include <stdarg.h>
#include <bits/cdefs.h>
#include <bits/types.h>
#include <bits/timespec.h>

void *calloc(size_t len, size_t cnt);
void free();
void *valloc(size_t len);

int printf(const char *msg, ...);
int vprintf(const char *msg, va_list ap);

long long clock_read(int clockid);
int usleep(__useconds_t usecs);

void clock_gettime(int clockid, struct timespec *time_point);

_Noreturn void abort();



int sched_yield();

#ifndef KORA_KRN
#define __CPU_MASK_TYPE int
#include <pthread.h>
#endif

/*
typedef unsigned long int pthread_t;

int pthread_create(pthread_t *restrict thrd, const void *restrict attr, void *(*func)(void *), void *restrict arg);
int pthread_join(pthread_t thrd, void **ret);
int pthread_equal(pthread_t thrd1, pthread_t thrd2);
_Noreturn void pthread_exit(void *retval);
int pthread_detach(pthread_t thrd);
pthread_t pthread_self(void);
*/

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// void nanosleep(struct timespec *, struct timespec *);

#define TEST_CASE(n) void n()
#define ck_assert(e) do { if (!(e)) ck_fails(#e, __FILE__, __LINE__); } while(0)
#define ck_ok(e)  ck_assert(e)
#define tcase_create(f)  tcase_create_(#f,f)

void kprintf(int no, const char *fmt, ...);
void *kalloc(size_t lg);
void kfree(void *ptr);

#if !defined(_WIN32)
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



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define CK_NORMAL  0

extern jmp_buf __tcase_jump;
extern SRunner runner;

static inline int summarize()
{
    if (runner.count == runner.success)
        kprintf(-1, "  \033[92mTotal of %d unit-tests passed with success.\033[0m\n", runner.count);
    else
        kprintf(-1, "  \033[91mTest failure, only %d of %d tests succeed (%d%%).\033[0m\n", runner.success, runner.count, runner.success * 100 / runner.count);
    return runner.count - runner.success;
}

static inline void suite_create(const char *name)
{
    kprintf(-1, "\033[1;36m%s\033[0m\n", name);
}

static inline void fixture_create(const char *name)
{
    kprintf(-1, "  \033[1;35m%s\033[0m\n", name);
}

static inline void tcase_create_(const char *name, void(*test)())
{
    runner.count++;
    if (setjmp(__tcase_jump) == 0) {
        test();
        kprintf(-1, "\033[90m%24s %s%s\033[0m\n", name, "\033[32m", "OK");
        runner.success++;
    } else
        kprintf(-1, "\033[90m%24s %s%s\033[0m\n", name, "\033[31m", "FAILS");
}


static inline void ck_fails(const char *expr, const char *file, int line)
{
    kprintf(-1, "\033[91mCheck fails at %s l.%d: %s\033[0m\n", file, line, expr);
    longjmp(__tcase_jump, 1);
}

void ck_reset();

#endif  /* _SRC_CHECK_H */
