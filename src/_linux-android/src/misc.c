// #include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
// #include <unistd.h>
// #include "check.h"
#include <threads.h>

#include <kernel/task.h>
#include <kernel/futex.h>


utime_t cpu_clock(int clk)
{
    struct timespec xt;
    clock_gettime(clk, &xt);
    return TMSPEC_TO_USEC(xt);
}

int posix_memalign(void **, size_t, size_t);
void *valloc(size_t len)
{
    void *ptr;
    posix_memalign(&ptr, PAGE_SIZE, len);
    return ptr;
}


