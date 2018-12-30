/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2018  <Fabien Bavent>
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


void kernel_tasklet(void *start, void *arg, CSTR name)
{
    task_t *task = task_create(NULL, NULL, 0, name);
    task_start(task, start, arg);
}

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

// volatile inode_t *pp;
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
    kprintf(-1, "Tty found keyboard\n");

    // while (pp == NULL)
    while (pp[1] == -1)
        sys_sleep(10000);

    event_t event;
    for (;;) {
        vfs_read(dev, (char *)&event, sizeof(event), 0, 0);
        if (event.type == 3) {
            char c = seat_key_unicode(event.param2 & 0xFFF, event.param2 >> 16);
            kprintf(-1, "Tty key %x -> %x\n", event.param2 & 0xFFF, c);
            if ((c >= ' ' && c < 0x80) || c == '\n')
                sys_write(pp[1], &c, 1);
            // vfs_write(pp, &c, 1, 0, 0);
        }
    }
}


extern tty_t *slog;
void desktop();


void kernel_master()
{
    kernel_tasklet(desktop, NULL, "Desktop #1");
    kernel_tasklet(tty_start, NULL, "Syslog Tty");
    kernel_tasklet(kernel_top, (void *)5, "Dbg top 5s");
    sys_sleep(5000);
    for (;;) {
        inode_t *dev = vfs_search_device("sdC");
        if (dev != NULL)
            break;
        sys_sleep(10000);
    }
    kprintf(-1, "Master found the device\n");

    // vfs_mount(root, "dev", NULL, "devfs");
    // vfs_mount(root, "tmp", NULL, "tmpfs");

    // Look for home file system
    inode_t *root = vfs_mount("sdC", "isofs");
    if (root == NULL) {
        kprintf(-1, "Expected mount point over 'sdC' !\n");
        sys_exit(0);
    }

    kprintf(-1, "Master mounted root: %p\n", root);
    sys_sleep(10000);

    inode_t *ino;
    char name[256];
    void *dir_ctx = vfs_opendir(root, NULL);
    while ((ino = vfs_readdir(root, name, dir_ctx)) != NULL) {
        kprintf(-1, " %p  /%s%s\n", ino, name, ino->type == FL_DIR ? "/" : "");
        vfs_close(ino);
    }
    vfs_closedir(root, dir_ctx);

    sys_sleep(10000);

    // const char *txt_filename = "boot/grub/grub.cfg";
    const char *txt_filename = "BOOT/GRUB/MENU.LST";
    char *buf;
    inode_t *txt = vfs_search(root, root, txt_filename, NULL);
    if (txt != NULL) {
        kprintf(-1, "Reading file from CDROM...\n");
        buf = kalloc(txt->length + 1);
        vfs_read(txt, buf, txt->length, 0, 0);
        kprintf(-1, "CONTENT (%x) \n%s\n", txt->length, buf);
        vfs_close(txt);
        kfree(buf);
    } else
        kprintf(-1, "Unable to open %s using vfs_search\n", txt_filename);

    kCPU.running->pwd = root;
    int fd = sys_open(-1, txt_filename, 0);
    if (fd == -1)
        kprintf(-1, "Unable to open %s using sys_open\n", txt_filename);
    buf = kalloc(512);
    for (;;) {
        int lg = sys_read(fd, buf, 512);
        if (lg <= 0)
            break;
        buf[lg] = '\0';
        kprintf(-1, "CONTENT (%x) \n%s\n", lg, buf);
    }
    sys_close(fd);
    kfree(buf);

    sys_sleep(1000000);
    mspace_display(kMMU.kspace);

    // pp = vfs_inode(1, FL_PIPE, NULL);
    sys_pipe(pp);

    buf = kalloc(32);
    int idx = 0;
    for (;;) {
        kprintf(-1, "Master >\n");
        // idx = vfs_read(pp, &buf[idx], 10 - idx, 0, 0);
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

    slog = tty_create(128);
    kprintf(KLOG_MSG, "\n\033[94m  Greetings...\033[0m\n\n");


    vfs_init();
    platform_setup();
    assert(kCPU.irq_semaphore == 1);

    kmod_init();
    kernel_tasklet(kmod_loader, NULL, "Kernel loader #1");
    kernel_tasklet(kmod_loader, NULL, "Kernel loader #2");
    kernel_tasklet(kernel_master, NULL, "Master");

    // no_dbg = 0;
    // int clock_irq = 2;
    clock_init();
    irq_register(0, (irq_handler_t)sys_ticks, NULL);
    // irq_register(2, (irq_handler_t)sys_ticks, NULL);
    // PS2_reset();
    assert(kCPU.irq_semaphore == 1);
    irq_reset(false);
}

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
