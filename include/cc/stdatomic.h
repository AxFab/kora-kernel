/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#ifndef __STDATOMIC_H
#define __STDATOMIC_H 1

#include <stdbool.h>
#include <stdint.h>
#include <kora/mcrs.h>

#define SIZEOF(object) (char *)(&(object)+1) - (char *)(&(object))

typedef enum {
#ifdef __ATOMIC_RELAXED
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_consume = __ATOMIC_CONSUME,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST
#else
    memory_order_relaxed = 0,
    memory_order_consume = 1,
    memory_order_acquire = 2,
    memory_order_release = 3,
    memory_order_acq_rel = 4,
    memory_order_seq_cst = 5
#endif

} memory_order;

#ifndef _Atomic
#define _Atomic
#endif

typedef _Atomic _Bool atomic_bool;
typedef _Atomic char atomic_char;
typedef _Atomic signed char atomic_schar;
typedef _Atomic unsigned char atomic_uchar;
typedef _Atomic short atomic_short;
typedef _Atomic unsigned short atomic_ushort;
typedef _Atomic int atomic_int;
typedef _Atomic unsigned int atomic_uint;
typedef _Atomic long atomic_long;
typedef _Atomic unsigned long atomic_ulong;
typedef _Atomic long long atomic_llong;
typedef _Atomic unsigned long long atomic_ullong;
typedef _Atomic __CHAR16_TYPE__ atomic_char16_t;
typedef _Atomic __CHAR32_TYPE__ atomic_char32_t;
typedef _Atomic __WCHAR_TYPE__ atomic_wchar_t;
typedef _Atomic __INT_LEAST8_TYPE__ atomic_int8_t;
typedef _Atomic __UINT_LEAST8_TYPE__ atomic_uint8_t;
typedef _Atomic __INT_LEAST16_TYPE__ atomic_int16_t;
typedef _Atomic __UINT_LEAST16_TYPE__ atomic_uint16_t;
typedef _Atomic __INT_LEAST32_TYPE__ atomic_int32_t;
typedef _Atomic __UINT_LEAST32_TYPE__ atomic_uint32_t;
typedef _Atomic __INT_LEAST64_TYPE__ atomic_int64_t;
typedef _Atomic __UINT_LEAST64_TYPE__ atomic_uint64_t;
typedef _Atomic __INT_LEAST8_TYPE__ atomic_int_least8_t;
typedef _Atomic __UINT_LEAST8_TYPE__ atomic_uint_least8_t;
typedef _Atomic __INT_LEAST16_TYPE__ atomic_int_least16_t;
typedef _Atomic __UINT_LEAST16_TYPE__ atomic_uint_least16_t;
typedef _Atomic __INT_LEAST32_TYPE__ atomic_int_least32_t;
typedef _Atomic __UINT_LEAST32_TYPE__ atomic_uint_least32_t;
typedef _Atomic __INT_LEAST64_TYPE__ atomic_int_least64_t;
typedef _Atomic __UINT_LEAST64_TYPE__ atomic_uint_least64_t;
typedef _Atomic __INT_FAST8_TYPE__ atomic_int_fast8_t;
typedef _Atomic __UINT_FAST8_TYPE__ atomic_uint_fast8_t;
typedef _Atomic __INT_FAST16_TYPE__ atomic_int_fast16_t;
typedef _Atomic __UINT_FAST16_TYPE__ atomic_uint_fast16_t;
typedef _Atomic __INT_FAST32_TYPE__ atomic_int_fast32_t;
typedef _Atomic __UINT_FAST32_TYPE__ atomic_uint_fast32_t;
typedef _Atomic __INT_FAST64_TYPE__ atomic_int_fast64_t;
typedef _Atomic __UINT_FAST64_TYPE__ atomic_uint_fast64_t;
typedef _Atomic __INTPTR_TYPE__ atomic_intptr_t;
typedef _Atomic __UINTPTR_TYPE__ atomic_uintptr_t;
typedef _Atomic __SIZE_TYPE__ atomic_size_t;
typedef _Atomic __PTRDIFF_TYPE__ atomic_ptrdiff_t;
typedef _Atomic __INTMAX_TYPE__ atomic_intmax_t;
typedef _Atomic __UINTMAX_TYPE__ atomic_uintmax_t;


#define ATOMIC_VAR_INIT(VALUE)  (VALUE)

/* Initialize an atomic object pointed to by PTR with VAL.  */
#define atomic_init(PTR, VAL)  atomic_store_explicit (PTR, VAL, memory_order_relaxed)

#define kill_dependency(Y)          \
  __extension__                 \
  ({                        \
    __auto_type __kill_dependency_tmp = (Y);    \
    __kill_dependency_tmp;          \
  })

extern void atomic_thread_fence(memory_order);
#define atomic_thread_fence(MO) __atomic_thread_fence (MO)
extern void atomic_signal_fence(memory_order);
#define atomic_signal_fence(MO) __atomic_signal_fence  (MO)
#define atomic_is_lock_free(OBJ) __atomic_is_lock_free (sizeof (*(OBJ)), (OBJ))

#define ATOMIC_BOOL_LOCK_FREE       __GCC_ATOMIC_BOOL_LOCK_FREE
#define ATOMIC_CHAR_LOCK_FREE       __GCC_ATOMIC_CHAR_LOCK_FREE
#define ATOMIC_CHAR16_T_LOCK_FREE   __GCC_ATOMIC_CHAR16_T_LOCK_FREE
#define ATOMIC_CHAR32_T_LOCK_FREE   __GCC_ATOMIC_CHAR32_T_LOCK_FREE
#define ATOMIC_WCHAR_T_LOCK_FREE    __GCC_ATOMIC_WCHAR_T_LOCK_FREE
#define ATOMIC_SHORT_LOCK_FREE      __GCC_ATOMIC_SHORT_LOCK_FREE
#define ATOMIC_INT_LOCK_FREE        __GCC_ATOMIC_INT_LOCK_FREE
#define ATOMIC_LONG_LOCK_FREE       __GCC_ATOMIC_LONG_LOCK_FREE
#define ATOMIC_LLONG_LOCK_FREE      __GCC_ATOMIC_LLONG_LOCK_FREE
#define ATOMIC_POINTER_LOCK_FREE    __GCC_ATOMIC_POINTER_LOCK_FREE


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define atomic_store_explicit(PTR, VAL, MO)   do { (*PTR) = (VAL); } while(0)
#define atomic_load_explicit(PTR, MO) (PTR)

#define atomic_exchange_explicit(X, V, MO)   ({ \
  typeof(*(X)) _r;
typeof((X)) _v = V;
\
__atomic_exchange((X), &_v, _r, (MO));
\
*_r;
\
})

// #define atomic_compare_exchange_weak_explicit(X, E, V, MOS, MOF)        \
//   __atomic_compare_exchange((X), (E), &__atmp(*(X), (V)), 1, (MOS), (MOF))

// #define atomic_compare_exchange_strong_explicit(X, E, V, MOS, MOF)       \
//   __atomic_compare_exchange((X), (E), &__atmp(*(X), (V)), 0, (MOS), (MOF))


// #define atomic_exchange_explicit(PTR, VAL, MO)              \
//  {                                    \
//    __auto_type __atomic_exchange_ptr = (PTR);              \
//    __typeof__ (*__atomic_exchange_ptr) __atomic_exchange_val = (VAL);  \
//    __typeof__ (*__atomic_exchange_ptr) __atomic_exchange_tmp;      \
//    __atomic_exchange (__atomic_exchange_ptr, &__atomic_exchange_val,   \
//               &__atomic_exchange_tmp, (MO));           \
//    __atomic_exchange_tmp;                      \
//  }

// #define atomic_compare_exchange_strong_explicit(PTR, VAL, DES, SUC, FAIL) \
//  {                                    \
//    __auto_type __atomic_compare_exchange_ptr = (PTR);          \
//    __typeof__ (*__atomic_compare_exchange_ptr) __atomic_compare_exchange_tmp \
//      = (DES);                              \
//    __atomic_compare_exchange (__atomic_compare_exchange_ptr, (VAL),    \
//                   &__atomic_compare_exchange_tmp, 0,   \
//                   (SUC), (FAIL));              \
//  }

// #define atomic_compare_exchange_weak_explicit(PTR, VAL, DES, SUC, FAIL) \
//  {                                    \
//    __auto_type __atomic_compare_exchange_ptr = (PTR);          \
//    __typeof__ (*__atomic_compare_exchange_ptr) __atomic_compare_exchange_tmp \
//      = (DES);                              \
//    __atomic_compare_exchange (__atomic_compare_exchange_ptr, (VAL),    \
//                   &__atomic_compare_exchange_tmp, 1,   \
//                   (SUC), (FAIL));              \
//  }


#define atomic_store(PTR, VAL)  atomic_store_explicit (PTR, VAL, memory_order_seq_cst)
#define atomic_load(PTR)  atomic_load_explicit (PTR, __ATOMIC_SEQ_CST)

#define atomic_exchange(PTR, VAL)  \
    atomic_exchange_explicit(PTR, VAL, memory_order_seq_cst)
#define atomic_compare_exchange_strong(PTR, VAL, DES)  \
    atomic_compare_exchange_strong_explicit (PTR, VAL, DES, __ATOMIC_SEQ_CST, memory_order_seq_cst)
#define atomic_compare_exchange_weak(PTR, VAL, DES)  \
    atomic_compare_exchange_weak_explicit (PTR, VAL, DES, __ATOMIC_SEQ_CST, memory_order_seq_cst)

#define atomic_fetch_add(PTR, VAL) __atomic_fetch_add ((PTR), (VAL), memory_order_seq_cst)
#define atomic_fetch_add_explicit(PTR, VAL, MO)  __atomic_fetch_add ((PTR), (VAL), (MO))

#define atomic_fetch_sub(PTR, VAL) __atomic_fetch_sub ((PTR), (VAL), memory_order_seq_cst)
#define atomic_fetch_sub_explicit(PTR, VAL, MO)  __atomic_fetch_sub ((PTR), (VAL), (MO))

#define atomic_fetch_or(PTR, VAL) __atomic_fetch_or ((PTR), (VAL), memory_order_seq_cst)
#define atomic_fetch_or_explicit(PTR, VAL, MO)  __atomic_fetch_or ((PTR), (VAL), (MO))

#define atomic_fetch_xor(PTR, VAL) __atomic_fetch_xor ((PTR), (VAL), memory_order_seq_cst)
#define atomic_fetch_xor_explicit(PTR, VAL, MO)  __atomic_fetch_xor ((PTR), (VAL), (MO))

#define atomic_fetch_and(PTR, VAL) __atomic_fetch_and ((PTR), (VAL), memory_order_seq_cst)
#define atomic_fetch_and_explicit(PTR, VAL, MO)  __atomic_fetch_and ((PTR), (VAL), (MO))


typedef _Atomic struct {
#if __GCC_ATOMIC_TEST_AND_SET_TRUEVAL == 1
    _Bool __val;
#else
    unsigned char __val;
#endif
} atomic_flag;

#define ATOMIC_FLAG_INIT    { 0 }


extern _Bool atomic_flag_test_and_set(volatile atomic_flag *);
#define atomic_flag_test_and_set(PTR)  __atomic_test_and_set ((PTR), memory_order_seq_cst)
extern _Bool atomic_flag_test_and_set_explicit(volatile atomic_flag *, memory_order);
#define atomic_flag_test_and_set_explicit(PTR, MO) __atomic_test_and_set ((PTR), (MO))

extern void atomic_flag_clear(volatile atomic_flag *);
#define atomic_flag_clear(PTR)  __atomic_clear ((PTR), memory_order_seq_cst)
extern void atomic_flag_clear_explicit(volatile atomic_flag *, memory_order);
#define atomic_flag_clear_explicit(PTR, MO)   __atomic_clear ((PTR), (MO))

#define atomic_inc(PTR)   (++(*PTR))
#define atomic_dec(PTR)   (--(*PTR))

#endif  /* _STDATOMIC_H */
