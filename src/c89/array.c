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
#include <string.h>
#include <kora/std.h>


static void memswap(void *a, void *b, void *t, size_t sz)
{
    memcpy(t, a, sz);
    memcpy(a, b, sz);
    memcpy(b, t, sz);
}

/* Sort an array */
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
    int i;
    int beginIdx = - 1;
    int endIdx = nmemb - 1;
    int swapped = 1;
    char *array = (char *)base;
    void *tmp = malloc(size);
    do {
        swapped = 0;
        beginIdx++;
        for (i = beginIdx; i < endIdx; ++i) {
            if (compar(&array[size * i], &array[size * (i + 1)]) > 0) {
                memswap(&array[size * i], &array[size * (i + 1)], tmp, size);
                swapped = 1;
            }
        }

        if (!swapped)
            break;

        swapped = 0;
        endIdx--;
        for (i = beginIdx; i < endIdx; ++i) {
            if (compar(&array[size * i], &array[size * (i + 1)]) > 0) {
                memswap(&array[size * i], &array[size * (i + 1)], tmp, size);
                swapped = 1;
            }
        }
    } while (swapped);
    free(tmp);
}


/* Binary search of a sorted array */
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));

