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
#ifndef _BITS_CDEFS_H
#define _BITS_CDEFS_H 1

#define _Noreturn __attribute__((noreturn))
#define PACK(decl) decl __attribute__((__packed__))
#define unlikely(c) c
#define likely(c) c

#define PAGE_SIZE 4096
#define __ARCH "arm"
#define __ILP32
#define __ILPx

#if defined KORA_KRN || defined UM_KRN
extern void irq_reset(bool enable);
extern bool irq_enable();
extern void irq_disable();
#define RELAX ((void)0)
#define BARRIER ((void)0)
#define THROW_ON irq_enable()
#define THROW_OFF irq_disable()
#else
#define RELAX ((void)0)
#define BARRIER ((void)0)
#define THROW_ON ((void)0)
#define THROW_OFF ((void)0)
#endif

#endif /* _BITS_CDEFS_H */
