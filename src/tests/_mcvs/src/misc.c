#include <kernel/core.h>
#include <windows.h>
#include <kernel/utils.h>

void clock_gettime(int no, struct timespec *sp)
{
}

void usleep(__useconds_t usecs)
{
    Sleep(usecs / 1000);
}

void rand_r() {}

utime_t cpu_clock(int no) {}


char *strtok_r() {}

struct tm *gmtime_r(const time_t *time, struct tm *spec)
{
    return spec;
}

char *strndup(const char *str)
{
    char *ptr = malloc(strlen(str) + 1);
    strcpy(ptr, str);
    return ptr;
}

void futex_tick() {}
