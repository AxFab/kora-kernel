#include <time.h>
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/memory.h>
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
int ISO9660_setup();



extern uint32_t splash_width;
extern uint32_t splash_height;
extern uint8_t splash_pixels[];
extern uint32_t splash_colors[];

void seat_fb_clear(int no, uint32_t color);
void seat_fb_splash(int no, int width, int height, uint32_t *colors,
                    uint8_t *pixels);


#include <kernel/sys/device.h>
void ARP_who_is(const unsigned char *ip);
inode_t *ISO9660_mount(inode_t *dev);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct kSys kSYS;
struct kCpu kCPU0;

void kernel_start()
{
    kprintf(-1, "Kora OS - x86 - v1.0-4b825dc+"/* _VTAG_*/"\nBuild the " __DATE__
            ".\n");
    kprintf (-1, "\n\e[94m  Greetings...\e[0m\n\n");

    page_initialize();

    kSYS.cpus[0] = &kCPU0;
    irq_reset(false);
    cpu_awake();
    irq_reset(false);

    time_t now = cpu_time();
    kprintf(-1, "[TIME] Date is %s", asctime(gmtime(&now)));

    seat_fb_clear(0, 0xd8d8d8);
    seat_fb_splash(0, splash_width, splash_height, splash_colors, splash_pixels);

    /* Drivers startup */
    TMPFS_setup();
    DEVFS_setup();
    if (true) {
        ATA_setup();
        PCI_setup();
        // E1000_setup();
        PS2_setup();
        // VBE_setup();
    } else {
        // IMG_setup();
    }
    ISO9660_setup();


    inode_t *root = vfs_mount_as_root("sdC", "iso9660");
    if (root == NULL) {
        kprintf(-1, "Expected mount point named 'ISOIMAGE' over 'sdC' !\n");
        return;
    }

    vfs_mount(root, "dev", NULL, "devfs");
    vfs_mount(root, "tmp", NULL, "tmpfs");

    inode_t *ino = vfs_search(root, root, "BIN/INIT");
    if (ino == NULL) {
        kprintf(-1, "Expected file 'INIT'.\n");
        return;
    }

    task_t *task0 = task_create(NULL, root, TSK_USER_SPACE);
    if (elf_open(task0, ino) != 0) {
        kprintf(-1, "Unable to execute file\n");
    }

    vfs_close(root);
    vfs_close(ino);

    irq_register(0, (irq_handler_t)scheduler_ticks, 0);
}


void kernel_ready()
{
    for (;;);
}

