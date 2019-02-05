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
#ifndef _KORA_STRLIB_H
#define _KORA_STRLIB_H 1

#include <kora/stddef.h>

#define __long long
#define __ulong unsigned __long


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

__ulong _strtox(const char *str, char **endptr, int base, char *sign);

/* Convert ASCII string to long integer */
long _PRT(strtol)(const char *nptr, char **endptr, int base);

/* Convert ASCII string to unsigned long integer */
unsigned long _PRT(strtol)(const char *nptr, char **endptr, int base);

/* Convert a string to an integer */
int _PRT(atoi)(const char *nptr);

/* Convert a string to an integer */
long _PRT(atol)(const char *nptr);

#ifdef __USE_C99
/* Convert a string to an integer */
long long _PRT(atoll)(const char *nptr);

/* Convert a string to an integer */
long long _PRT(atoq)(const char *nptr);

long long _PRT(strtoll)(const char *nptr, char **endptr, int base);

unsigned long long _PRT(strtoull)(const char *nptr, char **endptr, int base);

#endif

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

/* Convert ASCII string to floating-point number */
double _PRT(strtod)(const char *nptr, char **endptr);

/* Convert a string to a double */
double _PRT(atof)(const char *nptr);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

char *_utoa(__ulong value, char *str, int base, const char *digits);

char *_PRT(itoa)(int value, char *str, int base);

#endif /* _KORA_STRLIB_H */
