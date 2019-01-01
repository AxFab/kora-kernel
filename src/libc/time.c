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
#include <time.h>
#include <assert.h>
#include <errno.h>

int snprintf(char *, size_t, const char *, ...);
#if defined(_WIN32)
int sprintf_s(char *buf, int lg, const char *msg, ...);
# define snprintf(s,i,f,...) sprintf_s(s,i,f,__VA_ARGS__)
#endif


/* 2000-03-01 (mod 400 year, immediately after feb29 */
#define LEAPOCH (946684800LL + 86400*(31+29))

#define DAYS_PER_400Y (365*400 + 97)
#define DAYS_PER_100Y (365*100 + 24)
#define DAYS_PER_4Y   (365*4   + 1)


#undef INT_MAX
#define INT_MAX ((int)2147483647)

#undef INT_MIN
#define INT_MIN ((int)-INT_MAX - 1)


// static const char *const weekDayStrings[] = {
//     "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
// };


static const char *const shWeekDayStrings[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};


static const char *const shMonthStrings[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


static long long __yeartosecs(long long year, int *is_leap)
{
    if (year - 2ULL <= 136) {
        int y = year;
        int leaps = (y - 68) >> 2;

        if (!((y - 68) & 3)) {
            leaps--;

            if (is_leap)
                *is_leap = 1;
        } else if (is_leap)
            *is_leap = 0;

        return 31536000 * (y - 70) + 86400 * leaps;
    }

    int cycles, centuries, leaps, rem;

    if (!is_leap)
        is_leap = &(int) {
        0
    };

    cycles = (year - 100) / 400;

    rem = (year - 100) % 400;

    if (rem < 0) {
        cycles--;
        rem += 400;
    }

    if (!rem) {
        *is_leap = 1;
        centuries = 0;
        leaps = 0;
    } else {
        if (rem >= 200) {
            if (rem >= 300)
                centuries = 3, rem -= 300;

            else
                centuries = 2, rem -= 200;
        } else {
            if (rem >= 100)
                centuries = 1, rem -= 100;

            else
                centuries = 0;
        }

        if (!rem) {
            *is_leap = 0;
            leaps = 0;
        } else {
            leaps = rem / 4U;
            rem %= 4U;
            *is_leap = !rem;
        }
    }

    leaps += 97 * cycles + 24 * centuries - *is_leap;

    return (year - 100) * 31536000LL + leaps * 86400LL + 946684800 + 86400;
}


static int __monthtosecs(int month, int is_leap)
{
    static const int secs_through_month[] = {
        0, 31 * 86400, 59 * 86400, 90 * 86400,
        120 * 86400, 151 * 86400, 181 * 86400, 212 * 86400,
        243 * 86400, 273 * 86400, 304 * 86400, 334 * 86400
    };
    int t = secs_through_month[month];

    if (is_leap && month >= 2)
        t += 86400;

    return t;
}


static long long __tmtosecs(const struct tm *tm)
{
    int is_leap;
    long long year = tm->tm_year;
    int month = tm->tm_mon;

    if (month >= 12 || month < 0) {
        int adj = month / 12;
        month %= 12;

        if (month < 0) {
            adj--;
            month += 12;
        }

        year += adj;
    }

    long long t = __yeartosecs(year, &is_leap);
    t += __monthtosecs(month, is_leap);
    t += 86400LL * (tm->tm_mday - 1);
    t += 3600LL * tm->tm_hour;
    t += 60LL * tm->tm_min;
    t += tm->tm_sec;
    return t;
}

static int __secstotm(long long t, struct tm *tm)
{
    long long days, secs;
    int remdays, remsecs, remyears;
    int qc_cycles, c_cycles, q_cycles;
    int years, months;
    int wday, yday, leap;
    static const char days_in_month[] = {31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29};

    /* Reject time_t values whose year would overflow int */
    if (t < INT_MIN * 31622400LL || t > INT_MAX * 31622400LL)
        return -1;

    secs = t - LEAPOCH;
    days = secs / 86400;
    remsecs = secs % 86400;

    if (remsecs < 0) {
        remsecs += 86400;
        days--;
    }

    wday = (3 + days) % 7;

    if (wday < 0)
        wday += 7;

    qc_cycles = days / DAYS_PER_400Y;
    remdays = days % DAYS_PER_400Y;

    if (remdays < 0) {
        remdays += DAYS_PER_400Y;
        qc_cycles--;
    }

    c_cycles = remdays / DAYS_PER_100Y;

    if (c_cycles == 4)
        c_cycles--;

    remdays -= c_cycles * DAYS_PER_100Y;

    q_cycles = remdays / DAYS_PER_4Y;

    if (q_cycles == 25)
        q_cycles--;

    remdays -= q_cycles * DAYS_PER_4Y;

    remyears = remdays / 365;

    if (remyears == 4)
        remyears--;

    remdays -= remyears * 365;

    leap = !remyears && (q_cycles || !c_cycles);
    yday = remdays + 31 + 28 + leap;

    if (yday >= 365 + leap)
        yday -= 365 + leap;

    years = remyears + 4 * q_cycles + 100 * c_cycles + 400 * qc_cycles;

    for (months = 0; days_in_month[months] <= remdays; months++)
        remdays -= days_in_month[months];

    if (years + 100 > INT_MAX || years + 100 < INT_MIN)
        return -1;

    tm->tm_year = years + 100;
    tm->tm_mon = months + 2;

    if (tm->tm_mon >= 12) {
        tm->tm_mon -= 12;
        tm->tm_year++;
    }

    tm->tm_mday = remdays + 1;
    tm->tm_wday = wday;
    tm->tm_yday = yday;

    tm->tm_hour = remsecs / 3600;
    tm->tm_min = remsecs / 60 % 60;
    tm->tm_sec = remsecs % 60;

    return 0;
}


// FIXME use local
char *asctime_r(const struct tm *date, char *str)
{
    // assert(date->tm_wday >= 0 && date->tm_wday < 7);
    // assert(date->tm_mon >= 0 && date->tm_mon < 12);
    snprintf(str, 26, "%.3s %.3s %2d %.2d:%.2d:%.2d %d\n",
             shWeekDayStrings[date->tm_wday % 7], shMonthStrings[date->tm_mon % 12],
             date->tm_mday, date->tm_hour,
             date->tm_min, date->tm_sec,
             1900 + date->tm_year);
    return str;
}


char *asctime(const struct tm *date)
{
    static char buf[30];
    return asctime_r(date, buf);
}

time_t timegm(struct tm *tm)
{
    struct tm datetime;
    long long t = __tmtosecs(tm);

    if (__secstotm(t, &datetime) < 0) {
        errno = EOVERFLOW;
        return -1;
    }

    *tm = datetime;
    tm->tm_isdst = 0;
    return t;
}

time_t mktime(struct tm *tm)
{
    return timegm(tm);
}


struct tm *gmtime_r(const time_t *time, struct tm *tm)
{
    if (__secstotm(*time, tm) < 0) {
        errno = EOVERFLOW;
        return 0;
    }
    tm->tm_isdst = 0;
    // tm->__tm_gmtoff = 0;
    // tm->__tm_zone = __gmt;
    return tm;
}

struct tm *gmtime(const time_t *time)
{
    static struct tm tm;
    return gmtime_r(time, &tm);
}

