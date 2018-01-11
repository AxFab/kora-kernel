/*
 *      This file is part of the SmokeOS project.
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
#ifndef _SKC_RWLOCK_H
#define _SKC_RWLOCK_H 1

#include <skc/atomic.h>
#include <skc/splock.h>
#undef BUSY
#define BUSY 1
#define RWLOCK_BASIC

#if defined RWLOCK_BASIC

  typedef struct rwlock rwlock_t;

  struct rwlock
  {
    splock_t lock;
    atomic_t readers;
  };

  __stinline void rwlock_init(rwlock_t *lock)
  {
    splock_init(&lock->lock);
    lock->readers = 0;
  }

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

  __stinline void rwlock_wrlock(rwlock_t *lock)
  {
    splock_lock(&lock->lock);
    while (lock->readers) {
      cpu_relax();
    }
  }

  __stinline void rwlock_rdunlock(rwlock_t *lock)
  {
    atomic_dec(&lock->readers);
  }

  __stinline void rwlock_wrunlock(rwlock_t *lock)
  {
    splock_unlock(&lock->lock);
  }


  __stinline int rwlock_rdtrylock(rwlock_t *lock)
  {
    atomic_inc(&lock->readers);
    if (!lock->lock) {
      return 0;
    }

    atomic_dec(&lock->readers);
    return BUSY;
  }

  __stinline int rwlock_wrtrylock(rwlock_t *lock)
  {
    if (lock->readers) {
      return BUSY;
    } else if (splock_trylock(&lock->lock)) {
      return BUSY;
    } else if (lock->readers) {
      splock_unlock(&lock->lock);
      return BUSY;
    }

    return 0;
  }

  __stinline int rwlock_upgrade(rwlock_t *lock)
  {
    if (splock_trylock(&lock->lock)) {
      return BUSY;
    }

    atomic_dec(&lock->readers);
    while (lock->readers) {
      cpu_relax();
    }

    return 0;
  }

  __stinline int rwlock_wrlocked(rwlock_t *lock)
  {
    return splock_locked(&lock->lock);
  }

#endif /* RWLOCK_ */

#endif /* _SKC_RWLOCK_H */
