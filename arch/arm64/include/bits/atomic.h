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
 */
#ifndef __BITS_ATOMIC_H
#define __BITS_ATOMIC_H 1

typedef volatile int atomic_int;
typedef volatile void *atomic_ptr;
typedef volatile unsigned long long atomic_u64;

void __lock(atomic_int *lock);
void __unlock(atomic_int *lock);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  32-bit integer operations
//  ARM64 uses load-exclusive / store-exclusive (ldaxr/stlxr) for atomics.
//  %w[x] forces the 32-bit (W-register) view of a register operand.
//  "+Q"(*p)  — read+write memory operand using base-register addressing,
//              required by ldxr/stxr family instructions.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Compare *p with t; if equal store s; return the old value. */
static inline int atomic_cmpxchg(atomic_int *p, int t, int s)
{
    int result, tmp;
    __asm__ volatile(
        "1: ldaxr  %w[result], %[mem]\n"
        "   cmp    %w[result], %w[expected]\n"
        "   b.ne   2f\n"
        "   stlxr  %w[tmp], %w[desired], %[mem]\n"
        "   cbnz   %w[tmp], 1b\n"
        "2:"
        : [result] "=&r"(result), [tmp] "=&r"(tmp), [mem] "+Q"(*p)
        : [expected] "r"(t), [desired] "r"(s)
        : "memory", "cc");
    return result;
}

/* Atomically store v into *p; return the old value. */
static inline int atomic_xchg(atomic_int *p, int v)
{
    int result, tmp;
    __asm__ volatile(
        "1: ldaxr  %w[result], %[mem]\n"
        "   stlxr  %w[tmp], %w[val], %[mem]\n"
        "   cbnz   %w[tmp], 1b\n"
        : [result] "=&r"(result), [tmp] "=&r"(tmp), [mem] "+Q"(*p)
        : [val] "r"(v)
        : "memory");
    return result;
}

/* Atomically add v to *p; return the old value. */
static inline int atomic_xadd(atomic_int *p, int v)
{
    int result, newval, tmp;
    __asm__ volatile(
        "1: ldaxr  %w[result], %[mem]\n"
        "   add    %w[newval], %w[result], %w[val]\n"
        "   stlxr  %w[tmp], %w[newval], %[mem]\n"
        "   cbnz   %w[tmp], 1b\n"
        : [result] "=&r"(result), [newval] "=&r"(newval),
          [tmp] "=&r"(tmp), [mem] "+Q"(*p)
        : [val] "r"(v)
        : "memory");
    return result;
}

static inline void atomic_inc(atomic_int *p)
{
    int val, tmp;
    __asm__ volatile(
        "1: ldxr   %w[val], %[mem]\n"
        "   add    %w[val], %w[val], #1\n"
        "   stlxr  %w[tmp], %w[val], %[mem]\n"
        "   cbnz   %w[tmp], 1b\n"
        : [val] "=&r"(val), [tmp] "=&r"(tmp), [mem] "+Q"(*p)
        :
        : "memory");
}

static inline void atomic_dec(atomic_int *p)
{
    int val, tmp;
    __asm__ volatile(
        "1: ldxr   %w[val], %[mem]\n"
        "   sub    %w[val], %w[val], #1\n"
        "   stlxr  %w[tmp], %w[val], %[mem]\n"
        "   cbnz   %w[tmp], 1b\n"
        : [val] "=&r"(val), [tmp] "=&r"(tmp), [mem] "+Q"(*p)
        :
        : "memory");
}

/* Store with release semantics (stlr). */
static inline void atomic_store(atomic_int *p, int x)
{
    __asm__ volatile(
        "stlr  %w[val], %[mem]"
        : [mem] "=Q"(*p)
        : [val] "r"(x)
        : "memory");
}

/* Load with acquire semantics (ldar). */
static inline int atomic_load(atomic_int *p)
{
    int result;
    __asm__ volatile(
        "ldar  %w[result], %[mem]"
        : [result] "=r"(result)
        : [mem] "Q"(*p)
        : "memory");
    return result;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Pointer-wide (64-bit) operations
//  Drop the %w prefix so clang selects X-registers automatically.
//  stlxr status is always a W-register, so %w[tmp] stays on that operand.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static inline void *atomic_ptr_cmpxchg(atomic_ptr *p, void *t, void *s)
{
    void *result;
    int tmp;
    __asm__ volatile(
        "1: ldaxr  %[result], %[mem]\n"
        "   cmp    %[result], %[expected]\n"
        "   b.ne   2f\n"
        "   stlxr  %w[tmp], %[desired], %[mem]\n"
        "   cbnz   %w[tmp], 1b\n"
        "2:"
        : [result] "=&r"(result), [tmp] "=&r"(tmp), [mem] "+Q"(*p)
        : [expected] "r"(t), [desired] "r"(s)
        : "memory", "cc");
    return result;
}

static inline void *atomic_ptr_xchg(atomic_ptr *p, void *v)
{
    void *result;
    int tmp;
    __asm__ volatile(
        "1: ldaxr  %[result], %[mem]\n"
        "   stlxr  %w[tmp], %[val], %[mem]\n"
        "   cbnz   %w[tmp], 1b\n"
        : [result] "=&r"(result), [tmp] "=&r"(tmp), [mem] "+Q"(*p)
        : [val] "r"(v)
        : "memory");
    return result;
}

static inline void atomic_ptr_store(atomic_ptr *p, void *x)
{
    __asm__ volatile(
        "stlr  %[val], %[mem]"
        : [mem] "=Q"(*p)
        : [val] "r"(x)
        : "memory");
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/* Full hardware memory barrier (data memory barrier, inner shareable). */
static inline void atomic_barrier()
{
    __asm__ volatile("dmb ish" : : : "memory");
}

/* Spin-wait hint — ARM64 equivalent of x86 pause. */
static inline void atomic_break()
{
    __asm__ volatile("yield" : : : "memory");
}

/* Halt / software breakpoint — ARM64 equivalent of x86 hlt. */
static inline void atomic_crash()
{
    __asm__ volatile("brk #0" : : : "memory");
}

#endif  /* __BITS_ATOMIC_H */
