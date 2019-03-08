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
#include <kora/iofile.h>
#include <kora/llist.h>
#include <string.h>


// void ARP_who_is(const unsigned char *ip);
void clock_init();
void sys_ticks();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


struct kSys kSYS;
struct kCpu kCPU0;


llhead_t modules = INIT_LLHEAD;

extern int no_dbg;

void kernel_top(long sec)
{
    sys_sleep(10000);
    for (;;) {
        async_wait(NULL, NULL, sec * 1000000);
        task_show_all();
        mspace_display(kMMU.kspace);
        memory_info();
    }
}



extern tty_t *slog;
void wmgr_main();


void kernel_master()
{
    // Read kernel command, load modules and mount correct device
#if defined KORA_KRN
    for (;;) {
        inode_t *dev = vfs_search_device("sdC");
        if (dev != NULL)
            break;
        sys_sleep(10000);
    }

    // vfs_mount(root, "dev", NULL, "devfs");
    // vfs_mount(root, "tmp", NULL, "tmpfs");

    // Look for home file system
    inode_t *root = vfs_mount("sdC", "isofs");
    if (root == NULL) {
        kprintf(-1, "Expected mount point over 'sdC' !\n");
        sys_exit(0);
    }

    resx_fs_chroot(kCPU.running->resx_fs, root);
    resx_fs_chpwd(kCPU.running->resx_fs, root);
    vfs_close(root);
#endif

    task_create(wmgr_main, NULL, "Local display");

    sys_sleep(10000);
    task_show_all();
    kmod_dump();
    memory_info();

    // sys_sleep(1000000);
    // mspace_display(kMMU.kspace);

    for (;;)
        sys_sleep(SEC_TO_KTIME(60));
}

void kmod_loader();


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Kernel entry point, must be reach by a single CPU */
void kernel_start()
{
    kSYS.cpus = &kCPU0;
    irq_reset(false);
    irq_disable();
    kprintf(KLOG_MSG, "\033[97mKoraOS\033[0m - " __ARCH " - v" _VTAG_ "\nBuild the " __DATE__ ".\n");

    assert(kCPU.irq_semaphore == 1);
    memory_initialize();
    // Resolve page fault for allocation -- circular deps between mspace_map and kalloc
    kalloc(2);
    memory_info();
    assert(kCPU.irq_semaphore == 1);

    cpu_setup();
    assert(kCPU.irq_semaphore == 1);

    kprintf(KLOG_MSG, "\n");
    slog = tty_create(1024);
    kprintf(KLOG_MSG, "\033[94m  Greetings on KoraOS...\033[0m\n");

    assert(kCPU.irq_semaphore == 1);
    vfs_init();
    kmod_init();
    platform_setup();
    assert(kCPU.irq_semaphore == 1);

    task_create(kmod_loader, NULL, "Kernel loader #1");
    task_create(kmod_loader, NULL, "Kernel loader #2");
    task_create(kernel_master, NULL, "Master");

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
