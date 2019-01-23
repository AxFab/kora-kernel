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
#ifndef _STRING_H
#define _STRING_H 1

#include <stddef.h>


#define _TVOID void
#define _TCHAR char
#define _SFM(n)  mem ## n
#define _SFX(n)  str ## n
#include <cbuild/string.h>
#undef _TVOID
#undef _TCHAR
#undef _SFM
#undef _SFX


/* Gives an error message string. */
char *strerror_r(int errnum, char *buf, int len);
/* Gives an error message string. */
char *strerror(int errnum);


#endif  /* _STRING_H */
