/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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

#define _Noreturn __declspec(noreturn)
#define PACK(decl) __pragma(pack(push,1)) decl __pragma(pack(pop))
#define __thread __declspec(thread)
#define unlikely(c) c
#define likely(c) c

#define __ARCH "x86"
#define __ILP32

#endif /* _BITS_CDEFS_H */
