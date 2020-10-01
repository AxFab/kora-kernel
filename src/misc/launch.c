/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#include <time.h>
#include <kernel/core.h>
#include <kernel/device.h>
#include <kernel/memory.h>
#include <kernel/files.h>
#include <kernel/input.h>
#include <kernel/task.h>
#include <kernel/syscalls.h>
#include <kernel/cpu.h>
// #include <kora/iofile.h>
#include <kora/llist.h>
#include <string.h>


// void ARP_who_is(const unsigned char *ip);
void clock_init();
void sys_ticks();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


struct kCpu kCPU0;


llhead_t modules = INIT_LLHEAD;

extern int no_dbg;

void kernel_top(long sec)
{
    sys_sleep(10000);
    for (;;) {
        sys_sleep(SEC_TO_KTIME(sec));
        task_show_all();
        mspace_display(kMMU.kspace);
        memory_info();
    }
}



extern tty_t *slog;
void wmgr_main();


void kmod_loader();
void futex_init();
void itimer_init();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void exec_proc(const char **exec_args);


/* Kernel entry point, must be reach by a single CPU */
void kernel_start()
{
    kSYS.cpus = &kCPU0;
    irq_reset(false);
    irq_disable();
    kprintf(KLOG_MSG, "\033[97mKoraOS\033[0m - " __ARCH " - v" _VTAG_ "\nBuild the " __DATE__ ".\n");

    /* Architecture initialization */
    assert(kCPU.irq_semaphore == 1);
    // mmu_setup();
    memory_initialize();
    memory_info();
    cpu_setup();
    assert(kCPU.irq_semaphore == 1);

    kprintf(KLOG_MSG, "\n");
    // slog = tty_create(1024);
    kprintf(KLOG_MSG, "\033[94m  Greetings on KoraOS...\033[0m\n");

    // kprintf(KLOG_IRQ, "Kernel start, on CPU%d, stack %p.\n", cpu_no(), ALIGN_UP((size_t)&i, PAGE_SIZE));
    assert(kCPU.irq_semaphore == 1);
    vfs_init();
    futex_init();
    itimer_init();
    kmod_init();
    platform_setup();
    assert(kCPU.irq_semaphore == 1);

    kSYS.init_fs = rxfs_create(kSYS.dev_ino);
    task_create(kmod_loader, NULL, "Kernel loader #1");
    // task_create(kmod_loader, NULL, "Kernel loader #2");

    task_create(exec_init, NULL, "Init");

    clock_init();
    assert(kCPU.irq_semaphore == 1);
    irq_reset(false);
}

/* Kernel secondary entry point, must be reach by additional CPUs */
void kernel_ready()
{
    /*
    int no = cpu_no();
    assert(no != 0);
    // kSYS.cpus[no] = (struct kCpu*)kalloc(sizeof(struct kCpu));
    irq_reset(false);
    irq_disable();
    assert(kCPU.irq_semaphore == 1);
    */
    //PIT_set_interval(CLOCK_HZ);
    kprintf(KLOG_MSG, "\033[32mCpu %d is waiting...\033[0m\n", cpu_no());
    for (;;);
    irq_reset(false);
}

void kernel_sweep()
{
    kmod_t *mod = ll_first(&modules, kmod_t, node);
    while (mod) {
        mod->teardown();
        kmod_t *rm = mod;
        mod = ll_next(&mod->node, kmod_t, node);
        kfree(rm);
    }
    memset(&modules, 0, sizeof(modules));
}
