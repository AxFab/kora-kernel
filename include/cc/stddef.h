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
#ifndef __STDDEF_H
#define __STDDEF_H 1


#ifndef __PTRDIFF_TYPE__
#define __PTRDIFF_TYPE__ long
#endif
#define _PTRDIFF_T
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#ifndef __SIZE_TYPE__
#define __SIZE_TYPE__ unsigned long
#endif
#define _SIZE_T
typedef __SIZE_TYPE__ size_t;

#ifndef __WCHAR_TYPE__
#define __WCHAR_TYPE__ int
#endif
#define _WCHAR_T
typedef __WCHAR_TYPE__ wchar_t;


#undef NULL
#ifndef __cplusplus
#define NULL ((void *)0)
#else
#define NULL 0
typedef decltype(nullptr) nullptr_t;
#endif

#if 0
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)
#else
#define offsetof(TYPE, MEMBER)  ((int)(&((TYPE*)0)->MEMBER))
#endif

#endif  /* __STDDEF_H */
