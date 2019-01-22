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
#include <kernel/cpu.h>
#include <kernel/syscalls.h>
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

int pp[] = { -1, -1 };


void tty_start()
{
    inode_t *dev;
    for (;;) {
        dev = vfs_search_device("kdb");
        if (dev != NULL)
            break;
        sys_sleep(10000);
    }
    // kprintf(-1, "Tty found keyboard\n");

    while (pp[1] == -1)
        sys_sleep(10000);

    event_t event;
    for (;;) {
        vfs_read(dev, (char *)&event, sizeof(event), 0, 0);
        int key = event.param2 & 0xFFF;
        int mod = event.param2 >> 16;
        kprintf(-1, "KDB %d (%x - %x)\n", event.type, key, mod);
    }
}


extern tty_t *slog;
void desktop();


void kernel_master()
{
    // task_create(tty_start, NULL, "Syslog Tty");
    // task_create(kernel_top, (void *)5, "Dbg top 5s");
    sys_sleep(5000);
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

    task_create(desktop, NULL, "Desktop #1");

    sys_sleep(10000);
    task_show_all();
    kmod_dump();
    memory_info();

    // sys_sleep(1000000);
    // mspace_display(kMMU.kspace);

    sys_pipe(pp);

    char buf[32];
    int idx = 0;
    for (;;) {
        kprintf(-1, "Master >\n");
        idx = sys_read(pp[0], &buf[idx], 10 - idx);
        buf[idx] = '\0';
        char *nx = strchr(buf, '\n');
        if (nx != NULL) {
            nx[0] = '\0';
            kprintf(-1, buf, nx - buf, 0, 0);
            strcpy(buf, &nx[1]);
            idx -= nx - buf;
        } else
            idx = 0;
    }
    sys_close(pp[0]);
}

void kmod_loader();

long irq_syscall(long no, long a1, long a2, long a3, long a4, long a5)
{
    kprintf(-1, "Syscall\n");
    return -1;
}

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
    slog = tty_create(128);
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
