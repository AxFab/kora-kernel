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
#if !defined __KORA_ATOMIC_H
# error "Never use <bits/atomic.h> directly; include <kora/atomic.h> instead."
#endif



static inline void __atomic_inc_1(atomic_i8 *ref, int mode)
{
    ((void)mode);
    asm volatile("lock incb %0" : "=m"(*ref));
}

static inline void __atomic_dec_1(atomic_i8 *ref, int mode)
{
    ((void)mode);
    asm volatile("lock decb %0" : "=m"(*ref));
}

static inline int8_t __atomic_fetch_add_1(atomic_i8 *ref, int8_t val, int mode)
{
    ((void)mode);
    asm volatile("lock xaddb %%al, %2;"
                 :"=a"(val) :"a"(val), "m"(*ref) :"memory");
    return val;
}

static inline int8_t __atomic_fetch_sub_1(atomic_i8 *ref, int8_t val, int mode)
{
    return __atomic_fetch_add_1(ref, -val, mode);
}

bool __atomic_exchange_1(atomic_i8 *ref, int8_t val, int mode);
bool __atomic_compare_exchange_1(atomic_i8 *ref, int8_t *ptr, int8_t val,
    bool weak, int mode_success, int mode_failure);
// {
//     ((void)weak);
//     ((void)mode_success);
//     ((void)mode_failure);
//     int prev = *ptr;
//     asm volatile("cmpxchgb  %1, %2;"
//                  :"=a"(val) :"r"(val), "m"(*ref), "a"(prev) :"memory");
//     *ptr = val;
//     return val == prev;
// }


/* ------------------------------------------------------------------------ */

static inline void __atomic_inc_2(atomic_i16 *ref, int mode)
{
    ((void)mode);
    asm volatile("lock incw %0" : "=m"(*ref));
}

static inline void __atomic_dec_2(atomic_i16 *ref, int mode)
{
    ((void)mode);
    asm volatile("lock decw %0" : "=m"(*ref));
}

static inline int16_t __atomic_fetch_add_2(atomic_i16 *ref, int16_t val, int mode)
{
    ((void)mode);
    asm volatile("lock xaddw %%ax, %2;"
                 :"=a"(val) :"a"(val), "m"(*ref) :"memory");
    return val;
}

static inline int16_t __atomic_fetch_sub_2(atomic_i16 *ref, int16_t val, int mode)
{
    return __atomic_fetch_add_2(ref, -val, mode);
}


int16_t __atomic_exchange_2(atomic_i16 *ref, int16_t val, int mode);
bool __atomic_compare_exchange_2(atomic_i16 *ref, int16_t *ptr, int16_t val,
    bool weak, int mode_success, int mode_failure);
// {
//     ((void)weak);
//     ((void)mode_success);
//     ((void)mode_failure);
//     int prev = *ptr;
//     asm volatile("cmpxchgq  %1, %2;"
//                  :"=a"(val) :"r"(val), "m"(*ref), "a"(prev) :"memory");
//     *ptr = val;
//     return val == prev;
// }

/* ------------------------------------------------------------------------ */

static inline void __atomic_inc_4(atomic_i32 *ref, int mode)
{
    ((void)mode);
    asm volatile("lock incl %0" : "=m"(*ref));
}

static inline void __atomic_dec_4(atomic_i32 *ref, int mode)
{
    ((void)mode);
    asm volatile("lock decl %0" : "=m"(*ref));
}

static inline int32_t __atomic_fetch_add_4(atomic_i32 *ref, int32_t val, int mode)
{
    ((void)mode);
    asm volatile("lock xaddl %%eax, %2;"
                 :"=a"(val) :"a"(val), "m"(*ref) :"memory");
    return val;
}

static inline int32_t __atomic_fetch_sub_4(atomic_i32 *ref, int32_t val, int mode)
{
    return __atomic_fetch_add_4(ref, -val, mode);
}

static inline int32_t __atomic_exchange_4(atomic_i32 *ref, int32_t val, int mode)
{
    ((void)mode);
    register int32_t store = val;
    asm volatile("lock xchg %1, %0" : "=m"(*ref), "=r"(store) : "1"(val));
    return store;
}

static inline bool __atomic_compare_exchange_4(atomic_i32 *ref, int32_t *ptr,
    int32_t val, bool weak, int mode_success, int mode_failure)
{
    ((void)weak);
    ((void)mode_success);
    ((void)mode_failure);
    register int32_t store = *ptr;
    asm volatile("cmpxchg %1, %2;"
                 :"=a"(val) :"r"(val), "m"(*ref), "a"(store) :"memory");
    *ptr = val;
    return val == store;
}

