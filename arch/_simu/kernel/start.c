#include <stdatomic.h>

struct {
    int cpu_count;
    const char *cpu_vendor;
} opts;

int __cpu_inc = 0;
__thread __cpu_no = 0;

int cpu_no()
{
    return __cpu_no;
}

void page_range(long long base, long long limit);

int parse_args(int argc, char **argv)
{
}

int main(int argc, char **argv)
{
    int i;
    // Initialize variables
    opts.cpu_count = 2;
    opts.cpu_vendor = "EloCorp.";
    // Parse args to customize the simulated platform
    parse_args(argc, argv);
    // Setup pages
    page_range(5 * PAGE_SIZE, 5 *);
    page_range(25 * PAGE_SIZE, 20 * PAGE_SIZE);
    // Initialize memory mapping



    return 0;
}

