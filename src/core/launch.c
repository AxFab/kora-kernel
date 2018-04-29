#include <time.h>
#include <kernel/core.h>
#include <kernel/mods/fs.h>
#include <kernel/memory.h>
// #include <kernel/input.h>
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

// extern FILE *klogs;

// FILE slog;
// // terminal_t *term_syslog;
// void syslog_write(FILE *fp, const char *buf, int lg)
// {
//     term_write(term_syslog, buf, lg);
// }

struct kSys kSYS;
struct kCpu kCPU0;


llhead_t modules = INIT_LLHEAD;

void kernel_module(kmod_t *mod)
{
    ll_append(&modules, &mod->node);
    mod->setup();
}

void kernel_start()
{
    kprintf(-1, "KoraOS - " __ARCH " - v" _VTAG_ "\nBuild the " __DATE__ ".\n");
    kprintf (-1, "\n\e[94m  Greetings...\e[0m\n\n");

    page_initialize();

    kSYS.cpus[0] = &kCPU0;
    irq_reset(false);
    cpu_awake();
    irq_reset(false);

    time_t now = cpu_time();
    kprintf(-1, "Startup: %s", asctime(gmtime(&now)));

    // seat_initscreen(); // Return an inode to close !
    // surface_t *src0 = seat_screen(0);
    // if (src0 != NULL)
    //     surface_fill_rect(src0, 0, 0, 65000, 65000, 0xd8d8d8);
        // surface_draw(seat_screen(0), splash_width, splash_height, splash_colors, splash_pixels);
        // seat_surface(120, 120, 4, 0, 0);

    // term_syslog = term_create(seat_screen(0));
    // slog.lbuf_ = EOF;
    // slog.lock_ = -1;
    // slog.write = syslog_write;
    // klogs = &slog;

    /* Drivers startup */
    // kernel_module("tmpfs", TMPFS_setup, TMPFS_teardown);
    // DEVFS_setup();

    bool irq = irq_enable();
    assert(irq);

#if 1
    KSETUP(ps2);
    KSETUP(ide_ata);
    KSETUP(pci);
    KSETUP(e1000);
    KSETUP(vbox);
    KSETUP(ac97);
    KSETUP(vga);
#else
    KSETUP(imgdk);
#endif
    KSETUP(isofs);

    irq_disable();

    inode_t *root = vfs_mount("sdC", "isofs");
    if (root == NULL) {
        kprintf(-1, "Expected mount point named 'ISOIMAGE' over 'sdC' !\n");
        return;
    }

    // vfs_mount(root, "dev", NULL, "devfs");
    // vfs_mount(root, "tmp", NULL, "tmpfs");

    inode_t *ino = vfs_search(root, root, "bin/init", NULL);
    if (ino == NULL) {
        kprintf(-1, "Expected file 'bin/init'.\n");
        return;
    }

    task_t *task0 = task_create(NULL, root, TSK_USER_SPACE);
    if (elf_open(task0, ino) != 0) {
        kprintf(-1, "Unable to execute file\n");
    }

    vfs_close(root);
    vfs_close(ino);

    clock_init();
    irq_register(0, (irq_handler_t)sys_ticks, NULL);
}

void kernel_ready()
{
    for (;;);
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
