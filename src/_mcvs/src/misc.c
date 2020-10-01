#include <kernel/core.h>
#include <windows.h>
#include <kernel/utils.h>
#include <Windows.h>

void clock_gettime(int no, struct timespec *sp)
{
}

void usleep(__useconds_t usecs)
{
    Sleep(usecs / 1000);
}

void rand_r() {}

utime_t cpu_clock(int no)
{
    FILETIME tm;
    GetSystemTimePreciseAsFileTime(&tm);
    uint64_t cl = (uint64_t)tm.dwHighDateTime << 32 | tm.dwLowDateTime;
    return cl / 10LL - SEC_TO_USEC(11644473600LL);
}

/* Searches the first len bytes of array str for character c. */
void *memrchr(const void *str, int c, size_t len)
{
    register const char *ptr0 = ((const char *)str) + len;

    while (len > 0 && (*ptr0 != (char)c)) {
        --ptr0;
        --len;
    }

    return (void *)(len ? ptr0 : 0);
}

/* Scans s1 for the first token not contained in s2. */
char *strtok_r(register char *s, const char *delim, char **lasts)
{
    int skip_leading_delim = 1;
    register char *spanp;
    register int c, sc;
    char *tok;

    if (s == NULL && (s = *lasts) == NULL)
        return NULL;

    /*
     * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
     */
cont:
    c = *s++;

    for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
        if (c == sc) {
            if (skip_leading_delim)
                goto cont;

            else {
                *lasts = s;
                s[-1] = 0;
                return (s - 1);
            }
        }
    }

    if (c == 0) {        /* no non-delimiter characters */
        *lasts = NULL;
        return NULL;
    }

    tok = s - 1;

    /*
     * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
    for (;;) {
        c = *s++;
        spanp = (char *)delim;

        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;

                else
                    s[-1] = 0;

                *lasts = s;
                return (tok);
            }
        } while (sc != 0);
    }
}

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

