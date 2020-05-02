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
#include <stddef.h>
#include <errno.h>

int mbtowc(wchar_t *wc, const char *str, size_t len)
{
    wchar_t dummy;
    errno = 0;
    if (str == NULL)
        return 0;
    if (wc == NULL)
        wc = &dummy;

    if (len < 1) {
        errno = EILSEQ;
        return -1;
    }

    if (*str >= 0)
        /* Returns 0 if '\0' or 1 in other cases */
        return !!(*wc = *str);


    *wc = 1;
    return -1;
}

/*
size_t mbstowcs (wchar_t *ws, const char **str, size_t wn, mbstate_t *st)
{
    return 0;
}

size_t mbstowcs (wchar_t *ws, const char *str, size_t wn)
{
    return mbsrtowcs(ws, &s, wn, 0);
}
*/

int mblen(const char *str, size_t len)
{
    return mbtowc(0, str, len);
}
