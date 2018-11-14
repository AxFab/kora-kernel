/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <bits/atomic.h>
#include <time.h>

#undef __STDC_NO_THREADS__

#define thread_local __tls

typedef int(*thrd_start_t)(void *);
typedef void(*tss_dtor_t)(void *);

typedef struct _US_THREAD *thrd_t;
typedef struct _US_MUTEX *mtx_t;
typedef struct _US_MUTEX *cnd_t;
typedef tss_dtor_t *tss_t;


typedef atomic_t once_flag;
#define ONCE_FLAG_INIT  0

/* Indicates a thread error status */
enum {
    thrd_success = 0,
    thrd_busy = 1,
    thrd_timedout,
    thrd_nomem,
    thrd_error = -1,
};

/* Defines the type of a mutex */
enum {
    mtx_plain = 0,
    mtx_recursive = 1,
    mtx_timed = 2,
    mtx_cnd_single = 4,
};

/* Threads -=-=-=-=-=-=-=-=-=-=-=-= */
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


/* Mutual exclusion -=-=-=-=-=-=-=-=-=-=-=-= */

/* Creates a mutex */
int mtx_init(mtx_t *mutex, int type);
/* Blocks locks a mutex */
int mtx_lock(mtx_t *mutex);
/* Blocks until locks a mutex or times out */
int mtx_timedlock(mtx_t *mutex, const struct timespec *time_point);
/* Locks a mutex or returns without blocking if already locked */
int mtx_trylock(mtx_t *mutex);
/* Unlocks a mutex */
int mtx_unlock(mtx_t *mutex);
/* Destroys a mutex */
void mtx_destroy(mtx_t *mutex);


/* Calls a function exactly once */
void call_once(once_flag *flag, void (*func)(void));


/* Condition variables -=-=-=-=-=-=-=-=-=-=-=-= */

/* Creates a condition variable */
int cnd_init(cnd_t *cond);
/* Unblocks one thread blocked on a condition variable */
int cnd_signal(cnd_t *cond);
/* Unblocks all threads blocked on a condition variable */
int cnd_broadcast(cnd_t *cond);
/* Blocks on a condition variable */
int cnd_wait(cnd_t *cond, mtx_t *mutex);
/* Blocks on a condition variable, with a timeout */
int cnd_timedwait(cnd_t *cond, mtx_t *mutex, const struct timespec *time_point);
/* Destroys a condition variable */
void cnd_destroy(cnd_t *cond);

/* Thread-local storage -=-=-=-=-=-=-=-=-=-=-=-= */

/* Maximum number of times destructors are called */
#define TSS_DTOR_ITERATIONS  5

/* Creates thread-specific storage pointer with a given destructor */
int tss_create(tss_t *tss_key, tss_dtor_t destructor);
/* Reads from thread-specific storage */
void *tss_get(tss_t tss_key);
/* Write to thread-specific storage */
int tss_set(tss_t tss_id, void *val);
/* Releases the resources held by a given thread-specific pointer */
void tss_delete(tss_t tss_id);

#endif  /* _THREADS_H */
