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
// #include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
// #include <unistd.h>
// #include "check.h"
#include <threads.h>

#include <kernel/task.h>
#include <kernel/futex.h>


utime_t cpu_clock(int clk)
{
    struct timespec xt;
    clock_gettime(clk, &xt);
    return TMSPEC_TO_USEC(xt);
}

int posix_memalign(void **, size_t, size_t);
void *valloc(size_t len)
{
    void *ptr;
    posix_memalign(&ptr, PAGE_SIZE, len);
    return ptr;
}
