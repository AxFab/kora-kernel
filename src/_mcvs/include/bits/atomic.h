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
#ifndef __BITS_ATOMIC_H
#define __BITS_ATOMIC_H 1

static inline void __atomic_dec_4(atomic_int *ptr)
{
    (*ptr)--;
}


static inline void __atomic_inc_4(atomic_int *ptr)
{
    (*ptr)++;
}

static inline int __atomic_fetch_add_4(atomic_int *ref, int val, int mode)
{
    ((void)mode);
    int prev = *ref;
    *ref += val;
    return prev;
}


static inline int __atomic_fetch_sub_4(atomic_int *ref, int val, int mode)
{
    return __atomic_fetch_add_4(ref, -val, mode);
}


static inline int __atomic_exchange_4(atomic_int *ref, int val, int mode)
{
    ((void)mode);
    int prev = *ref;
    *ref = val;
    return prev;
}


static inline bool __atomic_compare_exchange_4(atomic_int *ref, int *ptr, int val, int mode)
{
    ((void)mode);
    int prev = *ref;
    int next = *ptr;
    *ptr = prev;
    if (prev != next)
        return false;
    *ref = val;
    return true;
}


#endif /* __BITS_ATOMIC_H */
