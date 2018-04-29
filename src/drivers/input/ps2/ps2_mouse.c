#include "ps2.h"

uint8_t mouse_cycle = 0;
char mouse_byte[3];
int mouse_x = 0;
int mouse_y = 0;
uint8_t mouse_btn = 0;

static inline void PS2_mouse_wait_data()
{
    int timeout = 100000;
    while (timeout--) {
        if (inb(0x64) & 1) {
            break;
        }
    }
}

static inline void PS2_mouse_wait_signal()
{
    int timeout = 100000;
    while (timeout--) {
        if (inb(0x64) & 2) {
            break;
        }
    }
}

static inline void PS2_mouse_write(uint8_t a)
{
    //Wait to be able to send a command
    PS2_mouse_wait_signal();
    //Tell the mouse we are sending a command
    outb(0x64, 0xD4);
    //Wait for the final part
    PS2_mouse_wait_signal();
    //Finally write
    outb(0x60, a);
}

static inline uint8_t PS2_mouse_read()
{
    //Get's response from mouse
    PS2_mouse_wait_data();
    return inb(0x60);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

//Mouse functions
void PS2_mouse_handler()
{
    switch (mouse_cycle) {
    case 0:
        mouse_byte[0] = PS2_mouse_read();
        mouse_cycle++;
        break;
    case 1:
        mouse_byte[1] = PS2_mouse_read();
        mouse_cycle++;
        break;
    case 2:
        mouse_byte[2] = PS2_mouse_read();
        mouse_x = mouse_byte[1];
        mouse_y = mouse_byte[2];
        mouse_cycle = 0;

        if (mouse_byte[0] & 0x10) {
            mouse_x = -mouse_x;
        }
        if (mouse_byte[0] & 0x20) {
            mouse_y = -mouse_y;
        }
        if (mouse_x != 0 || mouse_y != 0) {
            PS2_event(mouse_ino, EV_MOUSE_MOTION, mouse_x, mouse_y);
        }

        if (mouse_btn != (mouse_byte[0] & 7)) {
            PS2_event(mouse_ino, EV_MOUSE_BTN, mouse_btn, 0);
            mouse_btn = mouse_byte[0] & 7;
        }

        break;
    }
}

void PS2_mouse_setup()
{
    uint8_t status;

    //Enable the auxiliary mouse device
    PS2_mouse_wait_signal();
    outb(0x64, 0xA8);

    //Enable the interrupts
    PS2_mouse_wait_signal();
    outb(0x64, 0x20);

    status = PS2_mouse_read() | 2;

    PS2_mouse_wait_signal();
    outb(0x64, 0x60);

    PS2_mouse_wait_signal();
    outb(0x60, status);

    //Tell the mouse to use default settings
    PS2_mouse_write(0xF6);
    PS2_mouse_read();  //Acknowledge

    //Enable the mouse
    PS2_mouse_write(0xF4);
    PS2_mouse_read();  //Acknowledge

    // TODO hot-plugin -- When a mouse is plugged into a running system it may send a 0xAA, then a 0x00 byte, and then go into default state (see below).
}
