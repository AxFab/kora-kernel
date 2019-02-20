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
#include <threads.h>
#include <kora/splock.h>
#include <kora/std.h>
#include <kora/futex.h>
#include <time.h>
#include <errno.h>
#include <limits.h>


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Creates a mutex */
int mtx_init(mtx_t *mutex, int flags)
{
    // TODO - Use shm in case of mtx_shared
    struct _US_MUTEX *ptr = malloc(sizeof(struct _US_MUTEX));
    ptr->counter = 0;
    ptr->flags = flags;
    ptr->thread = 0;
    splock_init(&ptr->splock);
    *mutex = ptr;
    return 0;
}

/* Blocks locks a mutex */
int mtx_lock(mtx_t *mutex)
{
    struct timespec ts;
    ts.tv_sec = LONG_MAX;
    return mtx_timedlock(mutex, &ts);
}

/* Blocks until locks a mutex or times out */
int mtx_timedlock(mtx_t *mutex, const struct timespec *until)
{
    struct _US_MUTEX *ptr = *mutex;
    if (ptr->counter > 0 && (ptr->flags & mtx_recursive) && thrd_equal(ptr->thread, thrd_current()))
        return 0;

    if (atomic_xadd(&ptr->counter, 1) == 0) {
        ptr->thread = thrd_current();
        return 0;
    }

    if (futex_wait(ptr, until) == 0) {
        ptr->thread = thrd_current();
        return 0;
    }
    return -1;
}

/* Locks a mutex or returns without blocking if already locked */
int mtx_trylock(mtx_t *mutex)
{
    struct _US_MUTEX *ptr = *mutex;
    if (ptr->counter > 0 && (ptr->flags & mtx_recursive) && thrd_equal(ptr->thread, thrd_current()))
        return 0;

    if (atomic_cmpxchg(&ptr->counter, 0, 1) == 0) {
        ptr->thread = thrd_current();
        return 0;
    }

    return 1;
}

/* Unlocks a mutex */
int mtx_unlock(mtx_t *mutex)
{
    struct _US_MUTEX *ptr = *mutex;
    int prev = atomic_xchg(&ptr->counter, 0);
    if (prev > 1)
        futex_raise(ptr);
    return 0;
}

/* Destroys a mutex */
void mtx_destroy(mtx_t *mutex)
{
    struct _US_MUTEX *ptr = *mutex;
    free(ptr);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


/* Creates a condition variable */
int cnd_init(cnd_t *cond)
{
    mtx_init(cond, mtx_timed);
    mtx_lock(cond);
    return 0;
}

/* Unblocks one thread blocked on a condition variable */
int cnd_signal(cnd_t *cond)
{
    struct _US_MUTEX *ptr = *cond;
    ptr->flags |= mtx_cnd_single;
    return mtx_unlock(cond);
}

/* Unblocks all threads blocked on a condition variable */
int cnd_broadcast(cnd_t *cond)
{
    struct _US_MUTEX *ptr = *cond;
    ptr->flags &= ~mtx_cnd_single;
    return mtx_unlock(cond);
}

/* Blocks on a condition variable */
int cnd_wait(cnd_t *cond, mtx_t *mutex)
{
    struct _US_MUTEX *ptr = *cond;
    if (mutex != NULL)
        mtx_unlock(mutex);
    if (mtx_lock(cond) != 0)
        return -1;
    if ((ptr->flags & mtx_cnd_single) == 0)
        mtx_unlock(cond);
    if (mutex != NULL)
        return mtx_lock(mutex);
    return 0;
}

/* Blocks on a condition variable, with a timeout */
int cnd_timedwait(cnd_t *cond, mtx_t *mutex, const struct timespec *until)
{
    struct _US_MUTEX *ptr = *cond;
    if (mutex != NULL)
        mtx_unlock(mutex);
    if (mtx_timedlock(cond, until) != 0)
        return -1;
    if ((ptr->flags & mtx_cnd_single) == 0)
        mtx_unlock(cond);
    if (mutex != NULL)
        return mtx_timedlock(mutex, until);
    return 0;
}

/* Destroys a condition variable */
void cnd_destroy(cnd_t *cond)
{
    mtx_destroy(cond);
}
