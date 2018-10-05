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
 *
 *      Standard definition.
 */
#ifndef _STDDEF_H
#define _STDDEF_H 1

#ifndef NULL
#  ifdef cplusplus
#    define NULL 0
#  else
#    define NULL ((void*)0)
#  endif
#endif

#define offsetof(t,m)   ((size_t)&(((t*)0)->m))

typedef unsigned long size_t;
typedef int wchar_t;

#endif /* _STDDEF_H */
