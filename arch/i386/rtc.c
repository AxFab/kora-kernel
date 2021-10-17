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
#include <kernel/stdc.h>
#include <kernel/arch.h>
#include <kora/mcrs.h>
#include <string.h>
#include <time.h>

#define CURRENT_YEAR        2018
#define CURRENT_CENTURY     (CURRENT_YEAR / 100)

#define CMOS_CMD        0x70
#define CMOS_DATA       0x71

#define RTC_SECONDS     0x00
#define RTC_MINUTES     0x02
#define RTC_HOURS       0x04
#define RTC_DAY_WEEK    0x06
#define RTC_DAY_MONTH   0x07
#define RTC_MONTH       0x08
#define RTC_YEAR        0x09
#define RTC_REGISTER_A  0x0A
#define RTC_REGISTER_B  0x0B
#define RTC_CENTURY     rtc_century_reg

int rtc_century_reg = 0; /* Set by ACPI table parsing code if possible */


#define BCD2DEC(v) (((v)&0xf)+(((v)>>4)&0xf)*10)

static inline uint8_t cmos_rd(uint8_t idx)
{
    outb(CMOS_CMD, idx);
    return inb(CMOS_DATA);
}

// static inline void cmos_wr(uint8_t idx, uint8_t val)
// {
//     outb(CMOS_CMD, idx);
//     outb(CMOS_DATA, val);
// }

static inline void cmos_read_tm(struct tm *date)
{
    do {
        outb(CMOS_CMD, 0x0A);
    } while (inb(CMOS_DATA) & 0x80);

    date->tm_sec = BCD2DEC(cmos_rd(RTC_SECONDS));
    date->tm_min = BCD2DEC(cmos_rd(RTC_MINUTES));
    int hour = cmos_rd(RTC_HOURS);
    date->tm_hour = BCD2DEC(hour & 0x7F) | (hour & 0x80);
    date->tm_wday = BCD2DEC(cmos_rd(RTC_DAY_WEEK));
    date->tm_mday = BCD2DEC(cmos_rd(RTC_DAY_MONTH));
    date->tm_mon = BCD2DEC(cmos_rd(RTC_MONTH));
    date->tm_year = CURRENT_CENTURY * 100 + BCD2DEC(cmos_rd(RTC_YEAR));
}

static inline void cmos_fix_tm(struct tm *date)
{
    int regB = cmos_rd(RTC_REGISTER_B);
    /* Convert 12 hour clock to 24 hour clock */
    if (!(regB & 0x02) && (date->tm_hour & 0x80))
        date->tm_hour = ((date->tm_hour & 0x7F) + 12) % 24;

    date->tm_yday = 0;
    date->tm_isdst = 0;
    date->tm_year -= 1900;
    date->tm_mon--;
    date->tm_wday--;

    date->tm_wday = date->tm_wday % 7;
}

xtime_t rtc_time()
{
    struct tm date;
    struct tm last;
    memset(&date, 0, sizeof(date));
    memset(&last, 0, sizeof(last));

    cmos_read_tm(&date);
    /* Check if we got it right */
    do {
        last = date;
        cmos_read_tm(&date);
    } while (memcmp(&last, &date, sizeof(struct tm)) != 0);

    cmos_fix_tm(&date);
    kprintf(0, "RTC clock: %s", asctime(&date));
    return SEC_TO_USEC(timegm(&date));
}
