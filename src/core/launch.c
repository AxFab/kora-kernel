#include <time.h>
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/memory.h>
#include <kernel/input.h>
#include <kernel/task.h>
#include <kernel/cpu.h>
#include <kernel/sys/inode.h>
#include <kernel/sys/device.h>

int TMPFS_setup();
int DEVFS_setup();
int ATA_setup();
int PCI_setup();
int E1000_setup();
int PS2_setup();
int VBE_setup();
int IMG_setup();
int ISOFS_setup();



extern uint32_t splash_width;
extern uint32_t splash_height;
extern uint8_t splash_pixels[];
extern uint32_t splash_colors[];

// void ARP_who_is(const unsigned char *ip);
void sys_ticks();

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct kSys kSYS;
struct kCpu kCPU0;

void kernel_start()
{
    kprintf(-1, "Kora OS - x86 - v1.0-" _VTAG_ "\nBuild the " __DATE__
            ".\n");
    kprintf (-1, "\n\e[94m  Greetings...\e[0m\n\n");

    page_initialize();

    kSYS.cpus[0] = &kCPU0;
    irq_reset(false);
    cpu_awake();
    irq_reset(false);

    time_t now = cpu_time();
    kprintf(-1, "[TIME] Date is %s", asctime(gmtime(&now)));

    seat_initscreen(); // Return an inode to close !
    surface_fill_rect(seat_screen(0), 0, 0, 65000, 65000, 0xd8d8d8);
    surface_draw(seat_screen(0), splash_width, splash_height, splash_colors, splash_pixels);

    seat_surface(120, 120, 4, 0, 0);

    /* Drivers startup */
    TMPFS_setup();
    DEVFS_setup();
#if 1
    ATA_setup();
    PCI_setup();
    // E1000_setup();
    PS2_setup();
    // VBE_setup();
#else
    IMG_setup();
#endif
    ISOFS_setup();


    inode_t *root = vfs_mount_as_root("sdC", "isofs");
    if (root == NULL) {
        kprintf(-1, "Expected mount point named 'ISOIMAGE' over 'sdC' !\n");
        return;
    }

    vfs_mount(root, "dev", NULL, "devfs");
    vfs_mount(root, "tmp", NULL, "tmpfs");

    inode_t *ino = vfs_search(root, root, "bin/init");
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
    irq_register(0, (irq_handler_t)sys_ticks, NULL);
}


void kernel_ready()
{
    for (;;);
}

