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
#include <assert.h>
#include <limits.h>
#include <errno.h>
// #include <threads.h>
#include <pthread.h>
#include <stdlib.h>


/* -------------------------------------------------------------------------
 * ---------------------------------------------------------------------- */

/* Ensure POSIX.1-2008 extensions are visible (pthread_mutexattr_settype,
 * pthread_mutex_timedlock, nanosleep, sched_yield, …).  Must appear before
 * any system header is included.  Guard against a caller that already set
 * a stricter level. */
#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200809L
#elif _POSIX_C_SOURCE < 200112L
# undef  _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200809L
#endif

#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>

/* -------------------------------------------------------------------------
 * Types
 * ---------------------------------------------------------------------- */

typedef pthread_t          thrd_t;
typedef pthread_mutex_t    mtx_t;
typedef pthread_cond_t     cnd_t;
typedef pthread_key_t      tss_t;
typedef pthread_once_t     once_flag;

typedef int (*thrd_start_t)(void *);
typedef void (*tss_dtor_t)(void *);

#define ONCE_FLAG_INIT  PTHREAD_ONCE_INIT

/* -------------------------------------------------------------------------
 * Return codes
 * ---------------------------------------------------------------------- */

enum {
    thrd_success  = 0,
    thrd_error    = 1,
    thrd_nomem    = 2,
    thrd_timedout = 3,
    thrd_busy     = 4
};

/* -------------------------------------------------------------------------
 * Mutex type flags (may be OR-ed)
 * ---------------------------------------------------------------------- */

enum {
    mtx_plain     = 0,
    mtx_recursive = 1,
    mtx_timed     = 2
};

/* -------------------------------------------------------------------------
 * Thread functions
 *
 * thrd_create needs a small trampoline because C11 thread functions return
 * int but pthread start routines return void*.  The trampoline is declared
 * static so each translation unit that includes this header gets its own
 * copy — harmless for a test shim.
 * ---------------------------------------------------------------------- */

struct _c11_thrd_param {
    thrd_start_t func;
    void        *arg;
};


/* -------------------------------------------------------------------------
 * ---------------------------------------------------------------------- */


struct impl_thrd_param {
    thrd_start_t func;
    void *arg;
};

void *impl_thrd_routine(void *p)
{
    struct impl_thrd_param pack = *((struct impl_thrd_param *)p);
    free(p);
    void *ret = (void *)(size_t)pack.func(pack.arg);
    return ret;
}


int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    struct impl_thrd_param *pack;
    if (!thr)
        return thrd_error;
    pack = malloc(sizeof(struct impl_thrd_param));
    if (!pack)
        return thrd_nomem;
    pack->func = func;
    pack->arg = arg;
    if (pthread_create(thr, NULL, impl_thrd_routine, pack) != 0) {
        free(pack);
        return thrd_error;
    }
    return thrd_success;
}

int thrd_detach(thrd_t thr)
{
    return (pthread_detach(thr) == 0) ? thrd_success : thrd_error;
}

int thrd_equal(thrd_t thr0, thrd_t thr1)
{
    return pthread_equal(thr0, thr1);
}

void thrd_exit(int res)
{
    for (;;)
        pthread_exit((void *)(size_t)res);

}

int thrd_join(thrd_t thr, int *res)
{
    void *code;
    if (pthread_join(thr, &code) != 0)
        return thrd_error;
    if (res)
        *res = (int)(size_t)code;
    return thrd_success;
}

int thrd_sleep(const struct timespec *req, struct timespec *res)
{
    struct timespec tp = *(req);
    nanosleep(&tp, res);
    return 0;
}

void thrd_yield(void)
{
    sched_yield();
}

/* -------------------------------------------------------------------------
 * ---------------------------------------------------------------------- */

#if 1


/* -------------------------------------------------------------------------
 * Mutex
 * ---------------------------------------------------------------------- */

int mtx_init(mtx_t *m, int type)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    if (type & mtx_recursive)
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    else
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    int r = pthread_mutex_init(m, &attr);
    pthread_mutexattr_destroy(&attr);
    return r == 0 ? thrd_success : thrd_error;
}

void mtx_destroy(mtx_t *m)
{
    pthread_mutex_destroy(m);
}

int mtx_lock(mtx_t *m)
{
    return pthread_mutex_lock(m) == 0 ? thrd_success : thrd_error;
}

int mtx_trylock(mtx_t *m)
{
    int r = pthread_mutex_trylock(m);
    if (r == 0)     return thrd_success;
    if (r == EBUSY) return thrd_busy;
    return thrd_error;
}

int mtx_timedlock(mtx_t *m, const struct timespec *ts)
{
#if 0 // Check POSIX.1-2001
    int r = pthread_mutex_timedlock(m, ts);
    if (r == 0)         return thrd_success;
    if (r == ETIMEDOUT) return thrd_timedout;
#else
    // TODO -- Loop with pthread_mutex_trylock() with sleep() or select()
#endif
    return thrd_error;
}

int mtx_unlock(mtx_t *m)
{
    return pthread_mutex_unlock(m) == 0 ? thrd_success : thrd_error;
}

/* -------------------------------------------------------------------------
 * Condition variables
 * ---------------------------------------------------------------------- */

 int cnd_init(cnd_t *c)
{
    return pthread_cond_init(c, NULL) == 0 ? thrd_success : thrd_error;
}

void cnd_destroy(cnd_t *c)
{
    pthread_cond_destroy(c);
}

int cnd_signal(cnd_t *c)
{
    return pthread_cond_signal(c) == 0 ? thrd_success : thrd_error;
}

int cnd_broadcast(cnd_t *c)
{
    return pthread_cond_broadcast(c) == 0 ? thrd_success : thrd_error;
}

int cnd_wait(cnd_t *c, mtx_t *m)
{
    return pthread_cond_wait(c, m) == 0 ? thrd_success : thrd_error;
}

int cnd_timedwait(cnd_t *c, mtx_t *m, const struct timespec *ts)
{
    int r = pthread_cond_timedwait(c, m, ts);
    if (r == 0)         return thrd_success;
    if (r == ETIMEDOUT) return thrd_timedout;
    return thrd_error;
}

/* -------------------------------------------------------------------------
 * Thread-local storage
 * ---------------------------------------------------------------------- */

int tss_create(tss_t *key, tss_dtor_t dtor)
{
    return pthread_key_create(key, dtor) == 0 ? thrd_success : thrd_error;
}

void tss_delete(tss_t key)
{
    pthread_key_delete(key);
}

void *tss_get(tss_t key)
{
    return pthread_getspecific(key);
}

int tss_set(tss_t key, void *val)
{
    return pthread_setspecific(key, val) == 0 ? thrd_success : thrd_error;
}

/* -------------------------------------------------------------------------
 * One-time initialisation
 * ---------------------------------------------------------------------- */

void call_once(once_flag *flag, void (*func)(void))
{
    pthread_once(flag, func);
}

#endif
