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
#ifndef _KORA_SPLOCK_H
#define _KORA_SPLOCK_H 1

#include <bits/atomic.h>
#include <stdbool.h>

#ifdef KORA_KRN
extern void irq_reset(bool enable);
extern bool irq_enable();
extern void irq_disable();
#define THROW_ON irq_enable()
#define THROW_OFF irq_disable()
#else
#define THROW_ON ((void)0)
#define THROW_OFF ((void)0)
#endif

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#if defined SPLOCK_TICKET

typedef union splock splock_t;

union splock {
    atomic32_t val;
    struct {
        atomic16_t ticket;
        atomic16_t users;
    };
};

#define INIT_SPLOCK  { 0 }

/* Initialize and reset a spin-lock structure */
static inline void splock_init(splock_t *lock)
{
    lock->val = 0;
}

/* Block the cpu until the lock might be taken */
static inline void splock_lock(splock_t *lock)
{
    THROW_OFF;
    uint16_t ticket = atomic16_xadd(&lock->users, 1);
    while (lock->ticket != ticket)
        cpu_relax();
}

/* Release a lock */
static inline void splock_unlock(splock_t *lock)
{
    cpu_barrier();
    atomic16_inc(&lock->ticket);
    THROW_ON;
}

/* Try to grab the lock but without blocking. */
static inline bool splock_trylock(splock_t *lock)
{
    THROW_OFF;
    uint16_t ticket = lock->users;
    unsigned cmp1 = ((unsigned)ticket << 16) + ticket;
    unsigned cmp2 = ((unsigned)(ticket + 1) << 16) + ticket;
    if (atomic32_cmpxchg(&lock->val, cmp1, cmp2))
        return true;
    THROW_ON;
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

typedef atomic32_t splock_t;

#define INIT_SPLOCK  0

/* Initialize and reset a spin-lock structure */
static inline void splock_init(splock_t *lock)
{
    *lock = 0;
}

/* Block the cpu until the lock might be taken */
static inline void splock_lock(splock_t *lock)
{
    THROW_OFF;
    for (;;) {
        if (atomic_xchg(lock, 1) == 0)
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
    THROW_ON;
}

/* Try to grab the lock but without blocking. */
static inline bool splock_trylock(splock_t *lock)
{
    THROW_OFF;
    if (atomic_xchg(lock, 1) == 0)
        return true;
    THROW_ON;
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
