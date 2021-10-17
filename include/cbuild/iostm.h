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
#ifndef _XIO
# define _XIOs(n) n
# define _XIOi(s,p) s ## p
#else
# define _XIOs(n) n ## _XIO
# define _XIOi(s,p) s ## _XIO p
#endif

/* Write formated string on standard output */
int _XIOs(printf)(const _XCHAR *format, ...);
/* Write formated string on an opened file */
int _XIOi(f, printf)(FILE *fp, const _XCHAR *format, ...);
/* Write formated string from a string */
int _XIOi(s, printf)(_XCHAR *str, const _XCHAR *format, ...);
/* Write formated string on standard output */
int _XIOi(v, printf)(const _XCHAR *format, va_list ap);
/* Write formated string on an opened file */
int _XIOi(vf, printf)(FILE *fp, const _XCHAR *str, va_list ap);
/* Write formated string from a string */
int _XIOi(vs, printf)(_XCHAR *str, const _XCHAR *format, va_list ap);


/* Read and parse standard input */
int _XIOs(scanf)(const _XCHAR *format, ...);
/* Read and parse an open file */
int _XIOi(f, scanf)(FILE *f, const _XCHAR *format, ...);
/* Read and parse a string */
int _XIOi(s, scanf)(const _XCHAR *str, const _XCHAR *format, ...);


/* Read a character from STREAM. */
int _XIOi(get, c)(FILE *stream);
/* Read a character from STREAM. */
int _XIOi(fget, c)(FILE *stream);
/* Get a newline-terminated string of finite length from STREAM. */
_XCHAR *_XIOi(fget, s)(_XCHAR *s, int n, FILE *stream);
/* Write a character to STREAM. */
int _XIOi(put, c)(int c, FILE *stream);
/* Write a character to STREAM. */
int _XIOi(fput, c)(int c, FILE *stream);
/* Write a string to STREAM. */
int _XIOi(fput, s)(const _XCHAR *s, FILE *stream);


/* Read a character from stdin. */
int _XIOi(get, char)(void);
/* Write a character to stdout. */
int _XIOi(put, char)(int c);
/* Push a character back onto the input buffer of STREAM. */
int _XIOi(unget, c)(int c, FILE *stream);
