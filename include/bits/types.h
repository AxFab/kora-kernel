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
#ifndef __BITS_TYPES_H
#define __BITS_TYPES_H 1


#include <bits/cdefs.h>
// #include <bits/typesizes.h>

typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;

/* Datatypes integer size */
#if defined __ILP32 || defined __LLP64

typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
typedef signed long long int __int64_t;
typedef unsigned long long int __uint64_t;
typedef long long int __quad_t;
typedef unsigned long long int __u_quad_t;

#elif defined __LP64

typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;
typedef long int __quad_t;
typedef unsigned long int __u_quad_t;

#endif  /* Datatypes integer size */



typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef int __daddr_t;
typedef int __key_t;
typedef int __clockid_t;
typedef void * __timer_t;
typedef long int __blksize_t;
typedef long int __blkcnt_t;
typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsfilcnt_t;
typedef char *__caddr_t;
typedef unsigned int __socklen_t;
typedef __quad_t *__qaddr_t;


/* Datatypes pointer size */
#if defined __ILP32

typedef long int __fsword_t;
typedef long int __syscall_slong_t;
typedef unsigned long int __syscall_ulong_t;
typedef int __intptr_t;
typedef long int __ssize_t;

#elif defined __LP64

typedef long int __fsword_t;
typedef long int __syscall_slong_t;
typedef unsigned long int __syscall_ulong_t;
typedef long int __intptr_t;
typedef long int __ssize_t;

#endif  /* Datatypes pointer size */




/* Datatypes 64 */
#if defined __LP64

typedef unsigned long int __ino64_t;
typedef long int __off64_t;
typedef unsigned long int __rlim64_t;
typedef long int __blkcnt64_t;
typedef unsigned long int __fsblkcnt64_t;
typedef unsigned long int __fsfilcnt64_t;
typedef __off64_t __loff_t;

#else

typedef unsigned long long int __ino64_t;
typedef long long int __off64_t;
typedef unsigned long long int __rlim64_t;
typedef long long int __blkcnt64_t;
typedef unsigned long long int __fsblkcnt64_t;
typedef unsigned long long int __fsfilcnt64_t;
typedef __off64_t __loff_t;

#endif  /* Datatypes 64 */


#endif  /* __BITS_TYPES_H */
