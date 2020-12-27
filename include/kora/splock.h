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

#include <bits/cdefs.h>
#include <kora/atomic.h>
#include <stdbool.h>

extern void irq_reset(bool enable);
extern bool irq_enable();
extern void irq_disable();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


typedef atomic_int splock_t;

#define INIT_SPLOCK  0

/* Initialize and reset a spin-lock structure */
static inline void splock_init(splock_t *lock)
{
    atomic_store(lock, 0);
}

/* Block the cpu until the lock might be taken */
static inline void splock_lock(splock_t *lock)
{
    irq_disable();
    for (;;) {
        if (atomic_exchange(lock, 1) == 0)
            return;
        while (*lock != 0)
            __asm_pause_;
    }
}

/* Release a lock */
static inline void splock_unlock(splock_t *lock)
{
    atomic_store(lock, 0);
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
    return atomic_load(lock) != 0;
}


#endif /* _KORA_SPLOCK_H */
