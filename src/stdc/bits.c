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
#include <stdint.h>
#include <stdbool.h>
#include <kora/mcrs.h>

void bitsclr(uint8_t *ptr, int start, int count)
{
    int i = start / 8;
    int j = start % 8;
    while (count > 0 && j != 0) {
        ptr[i] &= ~(1 << j);
        j = (j + 1) % 8;
        count--;
    }
    i = ALIGN_UP(start, 8) / 8;
    while (count > 8) {
        ptr[i] = 0;
        i++;
        count -= 8;
    }
    j = 0;
    while (count > 0) {
        ptr[i] &= ~(1 << j);
        j++;
        count--;
    }
}

void bitsset(uint8_t *ptr, int start, int count)
{
    int i = start / 8;
    int j = start % 8;
    while (count > 0 && j != 0) {
        ptr[i] |= (1 << j);
        j = (j + 1) % 8;
        count--;
    }
    i = ALIGN_UP(start, 8) / 8;
    while (count > 8) {
        ptr[i] = ~0;
        i++;
        count -= 8;
    }
    j = 0;
    while (count > 0) {
        ptr[i] |= (1 << j);
        j++;
        count--;
    }
}

int bitschrz(uint8_t *ptr, int len)
{
    int i = 0, j = 0;
    while (i < len / 8 && ptr[i] == 0xFF)
        i++;
    if (i >= len / 8)
        return -1;
    uint8_t by = ptr[i];
    while (by & 1) {
        j++;
        by = by >> 1;
    }
    return i * 8 + j;
}

bool bitstest(uint8_t *ptr, int start, int count)
{
    bool test = true;
    int i = start / 8;
    int j = start % 8;
    while (count > 0 && j != 0) {
        ptr[i] &= ~(1 << j);
        j = (j + 1) % 8;
        count--;
    }
    i = ALIGN_UP(start, 8) / 8;
    while (count > 8) {
        if (ptr[i] == 0)
            test = false;
        i++;
        count -= 8;
    }
    j = 0;
    while (count > 0) {
        if ((ptr[i] & (1 << j)) == 0)
            test = false;
        j++;
        count--;
    }
    return test;
}