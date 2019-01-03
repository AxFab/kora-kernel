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
#ifndef _TIME_H
#define _TIME_H 1

#include <stddef.h>
#include <kernel/types.h>

typedef __time_t time_t;
typedef __clock_t clock_t;

#define CLOCKS_PER_SEC  1000000L

struct timespec {
    time_t tv_sec;    /* Seconds.  */
    long int tv_nsec;   /* Nanoseconds.  */
};


/* Used by other time functions.  */
struct tm {
    /* Seconds. [0-60] (1 leap second) */
    int tm_sec;
    /* Minutes. [0-59] */
    int tm_min;
    /* Hours. [0-23] */
    int tm_hour;
    /* Day.   [1-31] */
    int tm_mday;
    /* Month. [0-11] */
    int tm_mon;
    /* Year - 1900.  */
    int tm_year;
    /* Day of week. [0-6] */
    int tm_wday;
    /* Days in year.[0-365] */
    int tm_yday;
    /* DST.   [-1/0/1]*/
    int tm_isdst;
    /* Seconds east of UTC.  */
    long int tm_gmtoff;
    /* Timezone abbreviation.  */
    const char *tm_zone;
};


/* Transform date and time to ASCII */
char *asctime(const struct tm *tm);
/* Transform date and time to ASCII */
char *ctime(const time_t *timep);
/* Calculate time difference */
double difftime(time_t time1, time_t time0);
/* Transform date and time to broken-down time */
struct tm *gmtime(const time_t *timep);
struct tm *gmtime_r(const time_t *timep, struct tm *tm);
/* Transform date and time to broken-down time */
struct tm *localtime(const time_t *timep);
/* Transform broken-down time to timestamp */
time_t mktime(struct tm *tm);
/* Format date and time */
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
/* Determine processor time */
clock_t clock(void);
/* Get time in seconds  */
time_t time(time_t *p);

time_t timegm(struct tm *tm);

void nanosleep(struct timespec *req, struct timespec *rest);

#endif  /* _TIME_H */
