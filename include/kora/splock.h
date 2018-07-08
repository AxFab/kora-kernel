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
 *      Spin-lock implementation.
 */
#ifndef _KORA_SPLOCK_H
#define _KORA_SPLOCK_H 1

#include <stdatomic.h>
#include <stdbool.h>
extern bool irq_enable();
extern void irq_disable();

#define cpu_relax() asm("pause")
#define cpu_barrier() asm("")


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#if defined SPLOCK_TICKET

typedef union splock splock_t;

union splock {
    atomic_char32_t val;
    struct {
        atomic_char16_t ticket;
        atomic_char16_t users;
    };
};

/* Initialize and reset a spin-lock structure */
static inline void splock_init(splock_t *lock)
{
    lock->val = 0;
}

/* Block the cpu until the lock might be taken */
static inline void splock_lock(splock_t *lock)
{
    irq_disable();
    unsigned short ticket = atomic16_xadd(&lock->users, 1);
    while (lock->ticket != ticket)
        cpu_relax();
}

/* Release a lock */
static inline void splock_unlock(splock_t *lock)
{
    cpu_barrier();
    atomic16_inc(lock->ticket);
    irq_enable();
}

/* Try to grab the lock but without blocking. */
static inline bool splock_trylock(splock_t *lock)
{
    irq_disable();
    unsigned short ticket = lock->users;
    unsigned cmp1 = ((unsigned)ticket << 16) + ticket;
    unsigned cmp2 = ((unsigned)(ticket + 1) << 16) + ticket;
    if (atomic32_cmpxchg(lock->val, cmp1, cmp2))
        return true;
    irq_enable();
    return false;
}

/* Return a boolean that check the lock is hold by someone. */
static inline bool splock_locked(splock_t *lock)
{
    splock_t cpy = *lock;
    cpu_barrier();
    return cpy.ticket != cpy.users;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#else  /* NAIVE IMPLEMENTATION */

typedef atomic_uint splock_t;

/* Initialize and reset a spin-lock structure */
static inline void splock_init(splock_t *lock)
{
    *lock = 0;
}

/* Block the cpu until the lock might be taken */
static inline void splock_lock(splock_t *lock)
{
    irq_disable();
    for (;;) {
        if (atomic_exchange(lock, 1) == 0)
            return;
        while (*lock != 0)
            cpu_relax();
    }
}

/* Release a lock */
static inline void splock_unlock(splock_t *lock)
{
    cpu_barrier();
    *lock = 0;
    irq_enable();
}

/* Try to grab the lock but without blocking. */
static inline bool splock_trylock(splock_t *lock)
{
    irq_disable();
    if (atomic_exchange(lock, 1) == 0)
        return true;
    irq_enable();
    return false;
}

/* Return a boolean that check the lock is hold by someone. */
static inline bool splock_locked(splock_t *lock)
{
    cpu_barrier();
    return *lock != 0;
}

#endif /* SPLOCK_ */

#endif /* _KORA_SPLOCK_H */
