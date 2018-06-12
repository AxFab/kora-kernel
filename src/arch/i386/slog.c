#include <kernel/core.h>

void csl_early_init();
int csl_write(inode_t *ino, const char *buf, int len);

void kwrite(const char *buf, int len)
{
    csl_write(NULL, buf, len);
}

uint64_t cpu_clock()
{
    return 0;
}

time_t cpu_time()
{
    return 0;
}