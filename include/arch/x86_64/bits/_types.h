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
#ifndef _BITS_TYPES_H
#define _BITS_TYPES_H 1

/* We define types and limit for x86 architecture */
typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef signed short int16_t;
typedef unsigned short uint16_t;

typedef signed int int32_t;
typedef unsigned int uint32_t;

typedef signed long int64_t;
typedef unsigned long uint64_t;

typedef signed long long intmax_t;
typedef unsigned long long uintmax_t;


typedef signed long ssize_t;
typedef unsigned long size_t;


typedef char bool;
#define false (0)
#define true (!0)



typedef unsigned long off_t;
typedef unsigned long id_t;
typedef id_t uid_t;
typedef id_t gid_t;
typedef id_t pid_t;


typedef long int time_t;
typedef long int clock_t;

/* Structure for a time value.  */
struct timespec
{
  time_t tv_sec;    /* Seconds.  */
  long int tv_nsec;   /* Nanoseconds.  */
};

// STDINT
// STDBOOL
// STDDEF
// STDARG
// #include <stdint.h>
// #include <stdbool.h>

// #include <sys/types.h>
// // typedef unsigned long long off_t;


// #if _ADD_STDC_MISS
// struct timespec {
//     long tv_sec;
//     long tv_nsec;
// };
// typedef unsigned long id_t;
// typedef  id_t uid_t;
// typedef  id_t gid_t;
// #endif


#endif /* _BITS_TYPES_H */
