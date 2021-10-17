/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
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
