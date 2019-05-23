#ifndef _BITS_TIMESPEC_H
#define _BITS_TIMESPEC_H 1

#include <bits/cdefs.h>
#include <bits/types.h>

struct timespec {
  __time_t tv_sec;
  long tv_nsec;
};

#endif
