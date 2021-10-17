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

#define _Noreturn __attribute__((noreturn))
#define PACK(decl) decl __attribute__((__packed__))
#define thread_local __thread
#define unlikely(c) c
#define likely(c) c

#define PAGE_SIZE  4096
#define WORDSIZE 32
#define __ARCH  "i386"
#define __ILP32
#define __ILPx

#define RELAX asm volatile("pause")
#define BARRIER asm volatile("")
#if defined KORA_KRN
#  define THROW_ON irq_enable()
#  define THROW_OFF irq_disable()
#else
#  define THROW_ON ((void)0)
#  define THROW_OFF ((void)0)
#endif


#endif /* __BITS_CDEFS_H */
