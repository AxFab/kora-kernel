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
 */
#include <stdint.h>

static unsigned int __seed;

#define RAND_MAX 0x7FFF

/* Sets the seed for a new sequence of pseudo-random integers. */
void srand(unsigned int seed)
{
    __seed = seed;
}

/* Pseudo-random generator based on Minimal Standard by
   Lewis, Goodman, and Miller in 1969: I[j+1] = a*I[j] (mod m) */
int rand_r(unsigned int *seed)
{
    long k;
    long s = (long)(*seed);
    if (s == 0)
        s = 0x12345987;
    k = s / 127773;
    s = 16807 * (s - k * 127773) - 2836 * k;
    if (s < 0)
        s += 2147483647;
    (*seed) = (unsigned int)s;
    return (int)(s & RAND_MAX);
}

/* Pseudo-random generator */
#ifndef _WIN32
int rand(void)
{
    return rand_r(&__seed);
}
#endif

uint64_t rand64()
{
	unsigned int seed = __seed;
    uint64_t r = (uint64_t)rand_r(&seed);
    r |= (uint64_t)rand_r(&seed) << 15;
    r |= (uint64_t)rand_r(&seed) << 30;
    r |= (uint64_t)rand_r(&seed) << 45;
    r |= (uint64_t)rand_r(&seed) << 60;
    return r;
}

uint32_t rand32()
{
	unsigned int seed = __seed;
    uint32_t r = (uint32_t)rand_r(&seed);
    r |= (uint32_t)rand_r(&seed) << 15;
    r |= (uint32_t)rand_r(&seed) << 30;
    return r;
}

uint16_t rand16()
{
	unsigned int seed = __seed;
    uint32_t r = (uint16_t)rand_r(&seed);
    r |= (uint16_t)rand_r(&seed) << 15;
    return r & 0xFFFF;
}

uint8_t rand8()
{
	unsigned int seed = __seed;
    uint32_t r = (uint16_t)rand_r(&seed);
    return r & 0xFF;
}
