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
#include <stdatomic.h>

#define __asm__ asm volatile

static inline cpu_barrier(void)
{
    __asm__("" : : : "memory");
}

static inline cpu_mbrd(void)
{
    // __asm__ ("dmb" : : : "memory");
    // __asm__ ("mcr p15, 0, %0, c7, c10, 5", : : : "memory");
    __asm__("" : : : "memory");
}

static inline cpu_mbwr(void)
{
    // __asm__ ("dmb st" : : : "memory");
    __asm__("" : : : "memory");
}

int __atomic_load_4(atomic_int *ptr, int mode)
{
    if (mode == memory_order_release || mode == memory_order_acq_rel)
        cpu_mbrd();
    int value = *ptr;
    if (mode == memory_order_acquire || mode == memory_order_acq_rel)
        cpu_mbrd();
    return value;
}

void __atomic_store_4(atomic_int *ptr, int value, int mode)
{
    if (mode == memory_order_release || mode == memory_order_acq_rel)
        cpu_mbrd();
    *ptr = value;
    if (mode == memory_order_acquire || mode == memory_order_acq_rel)
        cpu_mbrd();
}

int __atomic_compare_exchange_4(atomic_int *ptr, int *old_value, int new_value, int mode)
{
    int prev, status;
    cpu_mbrd();
    do {
        __asm__("ldrex %0, [%3]\n"
                "mov %1, #0\n"
                "teq %0, %4\n"
                "strexeq %1, %5, [%3]"
                : "=&r"(prev), "=&r"(status), "+m"(*ptr)
                : "r"(ptr), "Ir"(old_value), "r"(new_value)
                : "cc");
    } while (__builtin_expect(status != 0, 0));
    if (mode == memory_order_acquire || mode == memory_order_acq_rel)
        cpu_mbrd();
    return prev != old_value;
}

int __atomic_exchange_4(atomic_int *ptr, int value, int mode)
{
    int prev, tmp, status;
    cpu_mbrd();
    do {
        __asm__("ldrex %0, [%4]\n"
                "mov %1, %5\n"
                "strexeq %2, %1, [%4]"
                : "=&r"(prev), "=&r"(tmp), "=&r"(status), "+m"(*ptr)
                : "r"(ptr), "Ir"(value)
                : "cc");
    } while (__builtin_expect(status != 0, 0));
    if (mode == memory_order_acquire || mode == memory_order_acq_rel)
        cpu_mbrd();
    return prev;
}

int __atomic_fetch_add_4(atomic_int *ptr, int value, int mode)
{
    int prev, tmp, status;
    cpu_mbrd();
    do {
        __asm__("ldrex %0, [%4]\n"
                "add %1, %0, %5\n"
                "strexeq %2, %1, [%4]"
                : "=&r"(prev), "=&r"(tmp), "=&r"(status), "+m"(*ptr)
                : "r"(ptr), "Ir"(value)
                : "cc");
    } while (__builtin_expect(status != 0, 0));
    if (mode == memory_order_acquire || mode == memory_order_acq_rel)
        cpu_mbrd();
    return prev;
}
