#include "ps2.h"

inode_t *mouse_ino;
inode_t *kdb_ino;

void PS2_event(inode_t *ino, uint8_t type, uint32_t param1, uint16_t param2)
{
    if (seat_event(type, param1, param2) != 0) {
        return;
    }

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

    kdb_ino = vfs_inode(1, S_IFCHR, 0);
    kdb_ino->block = sizeof(event_t);
    vfs_mkdev("ps2_kbd", kdb_ino, NULL, "PS2/Keyboard", NULL, NULL);

    mouse_ino = vfs_inode(2, S_IFCHR, 0);
    mouse_ino->block = sizeof(event_t);
    vfs_mkdev("ps2_mouse", mouse_ino, NULL, "PS2/Mouse", NULL, NULL);
    return 0;
}
