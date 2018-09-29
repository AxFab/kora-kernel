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
 *      Standard definition, extended.
 */
#ifndef _KORA_STDDEF_H
#define _KORA_STDDEF_H 1

#include <stddef.h>
#include <bits/cdefs.h>

#ifndef NULL
#  ifdef cplusplus
#    define NULL 0
#  else
#    define NULL ((void*)0)
#  endif
#endif

#ifndef itemof
#  undef offsetof
#  define offsetof(t,m)   ((size_t)&(((t*)0)->m))
#  define itemof(p,t,m)   ((t*)itemof_((p), offsetof(t,m)))
#endif

static inline void *itemof_(void *ptr, int off)
{
    return ptr ? (char *)ptr - off : 0;
}


/* Add protection to not mess with standard symbols */
#ifdef KORA_STDC
# define _PRT(n) n
#else
# define _PRT(n) n##_p
#endif


#endif  /* _KORA_STDDEF_H */
