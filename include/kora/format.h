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
 *      Self-Balanced Bianry tree implementation.
 */
#ifndef _KORA_FORMAT_H
#define _KORA_FORMAT_H 1

#include <kora/stddef.h>
#include <kora/iofile.h>
#include <stdarg.h>

/* Write formated string from a string */
int _PRT(vfprintf)(FILE *fp, const char *str, va_list ap);
int _PRT(sprintf)(char *str, const char *format, ...);
int _PRT(snprintf)(char *str, size_t lg, const char *format, ...);
int _PRT(vsprintf)(char *str, const char *format, va_list ap);
int _PRT(vsnprintf)(char *str, size_t lg, const char *format, va_list ap);

#endif  /* _KORA_FORMAT_H */
