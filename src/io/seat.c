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
#include <kernel/core.h>
#include <kernel/input.h>
#include <kernel/vfs.h>
#include <kernel/sys/window.h>
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
    K2(0), // 0x1C - Enter
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

    // #define KEY_ZENKAKUHANKAKU  85
    // #define KEY_102ND   86
    // #define KEY_F11     87
    // #define KEY_F12     88
    // #define KEY_RO      89
    // #define KEY_KATAKANA    90
    // #define KEY_HIRAGANA    91
    // #define KEY_HENKAN    92
    // #define KEY_KATAKANAHIRAGANA  93
    // #define KEY_MUHENKAN    94
    // #define KEY_KPJPCOMMA   95
    // #define KEY_KPENTER   96
    // #define KEY_RIGHTCTRL   97
    // #define KEY_KPSLASH   98
    // #define KEY_SYSRQ   99
    // #define KEY_RIGHTALT    100
    // #define KEY_LINEFEED    101
    // #define KEY_HOME    102
    // #define KEY_UP      103
    // #define KEY_PAGEUP    104
    // #define KEY_LEFT    105
    // #define KEY_RIGHT   106
    // #define KEY_END     107
    // #define KEY_DOWN    108
    // #define KEY_PAGEDOWN    109
    // #define KEY_INSERT    110
    // #define KEY_DELETE    111
    // #define KEY_MACRO   112
    // #define KEY_MUTE    113
    // #define KEY_VOLUMEDOWN    114
    // #define KEY_VOLUMEUP    115
    // #define KEY_POWER   116 /* SC System Power Down */
    // #define KEY_KPEQUAL   117
    // #define KEY_KPPLUSMINUS   118
    // #define KEY_PAUSE   119
    // #define KEY_SCALE   120 /* AL Compiz Scale (Expose) */

    // #define KEY_KPCOMMA   121
    // #define KEY_HANGEUL   122
    // #define KEY_HANGUEL   KEY_HANGEUL
    // #define KEY_HANJA   123
    // #define KEY_YEN     124
    // #define KEY_LEFTMETA    125
    // #define KEY_RIGHTMETA   126
    // #define KEY_COMPOSE   127

    // #define KEY_STOP    128 /* AC Stop */
    // #define KEY_AGAIN   129
    // #define KEY_PROPS   130 /* AC Properties */
    // #define KEY_UNDO    131 /* AC Undo */
    // #define KEY_FRONT   132
    // #define KEY_COPY    133 /* AC Copy */
    // #define KEY_OPEN    134 /* AC Open */
    // #define KEY_PASTE   135 /* AC Paste */
    // #define KEY_FIND    136 /* AC Search */
    // #define KEY_CUT     137 /* AC Cut */
    // #define KEY_HELP    138 /* AL Integrated Help Center */
    // #define KEY_MENU    139 /* Menu (show menu) */
    // #define KEY_CALC    140 /* AL Calculator */
    // #define KEY_SETUP   141
    // #define KEY_SLEEP   142 /* SC System Sleep */
    // #define KEY_WAKEUP    143 /* System Wake Up */
    // #define KEY_FILE    144 /* AL Local Machine Browser */
    // #define KEY_SENDFILE    145
    // #define KEY_DELETEFILE    146
    // #define KEY_XFER    147
    // #define KEY_PROG1   148
    // #define KEY_PROG2   149
    // #define KEY_WWW     150 /* AL Internet Browser */
    // #define KEY_MSDOS   151
    // #define KEY_COFFEE    152 /* AL Terminal Lock/Screensaver */
    // #define KEY_SCREENLOCK    KEY_COFFEE
    // #define KEY_ROTATE_DISPLAY  153 /* Display orientation for e.g. tablets */
    // #define KEY_DIRECTION   KEY_ROTATE_DISPLAY
    // #define KEY_CYCLEWINDOWS  154
    // #define KEY_MAIL    155
    // #define KEY_BOOKMARKS   156 /* AC Bookmarks */
    // #define KEY_COMPUTER    157
    // #define KEY_BACK    158 /* AC Back */
    // #define KEY_FORWARD   159 /* AC Forward */
    // #define KEY_CLOSECD   160
    // #define KEY_EJECTCD   161
    // #define KEY_EJECTCLOSECD  162
    // #define KEY_NEXTSONG    163
    // #define KEY_PLAYPAUSE   164
    // #define KEY_PREVIOUSSONG  165
    // #define KEY_STOPCD    166
    // #define KEY_RECORD    167
    // #define KEY_REWIND    168
    // #define KEY_PHONE   169 /* Media Select Telephone */
    // #define KEY_ISO     170
    // #define KEY_CONFIG    171 /* AL Consumer Control Configuration */
    // #define KEY_HOMEPAGE    172 /* AC Home */
    // #define KEY_REFRESH   173 /* AC Refresh */
    // #define KEY_EXIT    174 /* AC Exit */
    // #define KEY_MOVE    175
    // #define KEY_EDIT    176
    // #define KEY_SCROLLUP    177
    // #define KEY_SCROLLDOWN    178
    // #define KEY_KPLEFTPAREN   179
    // #define KEY_KPRIGHTPAREN  180
    // #define KEY_NEW     181 /* AC New */
    // #define KEY_REDO    182 /* AC Redo/Repeat */

    // #define KEY_F13     183
    // #define KEY_F14     184
    // #define KEY_F15     185
    // #define KEY_F16     186
    // #define KEY_F17     187
    // #define KEY_F18     188
    // #define KEY_F19     189
    // #define KEY_F20     190
    // #define KEY_F21     191
    // #define KEY_F22     192
    // #define KEY_F23     193
    // #define KEY_F24     194

    // #define KEY_PLAYCD    200
    // #define KEY_PAUSECD   201
    // #define KEY_PROG3   202
    // #define KEY_PROG4   203
    // #define KEY_DASHBOARD   204 /* AL Dashboard */
    // #define KEY_SUSPEND   205
    // #define KEY_CLOSE   206 /* AC Close */
    // #define KEY_PLAY    207
    // #define KEY_FASTFORWARD   208
    // #define KEY_BASSBOOST   209
    // #define KEY_PRINT   210 /* AC Print */
    // #define KEY_HP      211
    // #define KEY_CAMERA    212
    // #define KEY_SOUND   213
    // #define KEY_QUESTION    214
    // #define KEY_EMAIL   215
    // #define KEY_CHAT    216
    // #define KEY_SEARCH    217
    // #define KEY_CONNECT   218
    // #define KEY_FINANCE   219 /* AL Checkbook/Finance */
    // #define KEY_SPORT   220
    // #define KEY_SHOP    221
    // #define KEY_ALTERASE    222
    // #define KEY_CANCEL    223 /* AC Cancel */
    // #define KEY_BRIGHTNESSDOWN  224
    // #define KEY_BRIGHTNESSUP  225
    // #define KEY_MEDIA   226

    // #define KEY_SWITCHVIDEOMODE 227 /* Cycle between available video
    //              outputs (Monitor/LCD/TV-out/etc) */
    // #define KEY_KBDILLUMTOGGLE  228
    // #define KEY_KBDILLUMDOWN  229
    // #define KEY_KBDILLUMUP    230

    // #define KEY_SEND    231 /* AC Send */
    // #define KEY_REPLY   232 /* AC Reply */
    // #define KEY_FORWARDMAIL   233 /* AC Forward Msg */
    // #define KEY_SAVE    234 /* AC Save */
    // #define KEY_DOCUMENTS   235

    // #define KEY_BATTERY   236

    // #define KEY_BLUETOOTH   237
    // #define KEY_WLAN    238
    // #define KEY_UWB     239

    // #define KEY_UNKNOWN   240

    // #define KEY_VIDEO_NEXT    241 /* drive next video source */
    // #define KEY_VIDEO_PREV    242 /* drive previous video source */
    // #define KEY_BRIGHTNESS_CYCLE  243 /* brightness up, after max is min */
    // #define KEY_BRIGHTNESS_AUTO 244 /* Set Auto Brightness: manual
    //             brightness control is off,
    //             rely on ambient */
    // #define KEY_BRIGHTNESS_ZERO KEY_BRIGHTNESS_AUTO
    // #define KEY_DISPLAY_OFF   245 /* display device to off state */

    // #define KEY_WWAN    246 /* Wireless WAN (LTE, UMTS, GSM, etc.) */
    // #define KEY_WIMAX   KEY_WWAN
    // #define KEY_RFKILL    247 /* Key that controls all radios */

    // #define KEY_MICMUTE   248 /* Mute / unmute the microphone */
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/**
 * Return the unicode key of a keyboard scancode according to the keyboard
 * layout selected by the current user.
 *
 * @kcode  keyboard scancode
 * @state  The state flags of the keyboard (cf. Keyboard state flags)
 */
static int seat_key_unicode(int kcode, int state)
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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct kUsr kUSR;

/**
 * Trigger an event for the current user application. If the application filter
 * the event, the function return -1, then the driver can write the event on
 * it's own char device.
 */
int seat_event(uint8_t type, uint32_t param1, uint16_t param2)
{
    if (type == EV_KEY_PRESS) {
        kUSR.key_last = param2;
        kUSR.key_ticks = 5; // TODO Config !
    } else if (type == EV_KEY_RELEASE)
        kUSR.key_last = 0;

    if (type == EV_KEY_PRESS || type == EV_KEY_RELEASE || type == EV_KEY_REPEAT)
        param1 = seat_key_unicode(param2 & 0xFF, param2 >> 16);

    // ...
    event_t ev;
    // ev.time = time(NULL);
    ev.type = type;
    ev.param1 = param1;
    ev.param2 = param2;
    (void)ev;
    // dev_char_write(input_ino, &ev, sizeof(ev));
    return 0;
}

// void seat_ticks()
// {
//     /* Keyboard repeat */
//     if (kUSR.key_last == 0) {
//         return;
//     }
//     if (kUSR.key_ticks > 0) {
//         kUSR.key_ticks--;
//         return;
//     }

//     kUSR.key_ticks = 0; // TODO Config !
//     seat_event(EV_KEY_REPEAT, 0, kUSR.key_last);

// }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void surface_copy_RGBA_TO_RGB(surface_t *dest, surface_t *src, int relx,
                                     int rely)
{
    int left = MAX(0, relx);
    int top = MAX(0, rely);
    int right = MIN(dest->width, relx + src->width);
    int bottom = MIN(dest->height, rely + src->height);

    int x, y, s, t;
    for (y = top; y < bottom; ++y) {
        s = y * dest->pitch;
        t = (y - rely) * src->pitch;
        for (x = left; x < right; ++x) {
            dest->pixels[s + x * 3 + 0] = src->pixels[t + (x - relx) * 4 + 0];
            dest->pixels[s + x * 3 + 1] = src->pixels[t + (x - relx) * 4 + 1];
            dest->pixels[s + x * 3 + 2] = src->pixels[t + (x - relx) * 4 + 2];
        }
    }
}

static void surface_copy_RGBA_TO_RGBA(surface_t *dest, surface_t *src, int relx,
                                      int rely)
{
    int left = MAX(0, relx);
    int top = MAX(0, rely);
    int right = MIN(dest->width, relx + src->width);
    int bottom = MIN(dest->height, rely + src->height);

    int x, y, s, t;
    for (y = top; y < bottom; ++y) {
        s = y * dest->pitch;
        t = (y - rely) * src->pitch;
        for (x = left; x < right; ++x) {
            dest->pixels[s + x * 4 + 0] = src->pixels[t + (x - relx) * 4 + 0];
            dest->pixels[s + x * 4 + 1] = src->pixels[t + (x - relx) * 4 + 1];
            dest->pixels[s + x * 4 + 2] = src->pixels[t + (x - relx) * 4 + 2];
            dest->pixels[s + x * 4 + 3] = 0xff;
        }
    }
}

static void surface_copy_RGB_TO_RGBA(surface_t *dest, surface_t *src, int relx,
                                     int rely)
{
    int left = MAX(0, relx);
    int top = MAX(0, rely);
    int right = MIN(dest->width, relx + src->width);
    int bottom = MIN(dest->height, rely + src->height);

    int x, y, s, t;
    for (y = top; y < bottom; ++y) {
        s = y * dest->pitch;
        t = (y - rely) * src->pitch;
        for (x = left; x < right; ++x) {
            dest->pixels[s + x * 4 + 0] = src->pixels[t + (x - relx) * 3 + 2];
            dest->pixels[s + x * 4 + 1] = src->pixels[t + (x - relx) * 3 + 1];
            dest->pixels[s + x * 4 + 2] = src->pixels[t + (x - relx) * 3 + 0];
            dest->pixels[s + x * 4 + 3] = 0xff;
        }
    }
}

static void surface_copy_RGB_TO_RGB(surface_t *dest, surface_t *src, int relx,
                                    int rely)
{
    int left = MAX(0, relx);
    int top = MAX(0, rely);
    int right = MIN(dest->width, relx + src->width);
    int bottom = MIN(dest->height, rely + src->height);

    int x, y, s, t;
    for (y = top; y < bottom; ++y) {
        s = y * dest->pitch;
        t = (y - rely) * src->pitch;
        for (x = left; x < right; ++x) {
            dest->pixels[s + x * 3 + 0] = src->pixels[t + (x - relx) * 3 + 2];
            dest->pixels[s + x * 3 + 1] = src->pixels[t + (x - relx) * 3 + 1];
            dest->pixels[s + x * 3 + 2] = src->pixels[t + (x - relx) * 3 + 0];
        }
    }
}

void surface_copy(surface_t *dest, surface_t *src, int relx, int rely)
{
    if (src->format == 4 && dest->format == 3)
        surface_copy_RGBA_TO_RGB(dest, src, relx, rely);

    else if (src->format == 4 && dest->format == 4)
        surface_copy_RGBA_TO_RGBA(dest, src, relx, rely);

    else if (src->format == 3 && dest->format == 4)
        surface_copy_RGB_TO_RGBA(dest, src, relx, rely);

    else if (src->format == 3 && dest->format == 3)
        surface_copy_RGB_TO_RGB(dest, src, relx, rely);

    else {
        // Not supported
    }
}

static void surface_fill_rect_RGB(surface_t *scr, int left, int top, int width,
                                  int height, uint32_t rgb)
{
    left = MAX(0, left); // CLIP !?
    top = MAX(0, top); // CLIP !?
    int right = MIN(scr->width, left + width);
    int bottom = MIN(scr->height, top + height);

    int x, y, k;
    for (y = top; y < bottom; ++y) {
        k = y * scr->pitch;
        for (x = left; x < right; ++x) {
            scr->pixels[k + x * 3 + 0] = rgb & 0xFF;
            scr->pixels[k + x * 3 + 1] = (rgb >> 8) & 0xFF;
            scr->pixels[k + x * 3 + 2] = (rgb >> 16) & 0xFF;
        }
    }
}

static void surface_fill_rect_RGBA(surface_t *scr, int left, int top, int width,
                                   int height, uint32_t rgb)
{
    left = MAX(0, left); // CLIP !?
    top = MAX(0, top); // CLIP !?
    int right = MIN(scr->width, left + width);
    int bottom = MIN(scr->height, top + height);

    int x, y, k;
    for (y = top; y < bottom; ++y) {
        k = y * scr->pitch;
        for (x = left; x < right; ++x) {
            scr->pixels[k + x * 4 + 0] = rgb & 0xFF;
            scr->pixels[k + x * 4 + 1] = (rgb >> 8) & 0xFF;
            scr->pixels[k + x * 4 + 2] = (rgb >> 16) & 0xFF;
            scr->pixels[k + x * 4 + 3] = 0xff;
        }
    }
}

void surface_fill_rect(surface_t *scr, int x, int y, int width, int height,
                       uint32_t rgb)
{
    switch (scr->format) {
    case 3:
        surface_fill_rect_RGB(scr, x, y, width, height, rgb);
        break;
    case 4:
        surface_fill_rect_RGBA(scr, x, y, width, height, rgb);
        break;
    }
}

void surface_draw(surface_t *scr, int width, int height, uint32_t *color,
                  uint8_t *px)
{
    int relx = (scr->width - width) / 2;
    int rely = (scr->height - height) / 2;

    int left = MAX(0, relx);
    int top = MAX(0, rely);
    int right = MIN(scr->width, relx + width);
    int bottom = MIN(scr->height, rely + height);

    int x, y, s, d = scr->depth;
    for (y = top; y < bottom; ++y) {
        s = y * scr->pitch;
        for (x = left; x < right; ++x) {
            uint32_t clr = color[px[(y - rely) * width + (x - relx)]];
            scr->pixels[s + x * d + 0] = clr & 0xff;
            scr->pixels[s + x * d + 1] = (clr >> 8) & 0xff;
            scr->pixels[s + x * d + 2] = (clr >> 16) & 0xff;
        }
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
#define MAX_SCREEN  8

llhead_t win_list = INIT_LLHEAD;

static long s_ticks = 0;

surface_t *__fbs[MAX_SCREEN] = { NULL };


void seat_ticks()
{
    if (((s_ticks++) % 20) != 0)   // TODO -- Or no update required
        return;

    // TODO -- Unoptimize redraw !
    surface_t *win;
    surface_t *screen = seat_screen(0);
    if (screen == NULL)
        return;
    kprintf(0, "PAINT\n");
    surface_fill_rect(screen, 0, 0, screen->width, screen->height, 0xd8d8d8);
    for ll_each(&win_list, win, surface_t, node)
        surface_copy(screen, win, win->x, win->y);
}

surface_t *seat_screen(int no)
{
    return no >= 0 && no < MAX_SCREEN ? __fbs[no] : NULL;
}

inode_t *seat_surface(int width, int height, int format, int features,
                      int events)
{
    inode_t *ino = vfs_inode(3, S_IFWIN, 0);
    ino->win->width = width;
    ino->win->height = height;
    ino->win->format = format;
    ino->win->depth = 4;
    ino->win->pitch = ALIGN_UP(width * ino->win->depth, 16);
    // ino->win->full_height = ALIGN_UP(height, 4);
    ino->win->size = (size_t)ino->win->pitch * ino->win->height;
    ino->win->pixels = kmap(ALIGN_UP(ino->win->size, PAGE_SIZE), NULL, 0,
                            VMA_RW | VMA_ANON | VMA_RESOLVE);
    ino->win->features = features;
    ino->win->events = events;
    ino->win->ino = ino;
    ino->block = (size_t)ino->win->pitch;
    ino->length = ino->win->size;
    ll_append(&win_list, &ino->win->node);
    return ino;
}

inode_t *seat_framebuf(int width, int height, int format, int pitch, void *mmio)
{
    inode_t *ino = vfs_inode(3, S_IFWIN, 0);
    ino->win->width = width;
    ino->win->height = height;
    ino->win->format = format;
    ino->win->depth = format; // TODO - FIx this HACK
    ino->win->pitch = pitch;
    // ino->win->full_height = ALIGN_UP(height, 4);
    ino->win->size = (size_t)ino->win->pitch * ino->win->height;
    ino->win->pixels = kmap(ALIGN_UP(ino->win->size, PAGE_SIZE), NULL,
                            (size_t)mmio, VMA_RW | VMA_PHYS | VMA_RESOLVE);
    ino->win->features = 0;
    ino->win->events = 0;
    ino->win->ino = ino;
    ino->block = (size_t)ino->win->pitch;
    ino->length = ino->win->size;
    __fbs[0] = ino->win;
    return ino;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int __fb0_width;
int __fb0_height;
int __fb0_format;
int __fb0_pitch;
void *__fb0_pixels;

void seat_fb0(int width, int height, int format, int pitch, void *mmio)
{
    __fb0_width = width;
    __fb0_height = height;
    __fb0_format = format / 8;
    __fb0_pitch = pitch;
    __fb0_pixels = mmio;
}

inode_t *seat_initscreen()
{
    if (__fb0_pixels == NULL)
        return NULL;
    return seat_framebuf(__fb0_width, __fb0_height, __fb0_format, __fb0_pitch,
                         __fb0_pixels);
}

