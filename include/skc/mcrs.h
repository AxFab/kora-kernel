/*
 *      This file is part of the SmokeOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
 *      Define some useful macros
 */
#ifndef _SKC_MCRS_H
#define _SKC_MCRS_H 1

#define _Kib_ (1024L)
#define _Mib_ (1024L*_Kib_)
#define _Gib_ (1024LL*_Mib_)
#define _Tib_ (1024LL*_Gib_)
#define _Pib_ (1024LL*_Tib_)
#define _Eib_ (1024LL*_Pib_)

#define ALIGN_UP(v,a)      (((v)+(a-1))&(~(a-1)))
#define ALIGN_DW(v,a)      ((v)&(~(a-1)))

#define MIN(a,b)    ((a)<=(b)?(a):(b))
#define MAX(a,b)    ((a)>=(b)?(a):(b))
#define POW2(v)     ((v) != 0 && ((v) & ((v)-1)) == 0)

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define __AT__  __FILE__ ":" TOSTRING(__LINE__)


#define __FAIL(e,m) __perror_fail(e,__FILE__,__LINE__,m)
void __perror_fail(int, const char *, int, const char *);

#define POISON_PTR ((void*)0xAAAAAAAA)


#endif  /* _SKC_MCRS_H */
