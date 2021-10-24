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
#ifndef __BITS_CDEFS_H
#define __BITS_CDEFS_H 1

#ifndef _NO_STATIC_ASSERT
_Static_assert(sizeof(short) == 2, "Unsupported, short must be 16 bits");
_Static_assert(sizeof(int) == 4, "Unsupported, int must be 32 bits");
_Static_assert(sizeof(long long) == 8, "Unsupported, long long must be 64 bits");
#endif

/* Guess compiler and architecture */
#if defined __amd64 || defined __x86_64
# define __ARCH  "amd64"
# define BYTE_ORDER LITTLE_ENDIAN
# define WORDSIZE  64
# define LONG_BIT  64
# include <bits/cdefs/gcc.h>

#elif defined __arm__
# define __ARCH  "aarch64"
# define WORDSIZE 64
# include <bits/cdefs/gcc.h>

#elif defined __i386
# define __ARCH  "i386"
# define BYTE_ORDER LITTLE_ENDIAN
# define WORDSIZE 32
# define LONG_BIT 32
# include <bits/cdefs/gcc.h>

#elif defined __ia64__
# define __ARCH  "ia64"
# define __BYTE_ORDER __LITTLE_ENDIAN
# define WORDSIZE 64
# define LONG_BIT 64
# include <bits/cdefs/gcc.h>

#elif defined _M_ARM
# define __ARCH  "aarch64"
# define WORDSIZE 64
# define LONG_BIT 32
# include <bits/cdefs/mcvs.h>

#elif defined _M_IX86
# define __ARCH  "i386"
# define __BYTE_ORDER __LITTLE_ENDIAN
# define WORDSIZE 32
# define LONG_BIT 32
# include <bits/cdefs/mcvs.h>

#elif defined _M_IA64
# define __ARCH  "amd64"
# define __BYTE_ORDER __LITTLE_ENDIAN
# define WORDSIZE 64
# define LONG_BIT 32
# include <bits/cdefs/mcvs.h>

#else
# error Unknown compiler
#endif

/* Define environment specific macros */
#ifdef __cplusplus
# define __STDC_GUARD  extern "C" {
# define __STDC_END  }
#else
# define __STDC_GUARD
# define __STDC_END
#endif
#ifndef KORA_KRN
# undef __asm_irq_on_
# undef __asm_irq_off_
# define __asm_irq_on_  ((void)0)
# define __asm_irq_off_  ((void)0)
#endif
#ifndef __asm_pause_
# define  __asm_pause_  ((void)0)
#endif



/* Define specification level */
#if defined _GNU_SOURCE
# define __GNU
# define __UNIX
# define __POSIX2
# define __C11
#endif

#if defined __POSIX2
# define __POSIX  /* Enable IEEE Std 1003.1 extention. */
#endif


#if __STDC_VERSION__ >= 201112L
# define __C11  /* Ename ISO C11 extention */
#endif
#if __STDC_VERSION__ >= 199901L || defined __cplusplus || defined __C11
# define __C99  /* Ename ISO C99 extention */
#endif

#if __STDC_VERSION__ >= 199409L || defined __C99
# define __C95  /* Enable ISO C90 Amendment 1:1995 extention. */
#endif

#define __C89



#define PAGE_SIZE  4096

#endif  /* __BITS_CDEFS_H */
