#include <kernel/core.h>
#include <windows.h>

void clock_gettime(int no, struct timespec *sp)
{
}

void usleep(__useconds_t usecs)
{
    Sleep(usecs / 1000);
}

void rand_r() {}

void clock_read() {}

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


