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
#ifndef _ASSERT_H
#define _ASSERT_H 1

#include <bits/cdefs.h>

#if 0
#define assert(n) ((void)(n))
#else
void __assert_fail(const char *expr, const char *file, int line);
#define assert(n) do { if (!(n)) __assert_fail(#n,__FILE__,__LINE__); } while(0)
#endif

#endif /* _ASSERT_H */
