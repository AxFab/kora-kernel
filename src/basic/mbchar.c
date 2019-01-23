/*
 *      This file is part of the KoraOs project.
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
 */
#include <stddef.h>
#if 0
typedef struct mbstate {
    int __count;
} mbstate_t;

const char *__locale_charset(void);
int __mbtowc(wchar_t *ref, const char *buf, size_t len, const char *charset, mbstate_t *state);


/* Determine number of bytes in next multibyte character */
int mblen_r(const char *s, size_t n, mbstate_t *state)
{
    int ret;
    ret = __mbtowc(NULL, s, n, __locale_charset(), state);

    if (ret < 0) {
        state->__count = 0;
        return -1;
    }

    return ret;
}

/* Determine number of bytes in next multibyte character */
int mblen(const char *s, size_t n)
{
    static mbstate_t __mbstate;
    return mblen_r(s, n, &__mbstate);
}



/* Convert a multibyte string to a wide-character string */
size_t mbstowcs(wchar_t *dest, const char *src, size_t n);
/* Convert a multibyte sequence to a wide character */
int mbtowc(wchar_t *pwc, const char *s, size_t n);
/* Convert a wide-character string to a multibyte string */
size_t wcstombs(char *dest, const wchar_t *src, size_t n);
/* Convert a wide character to a multibyte sequence */
int wctomb(char *s, wchar_t wc);

#endif
