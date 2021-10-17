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
#define _Noreturn __attribute__((noreturn))
#define PACK(decl) decl __attribute__((packed))
#define thread_local __thread
#define unlikely(c)  c
#define likely(c)  c

#if WORDSIZE == 32
#    define __ILP32
#    define __ILPx
#else
#    define __LP64
#    define __LPx
#endif

#define __asm_irq_on_  asm("sti")
#define __asm_irq_off_  asm("cli")
#define  __asm_pause_  asm("pause")

// Memory barriers
#define __asm_rmb       asm("")
#define __asm_wmb       asm("")
#define __asm_mb       asm("")
