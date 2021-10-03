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
#ifndef __BITS_ATOMIC_H
#define __BITS_ATOMIC_H 1

typedef volatile int atomic_int;
typedef volatile void *atomic_ptr;
typedef volatile unsigned long long atomic_u64;

void __lock(atomic_int *lock);
void __unlock(atomic_int *lock);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static inline int atomic_cmpxchg(atomic_int *p, int t, int s)
{
    __asm__ volatile(
        "lock ; cmpxchg %3, %1"
        : "=a"(t), "=m"(*p) : "a"(t), "r"(s) : "memory");
    return t;
}

static inline int atomic_xchg(atomic_int *p, int v)
{
    __asm__ volatile(
        "xchg %0, %1"
        : "=r"(v), "=m"(*p) : "0"(v) : "memory");
    return v;
}

static inline int atomic_xadd(atomic_int *p, int v)
{
    __asm__ volatile(
        "lock ; xadd %0, %1"
        : "=r"(v), "=m"(*p) : "0"(v) : "memory");
    return v;
}

static inline void atomic_inc(atomic_int *p)
{
    __asm__ volatile(
        "lock ; incl %0"
        : "=m"(*p) : "m"(*p) : "memory");
}

static inline void atomic_dec(atomic_int *p)
{
    __asm__ volatile(
        "lock ; decl %0"
        : "=m"(*p) : "m"(*p) : "memory");
}

static inline void atomic_store(atomic_int *p, int x)
{
    __asm__ volatile(
        "mov %1, %0 ; lock ; orl $0,(%%esp)"
        : "=m"(*p) : "r"(x) : "memory");
}

static inline int atomic_load(atomic_int *p)
{
    return *p;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static inline void *atomic_ptr_cmpxchg(atomic_ptr *p, void *t, void *s)
{
    __asm__ volatile(
        "lock ; cmpxchg %3, %1"
        : "=a"(t), "=m"(*p) : "a"(t), "r"(s) : "memory");
    return t;
}

static inline void *atomic_ptr_xchg(atomic_ptr *p, void *v)
{
    __asm__ volatile(
        "xchg %0, %1"
        : "=r"(v), "=m"(*p) : "0"(v) : "memory");
    return v;
}

static inline void atomic_ptr_store(atomic_ptr *p, void *x)
{
    __asm__ volatile(
        "mov %1, %0 ; lock ; orl $0,(%%esp)"
        : "=m"(*p) : "r"(x) : "memory");
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static inline void atomic_barrier()
{
    __asm__ volatile("" : : : "memory");
}


static inline void atomic_break()
{
    __asm__ volatile("pause" : : : "memory");
}


static inline void atomic_crash()
{
    __asm__ volatile("hlt" : : : "memory");
}

#endif  /* __BITS_ATOMIC_H */
