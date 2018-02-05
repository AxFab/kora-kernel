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
 *
 *      Read-Write Lock implementation.
 */
#ifndef _KORA_RWLOCK_H
#define _KORA_RWLOCK_H 1

#include <kora/atomic.h>
#include <kora/splock.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#if defined RWLOCK_TICKET

typedef union rwlock rwlock_t;

union rwlock {
    atomic32_t val;
    struct {
        atomic16_t ticket;
        atomic16_t users;
    };
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#else  /* NAIVE IMPLEMENTATION */

typedef struct rwlock rwlock_t;

struct rwlock {
    splock_t lock;
    atomic_t readers;
};

/* Initialize or reset a read/write lock structure */
__stinline void rwlock_init(rwlock_t *lock)
{
    splock_init(&lock->lock);
    lock->readers = 0;
}

/* Block until the lock allow reading */
__stinline void rwlock_rdlock(rwlock_t *lock)
{
    for (;;) {
        atomic_inc(&lock->readers);
        if (!lock->lock) {
            return;
        }

        atomic_dec(&lock->readers);
        while (lock->lock) {
            cpu_relax();
        }
    }
}

/* Block until the lock can be hold for write operation */
__stinline void rwlock_wrlock(rwlock_t *lock)
{
    splock_lock(&lock->lock);
    while (lock->readers) {
        cpu_relax();
    }
}

/* Release a lock previously taken for reading */
__stinline void rwlock_rdunlock(rwlock_t *lock)
{
    atomic_dec(&lock->readers);
}

/* Release a lock previously taken for writing */
__stinline void rwlock_wrunlock(rwlock_t *lock)
{
    splock_unlock(&lock->lock);
}

/* Try to grab a lock for reading but without blocking. */
__stinline bool rwlock_rdtrylock(rwlock_t *lock)
{
    atomic_inc(&lock->readers);
    if (!lock->lock) {
        return true;
    }

    atomic_dec(&lock->readers);
    return false;
}

/* Try to grab a lock for writing but without blocking. */
__stinline bool rwlock_wrtrylock(rwlock_t *lock)
{
    if (lock->readers) {
        return false;
    } else if (!splock_trylock(&lock->lock)) {
        return false;
    } else if (lock->readers) {
        splock_unlock(&lock->lock);
        return false;
    }

    return true;
}

/* Transform a previously hold reading lock into a writing one. Might block. */
__stinline bool rwlock_upgrade(rwlock_t *lock)
{
    if (!splock_trylock(&lock->lock)) {
        return false;
    }

    atomic_dec(&lock->readers);
    while (lock->readers) {
        cpu_relax();
    }

    return true;
}

/* Return a boolean that check the lock is hold by someone for writing. */
__stinline bool rwlock_wrlocked(rwlock_t *lock)
{
    return splock_locked(&lock->lock);
}

#endif /* RWLOCK_ */

#endif /* _KORA_RWLOCK_H */
