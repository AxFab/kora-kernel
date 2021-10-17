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
#ifndef __STDARG_H
#define __STDARG_H 1


#if 0

typedef __builtin_va_list va_list;

# define va_start(v,l)   __builtin_va_start(v,l)
# define va_end(v)   __builtin_va_end(v)
# define va_arg(v,l) __builtin_va_arg(v,l)
# define va_copy(d,s)  __builtin_va_copy(d,s)

#else

typedef char *va_list;

# define va_start(v,l)   (v = (((char*)&(l)) + sizeof(l)))
# define va_end(v)   ((void)v)
# define va_arg(v,l)   (*((l*)((v += sizeof(l)) - sizeof(l))))
# define va_copy(d,s)   (d = (s))

#endif

#endif /* __STDARG_H */
