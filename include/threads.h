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
#ifndef _THREADS_H
#define _THREADS_H 1

#include <bits/cdefs.h>
#include <stdatomic.h>
#include <time.h>

#define ONCE_FLAG_INIT {0}
#define TSS_DTOR_ITERATIONS 1
#define  TIME_UTC  1

#define thread_local __thread


typedef atomic_int once_flag;

/* Indicates a thread error status */
enum {
    thrd_error = -1,
    thrd_success = 0,
    thrd_busy = 1,
    thrd_timedout,
    thrd_nomem,
};

/* Defines the type of a mutex */
enum {
    mtx_plain = 0,
    mtx_timed = 1,
    mtx_recursive = 2,

    mtx_realtime = 4,
    mtx_robust = 8,
    mtx_priority = 16,
    mtx_shared = 32,
    mtx_checks = 64,
};



typedef int(*thrd_start_t)(void *);
typedef void(*tss_dtor_t)(void *);

typedef struct mtx mtx_t;
typedef struct cnd cnd_t;
typedef unsigned tss_t;

#if defined _MSC_VER
#include <windows.h>
typedef HANDLE thrd_t;
#else
#define __CPU_MASK_TYPE int
#include <pthread.h>
typedef pthread_t thrd_t;
// #elif defined KORA_KRN
// typedef unsigned thrd_t;
#endif


struct mtx {
    atomic_int value;
    thrd_t thread;
    int flags;
};

struct cnd {
    mtx_t *mtx;
    atomic_int seq;
    int flags;
};


/* Creates a thread */
int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
/* Checks if two identifiers refer to the same thread */
int thrd_equal(thrd_t lhs, thrd_t rhs);
/* Obtains the current thread identifier */
thrd_t thrd_current(void);
/* Suspends execution of the calling thread for the given period of time */
int thrd_sleep(const struct timespec *duration, struct timespec *remaining);
/* Yields the current time slice */
void thrd_yield(void);
/* Terminates the calling thread */
_Noreturn void thrd_exit(int res);
/* Detaches a thread */
int thrd_detach(thrd_t thr);
/* Blocks until a thread terminates */
int thrd_join(thrd_t thr, int *res);


/* Creates thread-specific storage pointer with a given destructor */
int tss_create(tss_t *tss_key, tss_dtor_t destructor);
/* Reads from thread-specific storage */
void *tss_get(tss_t tss_key);
/* Write to thread-specific storage */
int tss_set(tss_t tss_id, void *val);
/* Releases the resources held by a given thread-specific pointer */
void tss_delete(tss_t tss_id);


/* create a mutex */
int mtx_init(mtx_t *mutex, int type);
/* blocks until locks a mutex */
int mtx_lock(mtx_t *mutex);
/* blocks until locks a mutex or times out */
int mtx_timedlock(mtx_t *restrict mutex, const struct timespec *restrict time_point);
/* locks a mutex or returns without blocking if already locked */
int mtx_trylock(mtx_t *mutex);
/* unlock a mutex */
int mtx_unlock(mtx_t *mutex);
/* destroys a mutex */
void mtx_destroy(mtx_t *mutex);


/* create a condition variable */
int cnd_init(cnd_t *cond);
/* unblocks one thread blocked on a condition variable */
int cnd_signal(cnd_t *cond);
/* unblocks all threads blocked on a condition variable */
int cnd_broadcast(cnd_t *cond);
/* blocks on a condition variable */
int cnd_wait(cnd_t *cond, mtx_t *mutex);
/* blocks on a condition variable, with a timeout */
int cnd_timedwait(cnd_t *restrict cond, mtx_t *restrict mutex, const struct timespec *restrict time_point);
/* destroys a condition variable */
void cnd_destroy(cnd_t *cond);

#endif  /* _THREADS_H */
