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
#include <errno.h>
#include <time.h>
#include <kernel/core.h>
#include "../check.h"

#if !defined(_WIN32)
time64_t time64()
{
    clock_t ticks = clock();
    if (_PwNano_ > CLOCKS_PER_SEC)
        ticks *= _PwNano_ / CLOCKS_PER_SEC;
    else
        ticks /= CLOCKS_PER_SEC / _PwNano_;
    return ticks;
}
#else
#include <windows.h>
time64_t time64()
{
    // January 1, 1970 (start of Unix epoch) in ticks
    const INT64 UNIX_START = 0x019DB1DED53E8000;

    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    LARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    // Convert ticks since EPOCH into nano-seconds
    return (li.QuadPart - UNIX_START) * 100;
}
#endif
