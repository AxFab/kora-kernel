#include "ps2.h"

inode_t *mouse_ino;
inode_t *kdb_ino;

device_t ps2_kbd;
device_t ps2_mse;

void PS2_event(inode_t *ino, uint8_t type, uint32_t param1, uint16_t param2)
{
    // if (seat_event(type, param1, param2) != 0) {
    //     return;
    // }

    event_t ev;
    // ev.time = time(NULL);
    ev.type = type;
    ev.param1 = param1;
    ev.param2 = param2;
    (void)ev;
    // dev_char_write(ino, &ev, sizeof(ev));
}

int PS2_irq()
{
    uint8_t r;
    while ((r = inb(0x64)) & 1) {
        if (r & 0x20) {
            PS2_mouse_handler();
        } else {
            PS2_kdb_handler();
        }
    }
    return 0;
}


int PS2_setup()
{
    PS2_mouse_setup();
    irq_register(1, (irq_handler_t)PS2_irq, NULL);
    irq_register(12, (irq_handler_t)PS2_irq, NULL);

    kdb_ino = vfs_inode(1, S_IFCHR | 700, NULL, 0);
    ps2_kbd.block = sizeof(event_t);
    ps2_kbd.class = "PS/2 Keyboard";
    vfs_mkdev("kdb", &ps2_kbd, kdb_ino);
    vfs_close(kdb_ino);

    mouse_ino = vfs_inode(2, S_IFCHR | 700, NULL, 0);
    ps2_mse.block = sizeof(event_t);
    ps2_mse.class = "PS/2 Mouse";
    vfs_mkdev("mise", &ps2_mse, mouse_ino);
    vfs_close(mouse_ino);
    return 0;
}

int PS2_teardown() {
    vfs_rmdev("kdb");
    vfs_rmdev("mise");
    return 0;
}