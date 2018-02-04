#include <kernel/core.h>
#include <time.h>

#define CURRENT_YEAR        2017
#define CURRENT_CENTURY     (CURRENT_YEAR / 100)

#define CMOS_ADDRESS    0x70
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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
static inline int RTC_OnUpdate()
{
    outb(CMOS_ADDRESS, 0x0A);
    return inb (CMOS_DATA) & 0x80;
}

static inline unsigned char RTC_Read (int reg)
{
    outb(CMOS_ADDRESS, reg);
    return inb (CMOS_DATA);
}

static inline void RTC_ReadTm (struct tm *date, int *century)
{
    while (RTC_OnUpdate());

    date->tm_sec = RTC_Read(RTC_SECONDS);
    date->tm_min = RTC_Read(RTC_MINUTES);
    date->tm_hour = RTC_Read(RTC_HOURS);
    date->tm_wday = RTC_Read(RTC_DAY_WEEK);
    date->tm_mday = RTC_Read(RTC_DAY_MONTH);
    date->tm_mon = RTC_Read(RTC_MONTH);
    date->tm_year = RTC_Read(RTC_YEAR);

    if (RTC_CENTURY) {
        (*century) = RTC_Read(RTC_CENTURY);
    }
}

static inline int RTC_Equals(struct tm *d1, struct tm *d2)
{
    return  d1->tm_sec == d2->tm_sec &&
            d1->tm_min == d2->tm_min &&
            d1->tm_hour == d2->tm_hour &&
            d1->tm_wday == d2->tm_wday &&
            d1->tm_mday == d2->tm_mday &&
            d1->tm_mon == d2->tm_mon &&
            d1->tm_year == d2->tm_year;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
void RTC_EnableCMOS()
{
    int rate = 7; /* 3->8Khz  6->1024Hz 7->512Hz */
    outb(CMOS_ADDRESS, 0x8B);
    char b = inb(CMOS_DATA);
    outb(CMOS_ADDRESS, 0x8B);
    outb(CMOS_DATA, b | 0x40);

    outb(CMOS_ADDRESS, 0x8A);   /* set index to register A, disable NMI */
    char p = inb(CMOS_DATA); /* get initial value of register A */
    outb(CMOS_ADDRESS, 0x8A);   /* reset index to A */
    outb(CMOS_DATA, (p & 0xF0) |
         rate); //write only our rate to A. Note, rate is the bottom 4 bits.
}

void RTC_DisableCMOS()
{
    outb(CMOS_ADDRESS, 0x0B);
    outb(CMOS_DATA, inb(CMOS_DATA) & 0xBF);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
struct tm RTC_GetTime()
{
    struct tm date;
    struct tm last;
    int century;
    int regB;

    RTC_ReadTm (&date, &century);

    /* Check if we got it right */
    do {
        last = date;
        RTC_ReadTm (&date, &century);
    } while (!RTC_Equals(&last, &date));

    regB = RTC_Read(RTC_REGISTER_B);

    /* Convert BCD to binary values */
    if (!(regB & 0x04)) {
        date.tm_sec = (date.tm_sec & 0x0F) + ((date.tm_sec / 16) * 10);
        date.tm_min = (date.tm_min & 0x0F) + ((date.tm_min / 16) * 10);
        date.tm_hour = ( (date.tm_hour & 0x0F) + (((date.tm_hour & 0x70) / 16) * 10) )
                       | (date.tm_hour & 0x80);
        date.tm_mday = (date.tm_mday & 0x0F) + ((date.tm_mday / 16) * 10);
        date.tm_mon = (date.tm_mon & 0x0F) + ((date.tm_mon / 16) * 10);
        date.tm_year = (date.tm_year & 0x0F) + ((date.tm_year / 16) * 10);

        if (RTC_CENTURY) {
            century = (century & 0x0F) + ((century / 16) * 10);
        }
    }

    /* Convert 12 hour clock to 24 hour clock */
    if (!(regB & 0x02) && (date.tm_hour & 0x80)) {
        date.tm_hour = ((date.tm_hour & 0x7F) + 12) % 24;
    }

    /* Calculate the full (4-digit) year */
    if (RTC_CENTURY != 0) {
        date.tm_year += century * 100;
    } else {
        date.tm_year += CURRENT_CENTURY * 100;
    }

    /* FIXME Compute the rest of tm struct */
    date.tm_yday = 0;
    date.tm_isdst = 0;
    date.tm_year -= 1900;
    date.tm_mon--;

    date.tm_wday = date.tm_wday % 7;
    /* date.tm_mon = date.tm_mon % 12; */
    return date;
}

