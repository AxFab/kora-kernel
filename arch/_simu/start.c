#include <kernel/core.h>
#include "opts.h"

struct opts opts;


int parse_args(int argc, char **argv)
{
    return 0;
}

int kwrite(const char *buf, int len)
{
    return write(1, buf, len);
}

void new_cpu()
{
    kprintf(-1, "CPU.%d MP\n", cpu_no());
}

int kmod_loaderrf() {}

int main(int argc, char **argv)
{
    if (argc < 2) {
        kprintf(-1, "No arguments...\n");
        return 0;
    }
    // Initialize variables
    opts.cpu_count = 1;
    opts.cpu_vendor = "EloCorp.";
    // Parse args to customize the simulated platform
    parse_args(argc, argv);
    // Start of the kernel simulation
    kprintf(-1, "\033[1;97mKoraOs\033[0m\n");
    mmu_setup();
    cpu_setup();
    vfs_init();
    futex_init();
    // scheduler_init();

    task_create(kmod_loaderrf, NULL, "Kernel loader #1");
    // Save syslogs
    // Load drivers
    // Irq on !
    // Start kernel tasks
    clock_ticks();
    return 0;
}

