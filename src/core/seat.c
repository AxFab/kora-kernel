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
    if ((state & KDB_CAPS_LOCK) != (state & KDB_SHIFT)) {
        r += 1;
    }
    // if (state & KDB_ALTGR)
    //   r += 2;
    if (kcode > 0x54) {
        return 0;
    }
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
    } else if (type == EV_KEY_RELEASE) {
        kUSR.key_last = 0;
    }

    if (type == EV_KEY_PRESS || type == EV_KEY_RELEASE || type == EV_KEY_REPEAT) {
        param1 = seat_key_unicode(param2 & 0xFF, param2 >> 16);
    }

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

void seat_ticks()
{
    /* Keyboard repeat */
    if (kUSR.key_last == 0) {
        return;
    }
    if (kUSR.key_ticks > 0) {
        kUSR.key_ticks--;
        return;
    }

    kUSR.key_ticks = 0; // TODO Config !
    seat_event(EV_KEY_REPEAT, 0, kUSR.key_last);
}

typedef struct framebuffer framebuffer_t;
struct framebuffer {
    unsigned width;
    unsigned height;
    unsigned flags;
    void *physic;
    uint8_t *pixels;
    uint8_t depth;
};

framebuffer_t __fbs[4];

void seat_init_framebuffer(int no, int width, int height, void *pixels, uint8_t depth)
{
    kprintf(0, "[SEAT] Initialize frame buffer %d: %dx%d \n", no, width, height);
    __fbs[no].width = width;
    __fbs[no].height = height;
    __fbs[no].physic = pixels;
    __fbs[no].pixels = NULL;
    __fbs[no].depth = depth;
}

void seat_fb_clear(int no, uint32_t color)
{
    if (__fbs[no].physic == NULL)
        return;
    unsigned d = __fbs[no].depth / 8;
    unsigned W = __fbs[no].width, H = __fbs[no].height;
    if (__fbs[no].pixels == NULL) {
        unsigned size = ALIGN_UP(W * H * d, PAGE_SIZE);
        kprintf(-1, "[SEAT] Screen size: %dx%d -> %s\n", W, H, sztoa(size),
                ALIGN_UP(size, PAGE_SIZE));
        __fbs[no].pixels = kmap(size, NULL, (off_t)__fbs[no].physic,
                                VMA_FG_PHYS | VMA_RESOLVE);
    }
    unsigned i;
    for (i = 0 ; i < W * H; i++) {
        __fbs[no].pixels[i * d + 0] = color & 0xFF;
        __fbs[no].pixels[i * d + 1] = (color >> 8) & 0xFF;
        __fbs[no].pixels[i * d + 2] = (color >> 16) & 0xFF;
    }
}

void seat_fb_splash(int no, unsigned width, unsigned height, uint32_t *colors,
                    uint8_t *pixels)
{
    if (__fbs[no].physic == NULL)
        return;
    unsigned i, j;
    unsigned d = __fbs[no].depth / 8;
    unsigned W = __fbs[no].width;
    unsigned H = __fbs[no].height;
    unsigned rw = (W - width) / 2;
    unsigned rh = (H - height) / 2;
    for (i = 0 ; i < height; i++) {
        for (j = 0; j < width; ++j) {
            uint32_t color = colors[pixels[i * width + j]];
            unsigned k = (i + rh) * W + j + rw;
            __fbs[no].pixels[k * d + 0] = color & 0xFF;
            __fbs[no].pixels[k * d + 1] = (color >> 8) & 0xFF;
            __fbs[no].pixels[k * d + 2] = (color >> 16) & 0xFF;
        }
    }
}

