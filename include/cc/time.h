
#ifndef _TIME_H
#define _TIME_H 1

#include <bits/cdefs.h>


typedef unsigned int time_t;

struct timespec {
  int tv_sec;
  unsigned int tv_nsec;
};

struct tm {
  int     tm_sec;         /* seconds after the minute [0-60] */
  int     tm_min;         /* minutes after the hour [0-59] */
  int     tm_hour;        /* hours since midnight [0-23] */
  int     tm_mday;        /* day of the month [1-31] */
  int     tm_mon;         /* months since January [0-11] */
  int     tm_year;        /* years since 1900 */
  int     tm_wday;        /* days since Sunday [0-6] */
  int     tm_yday;        /* days since January 1 [0-365] */
  int     tm_isdst;       /* Daylight Savings Time flag */
  long    tm_gmtoff;      /* offset from UTC in seconds */
  char    *tm_zone;       /* timezone abbreviation */
};

#define CLOCKS_PER_SEC  ((clock_t)1000000)


char *asctime_r(const struct tm *date, char *str);
char *asctime(const struct tm *date);
time_t timegm(struct tm *tm);
time_t mktime(struct tm *tm);
struct tm *gmtime_r(const time_t *time, struct tm *tm);
struct tm *gmtime(const time_t *time);
void tzset();
struct tm *localtime(const time_t *timep);



#endif /* _TIME_H */
