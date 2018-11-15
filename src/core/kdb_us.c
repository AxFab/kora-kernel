
#include <kernel/core.h>
#include <kernel/input.h>
#include <kora/mcrs.h>

#define K2(n) { n, n }

int keyboard_layout_US[][2] = {
    K2(0), // 0x00
    K2(0), // 0x01 - Escape
    { '1', '!' }, // 0x02
    { '2', '@' }, // 0x03
    { '3', '#' }, // 0x04
    { '4', '$' }, // 0x05
    { '5', '%' }, // 0x06
    { '6', '^' }, // 0x07
    { '7', '&' }, // 0x08
    { '8', '*' }, // 0x09
    { '9', '(' }, // 0x0A
    { '0', ')' }, // 0x0B
    { '-', '_' }, // 0x0C
    { '=', '+' }, // 0x0D
    K2(0), // 0x0E - Backspace
    K2(0), // 0x0F - Tab
    { 'q', 'Q' }, // 0x10
    { 'w', 'W' }, // 0x11
    { 'e', 'E' }, // 0x12
    { 'r', 'R' }, // 0x13
    { 't', 'T' }, // 0x14
    { 'y', 'Y' }, // 0x15
    { 'u', 'U' }, // 0x16
    { 'i', 'I' }, // 0x17
    { 'o', 'O' }, // 0x18
    { 'p', 'P' }, // 0x19
    { '[', '{' }, // 0x1A
    { ']', '}' }, // 0x1B
    K2('\n'), // 0x1C - Enter
    K2(0), // 0x1D - Left Ctrl
    { 'a', 'A' }, // 0x1E
    { 's', 'S' }, // 0x1F
    { 'd', 'D' }, // 0x20
    { 'f', 'F' }, // 0x21
    { 'g', 'G' }, // 0x22
    { 'h', 'H' }, // 0x23
    { 'j', 'J' }, // 0x24
    { 'k', 'K' }, // 0x25
    { 'l', 'L' }, // 0x26
    { ';', ':' }, // 0x27
    { '\'', 'A' }, // 0x28
    { '`', '~' }, // 0x29
    K2(0), // 0x2A - Left Shift
    { '\\', '|' }, // 0x2B
    { 'z', 'Z' }, // 0x2C
    { 'x', 'X' }, // 0x2D
    { 'c', 'C' }, // 0x2E
    { 'v', 'V' }, // 0x2F
    { 'b', 'B' }, // 0x30
    { 'n', 'N' }, // 0x31
    { 'm', 'M' }, // 0x32
    { ',', '<' }, // 0x33
    { '.', '>' }, // 0x34
    { '/', '?' }, // 0x35
    K2(0), // 0x36 - Right Shift
    K2(0), // 0x37 - KPASTERISK
    K2(0), // 0x38 - Left Alt
    K2(' '), // 0x39 - Space
    K2(0), // 0x3A - Caps Lock
    K2(0), // 0x3B - F1
    K2(0), // 0x3C - F2
    K2(0), // 0x3D - F3
    K2(0), // 0x3E - F4
    K2(0), // 0x3F - F5
    K2(0), // 0x40 - F6
    K2(0), // 0x41 - F7
    K2(0), // 0x42 - F8
    K2(0), // 0x43 - F9
    K2(0), // 0x44 - F10
    K2(0), // 0x45 - Num Lock
    K2(0), // 0x46 - Scroll Lock
    { '7', 0 }, // 0x47 - KeyPad7 / Home
    { '8', 0 }, // 0x48 - KeyPad8 / ArrowUp
    { '9', 0 }, // 0x49 - KeyPad9 / PgUp
    K2('-'), // 0x4A - KeyPad Minus
    { '4', 0 }, // 0x4B - KeyPad4 / ArrowLeft
    { '5', 0 }, // 0x4C - KeyPad
    { '6', 0 }, // 0x4D - KeyPad6 / ArrowRight
    K2('+'), // 0x4E - KeyPad Plus
    { '1', 0 }, // 0x4F - KeyPad4 / ArrowLeft
    { '2', 0 }, // 0x50 - KeyPad
    { '3', 0 }, // 0x51 - KeyPad6 / ArrowRight
    { '6', 0 }, // 0x52 - KeyPad6 / ArrowRight
    K2('.'), // 0x53 - KeyPad Dot
    K2(0), // !?
};

int seat_key_unicode(int kcode, int state)
{
    int r = 0;
    if ((state & KDB_CAPS_LOCK) != (state & KDB_SHIFT))
        r += 1;
    // if (state & KDB_ALTGR)
    //   r += 2;
    if (kcode > 0x54)
        return 0;
    return keyboard_layout_US[kcode][r];
}

