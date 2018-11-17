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
#include <kora/iofile.h>
#include <kora/llist.h>
#include <string.h>

KMODULE(ide_ata);
KMODULE(pci);
KMODULE(e1000);
KMODULE(vbox);
KMODULE(ac97);
KMODULE(ps2);
KMODULE(vga);
KMODULE(imgdk);
KMODULE(isofs);

// void ARP_who_is(const unsigned char *ip);
void clock_init();
void sys_ticks();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

extern const font_t font_6x10;
extern const font_t font_8x15;
extern const font_t font_7x13;
extern const font_t font_6x9;
extern const font_t font_8x8;


uint32_t colors_std[] = {
    0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0x808080, 0xFFFFFF,
    0xD0D0D0, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF, 0xFFFFFF,
};

uint32_t colors_kora[] = {
    0x181818, 0xA61010, 0x10A610, 0xA66010, 0x1010A6, 0xA610A6, 0x10A6A6, 0xA6A6A6, 0xFFFFFF,
    0x323232, 0xD01010, 0x10D010, 0xD06010, 0x1010D0, 0xD010D0, 0x10D0D0, 0xD0D0D0, 0xFFFFFF,
};

tty_t *tty_syslog = NULL;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


struct kSys kSYS;
struct kCpu kCPU0;


llhead_t modules = INIT_LLHEAD;

int sys_exit(pid_t pid, int ret)
{
    task_t *task = pid == 0 ? kCPU.running : task_search(pid);
    if (task == NULL)
        return -1;
    task_stop(task, ret);
    return 0;
}


void kernel_tasklet(void *start, void *arg, CSTR name)
{
    task_t *task = task_create(NULL, NULL, 0, name);
    task_start(task, start, arg);
}

extern int no_dbg;

void kernel_top(long sec)
{
    async_wait(NULL, NULL, 10000);
    for (;;) {
        async_wait(NULL, NULL, sec * 1000000);
        task_show_all();
        memory_info();
    }
}

volatile inode_t *pp;

void tty_start()
{
    inode_t *dev;
    for (;;) {
        dev = vfs_search_device("kdb");
        if (dev != NULL)
            break;
        async_wait(NULL, NULL, 10000);
    }
    kprintf(-1, "Tty found keyboard\n");

    while (pp == NULL)
        async_wait(NULL, NULL, 10000);

    event_t event;
    for (;;) {
        vfs_read(dev, &event, sizeof(event), 0, 0);
        if (event.type == 3) {
            char c = seat_key_unicode(event.param2 & 0xFFF, event.param2 >> 16);
            kprintf(-1, "Tty key %x -> %x\n", event.param2 & 0xFFF, c);
            if ((c >= ' ' && c < 0x80) || c == '\n')
                vfs_write(pp, &c, 1, 0, 0);
        }
    }
}

int x = 0;
int y = 0;
int width, height;

surface_t *create_win()
{
    surface_t *win;
    if (width > height) {
        win = vds_create(width / 2 - 2, height - 2, 4);
        win->x = x + 1;
        win->y = y + 1;
        x += width / 2;
        width /= 2;
    } else {
        win = vds_create(width - 2, height / 2 - 2, 4);
        win->x = x + 1;
        win->y = y + 1;
        y += height / 2;
        height /= 2;
    }
    return win;
}

void desktop ()
{
    inode_t *dev;
    for (;;) {
        dev = vfs_search_device("fb0");
        if (dev != NULL)
            break;
        async_wait(NULL, NULL, 10000);
    }
    kprintf(-1, "We have a screen !\n");

    surface_t *screen = (surface_t*)dev->info;
    width = screen->width;
    height = screen->height;

    kprintf(-1, "Desktop %dx%d\n", width, height);


    // create window
    surface_t *win1 = create_win();
    surface_t *win2 = create_win();
    surface_t *win3 = create_win();
    surface_t *win4 = create_win();
    surface_t *win5 = create_win();

    for (;;) {

        vds_fill(screen, 0xe2e2e2);
        vds_fill(win1, 0x10a6a6);
        vds_fill(win2, 0xa610a6);
        vds_fill(win3, 0x1010a6);
        vds_fill(win4, 0xa61010);
        vds_fill(win5, 0xa6a610);
        vds_copy(screen, win1, win1->x, win1->y);
        vds_copy(screen, win2, win2->x, win2->y);
        vds_copy(screen, win3, win3->x, win3->y);
        vds_copy(screen, win4, win4->x, win4->y);
        vds_copy(screen, win5, win5->x, win5->y);
        screen->flip(screen);
        async_wait(NULL, NULL, 1000000);

    }

}


void kernel_master()
{
    kernel_tasklet(desktop, NULL, "Desktop #1");
    kernel_tasklet(tty_start, NULL, "Syslog Tty");
    kernel_tasklet(kernel_top, (void*)5, "Dbg top 5s");
    async_wait(NULL, NULL, 5000);
    for (;;) {
        inode_t *dev = vfs_search_device("sdC");
        if (dev != NULL)
            break;
        async_wait(NULL, NULL, 10000);
    }
    kprintf(-1, "Master found the device\n");

    // vfs_mount(root, "dev", NULL, "devfs");
    // vfs_mount(root, "tmp", NULL, "tmpfs");

    // Look for home file system
    inode_t *root = vfs_mount("sdC", "isofs");
    if (root == NULL) {
        kprintf(-1, "Expected mount point over 'sdC' !\n");
        sys_exit(0, 0);
    }

    kprintf(-1, "Master mounted root: %p\n", root);
    async_wait(NULL, NULL, 10000);

    inode_t *ino;
    char name[256];
    void *dir_ctx = vfs_opendir(root, NULL);
    while ((ino = vfs_readdir(root, name, dir_ctx)) != NULL) {
        kprintf(-1, " %p  /%s%s\n", ino, name, ino->type == FL_DIR ? "/": "");
        vfs_close(ino);
    }
    vfs_closedir(root, dir_ctx);

    async_wait(NULL, NULL, 10000);

    inode_t *txt = vfs_search(root, root, "boot/grub/grub.cfg", NULL);
    if (txt != NULL) {
        kprintf(-1, "Reading file from CDROM...\n");
        char *buf = kalloc(txt->length + 1);
        vfs_read(txt, buf, txt->length, 0, 0);
        kprintf(-1, "CONTENT (%x) \n%s\n", txt->length, buf);
        vfs_close(txt);
        kfree(buf);
    }

    async_wait(NULL, NULL, 1000000);
    pp = vfs_inode(1, FL_PIPE, NULL);

    char buf[32];
    int idx = 0;
    for (;;) {
        kprintf(-1, "Master >\n");
        idx = vfs_read(pp, &buf[idx], 30 - idx, 0, 0);
        buf[idx] = '\0';
        char *nx = strchr(buf, '\n');
        if (nx != NULL) {
            nx[0] = '\0';
            kprintf(-1, buf, nx - buf, 0, 0);
            strcpy(buf, &nx[1]);
            idx -= nx - buf;
        } else
            idx = 0;

        // async_wait(NULL, NULL, 1000000);
    }

}

long irq_syscall(long no, long a1, long a2, long a3, long a4, long a5)
{
    kprintf(-1, "Syscall\n");
    return -1;
}


splock_t kmod_lock;
llhead_t kmod_standby;
llhead_t kmod_started;

void kmod_register(kmod_t *mod)
{
    splock_lock(&kmod_lock);
    ll_enqueue(&kmod_standby, &mod->node);
    splock_unlock(&kmod_lock);
}

void kmod_loader()
{
    kmod_t *mod;
    splock_lock(&kmod_lock);
    for (;;) {
        mod = ll_dequeue(&kmod_standby, kmod_t, node);
        splock_unlock(&kmod_lock);
        if (mod == NULL) {
            async_wait(NULL, NULL, 50000);
            splock_lock(&kmod_lock);
            continue;
        }
        kprintf(KLOG_MSG, "Loading driver \033[90m%s\033[0m by \033[90m#%d\033[0m\n", mod->name, kCPU.running->pid);
        mod->setup();
        splock_lock(&kmod_lock);
        ll_enqueue(&kmod_started, &mod->node);
    }
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

    kprintf(KLOG_MSG, "\n\033[94m  Greetings...\033[0m\n\n");

    vfs_init();
    platform_setup();
    assert(kCPU.irq_semaphore == 1);

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
