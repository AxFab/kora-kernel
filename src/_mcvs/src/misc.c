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
#include <kernel/stdc.h>
#include <time.h>


/* Searches the first len bytes of array str for character c. */
void *memrchr(const void *str, int c, size_t len)
{
    register const char *ptr0 = ((const char *)str) + len;

    while (len > 0 && (*ptr0 != (char)c)) {
        --ptr0;
        --len;
    }

    return (void *)(len ? ptr0 : 0);
}

/* Scans s1 for the first token not contained in s2. */
char *strtok_r(register char *s, const char *delim, char **lasts)
{
    int skip_leading_delim = 1;
    register char *spanp;
    register int c, sc;
    char *tok;

    if (s == NULL && (s = *lasts) == NULL)
        return NULL;

    /*
     * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
     */
cont:
    c = *s++;

    for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
        if (c == sc) {
            if (skip_leading_delim)
                goto cont;

            else {
                *lasts = s;
                s[-1] = 0;
                return (s - 1);
            }
        }
    }

    if (c == 0) {        /* no non-delimiter characters */
        *lasts = NULL;
        return NULL;
    }

    tok = s - 1;

    /*
     * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
    for (;;) {
        c = *s++;
        spanp = (char *)delim;

        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;

                else
                    s[-1] = 0;

                *lasts = s;
                return (tok);
            }
        } while (sc != 0);
    }
}

struct tm *gmtime_r(const time_t *time, struct tm *spec)
{
    int ret = gmtime_s(spec, time);
    return ret == 0 ? spec : NULL;
}

char *strndup(const char *str)
{
    char *ptr = kalloc(strlen(str) + 1);
    strcpy(ptr, str);
    return ptr;
}

