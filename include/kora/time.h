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
#ifndef __KORA_TIME_H
#define __KORA_TIME_H 1

typedef long long xtime_t;
typedef enum xtime_name xtime_name_t;

enum xtime_name {
    XTIME_CLOCK = 0,
    XTIME_UTC,
    XTIME_LOCAL,
    XTIME_MONOTONIC,
    XTIME_UPTIME,
    XTIME_THREAD,
};


xtime_t xtime_read(xtime_name_t name);
// xtime_t xtime_adjust(xtime_t expected);

#endif  /* __KORA_TIME_H */
