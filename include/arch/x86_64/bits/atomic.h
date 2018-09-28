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
 *      Atomic operation.
 */
#ifndef _KORA_ATOMIC_H
#define _KORA_ATOMIC_H 1

#include <stdint.h>
#include <stdbool.h>

static inline void cpu_relax()
{
    asm volatile("pause");
}

static inline void cpu_barrier()
{
    asm volatile("");
}

void irq_reset(bool enable);
bool irq_enable();
void irq_disable();


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef volatile uint16_t atomic16_t;

static inline void atomic16_inc(atomic16_t *ptr)
{
    asm volatile("lock incw %0" : "=m"(*ptr));
}

static inline void atomic16_dec(atomic16_t *ptr)
{
    asm volatile("lock decw %0" : "=m"(*ptr));
}

static inline uint16_t atomic16_xchg(atomic16_t *ptr, uint16_t value)
{
    register atomic16_t ref = value;
    asm volatile("lock xchg %1, %0" : "=m"(*ptr), "=r"(ref) : "1"(value));
    return ref;
}

static inline uint16_t atomic16_xadd(atomic16_t *ptr, uint16_t value)
{
    asm volatile("lock xaddw %%ax, %2;"
                 :"=a"(value) :"a"(value), "m"(*ptr) :"memory");
    return value;
}

static inline uint16_t atomic16_cmpxchg(atomic16_t *ptr, uint16_t reference,
                                        uint16_t value)
{
    asm volatile("lock cmpxchg %%ax, %2;"
                 :"=a"(value) :"a"(value), "m"(*ptr) :"memory");
    return value;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef volatile uint32_t atomic32_t;

static inline void atomic32_inc(atomic32_t *ptr)
{
    asm volatile("lock incl %0" : "=m"(*ptr));
}

static inline void atomic32_dec(atomic32_t *ptr)
{
    asm volatile("lock decl %0" : "=m"(*ptr));
}

static inline uint32_t atomic32_xchg(atomic32_t *ptr, uint32_t value)
{
    register atomic32_t ref = value;
    asm volatile("lock xchg %1, %0" : "=m"(*ptr), "=r"(ref) : "1"(value));
    return ref;
}

static inline uint32_t atomic32_xadd(atomic32_t *ptr, uint32_t value)
{
    asm volatile("lock xaddl %%eax, %2;"
                 :"=a"(value) :"a"(value), "m"(*ptr) :"memory");
    return value;
}

static inline
uint32_t atomic32_cmpxchg(atomic32_t *ptr, uint32_t reference,
                          uint32_t value)
{
    asm volatile("lock cmpxchg %%eax, %2;"
                 :"=a"(value) :"a"(value), "m"(*ptr) :"memory");
    return value;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef volatile uint64_t atomic64_t;

static inline void atomic64_inc(atomic64_t *ptr)
{
    asm volatile("lock incq %0" : "=m"(*ptr));
}

static inline void atomic64_dec(atomic64_t *ptr)
{
    asm volatile("lock decq %0" : "=m"(*ptr));
}

static inline uint64_t atomic64_xchg(atomic64_t *ptr, uint64_t value)
{
    register atomic64_t ref = value;
    asm volatile("lock xchg %1, %0" : "=m"(*ptr), "=r"(ref) : "1"(value));
    return ref;
}

static inline uint64_t atomic64_xadd(atomic64_t *ptr, uint64_t value)
{
    asm volatile("lock xaddq %%rax, %2;"
                 :"=a"(value) :"a"(value), "m"(*ptr) :"memory");
    return value;
}

static inline
uint64_t atomic64_cmpxchg(atomic64_t *ptr, uint64_t reference,
                          uint64_t value)
{
    asm volatile("lock cmpxchg %%rax, %2;"
                 :"=a"(value) :"a"(value), "m"(*ptr) :"memory");
    return value;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define atomic_t atomic32_t
#define atomic_inc atomic32_inc
#define atomic_dec atomic32_dec
#define atomic_xchg atomic32_xchg
#define atomic_xadd atomic32_xadd
#define atomic_cmpxchg atomic32_cmpxchg

#endif /* _KORA_ATOMIC_H */
