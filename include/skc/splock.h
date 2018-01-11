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
 *      Spin-lock implementation.
 */
#ifndef _SKC_SPLOCK_H
#define _SKC_SPLOCK_H 1

#include <skc/atomic.h>
#undef BUSY
#define BUSY 1
#define SPLOCK_BASIC

#if defined SPLOCK_BASIC

  typedef atomic_t splock_t;

  __stinline void splock_init(splock_t *lock)
  {
    *lock = 0;
  }

  __stinline void splock_lock(splock_t *lock)
  {
    cpu_disable_irq();
    for (;;) {
      if (atomic_xchg(lock, BUSY) == 0) {
        return;
      }
      while (*lock != 0) cpu_relax();
    }
  }

  __stinline void splock_unlock(splock_t *lock)
  {
    cpu_barrier();
    *lock = 0;
    cpu_enable_irq();
  }

  __stinline int splock_trylock(splock_t *lock)
  {
    cpu_disable_irq();
    if (atomic_xchg(lock, BUSY) == 0) {
      return 0;
    }
    cpu_enable_irq();
    return BUSY;
  }

  __stinline int splock_locked(splock_t *lock)
  {
    cpu_barrier();
    return *lock != 0;
  }

#elif defined SPLOCK_TICKET

  typedef union splock splock_t;

  union splock {
    atomic32_t val;
    struct {
      atomic16_t ticket;
      atomic16_t users;
    };
  };

  __stinline void splock_init(splock_t *lock)
  {
    lock->val = 0;
  }

  __stinline void splock_lock(splock_t *lock)
  {
    cpu_disable_irq();
    unsigned short ticket = atomic16_xadd(&lock->users, 1);
    while (lock->ticket != ticket) {
      cpu_relax();
    }
  }

  __stinline void splock_unlock(splock_t *lock)
  {
    cpu_barrier();
    atomic16_inc(lock->ticket);
    cpu_enable_irq();
  }

  __stinline int splock_trylock(splock_t *lock)
  {
    cpu_disable_irq();
    unsigned short ticket = lock->users;
    unsigned cmp1 = ((unsigned)ticket << 16) + ticket;
    unsigned cmp2 = ((unsigned)(ticket + 1) << 16) + ticket;
    if (atomic32_cmpxchg(lock->val, cmp, cmp2)) {
      return 0;
    }
    cpu_enable_irq();
    return BUSY;
  }

  __stinline int splock_locked(splock_t *lock)
  {
    splock_t cpy = *lock;
    cpu_barrier();
    return cpy.ticket != cpy.users;
  }

#endif /* SPLOCK_ */

#endif /* _SKC_SPLOCK_H */
