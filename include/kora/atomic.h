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
#ifndef __KORA_ATOMIC_H
#define __KORA_ATOMIC_H 1

#include <bits/cdefs.h>
#include <stdint.h>
#include <stdbool.h>

typedef int8_t atomic_i8;
typedef int16_t atomic_i16;
typedef int32_t atomic_i32;
typedef int64_t atomic_i64;
typedef void * atomic_ptr;

typedef atomic_i32 atomic_int;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void atomic_store(atomic_int *ptr, int value);
int atomic_load(atomic_int *ptr);

void atomic_dec(atomic_int *ptr);
void atomic_inc(atomic_int *ptr);

int atomic_fetch_add(atomic_int *ptr, int value);
int atomic_fetch_sub(atomic_int *ptr, int value);

int atomic_exchange_explicit(atomic_int *ptr, int value);
bool atomic_compare_exchange_explicit(atomic_int *ref, atomic_int *ptr, int value);


#define atomic_store(p,v) do { (*(p) = (v)); } while(0)
#define atomic_load(p)  (*(p))

#define atomic_inc(p)  __atomic_inc_4(p)
#define atomic_dec(p)  __atomic_dec_4(p)

#define atomic_fetch_add(p,v)  __atomic_fetch_add_4(p,v,0)
#define atomic_fetch_sub(p,v)  __atomic_fetch_sub_4(p,v,0)

#define atomic_exchange_explicit(r,v)  __atomic_exchange_4(r,v,0)
#define atomic_compare_exchange_explicit(r,p,v)  \
    __atomic_compare_exchange_4(r,p,v,false,0,0)

#define atomic_ptr_compare_exchange(r,p,v) \
    __atomic_compare_exchange_4((atomic_i32*)(r),p,(int32_t)(v),false,0,0)


#define atomic_exchange atomic_exchange_explicit
#define atomic_compare_exchange atomic_compare_exchange_explicit

#include <bits/atomic.h>

#endif  /* __KORA_ATOMIC_H */
