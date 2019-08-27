#ifndef __GFX_H
#define __GFX_H 1

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <kora/mcrs.h>
#include <kora/keys.h>

#define _16x10(n)  ((n)/10*16)
#define _16x9(n)  ((n)/9*16)

#ifndef PACK
#  ifndef _WIN32
#    define PACK(decl) decl __attribute__((__packed__))
#  else
#    define PACK(decl) __pragma(pack(push,1)) decl __pragma(pack(pop))
#  endif
#endif

typedef int uchar_t;
typedef struct gfx gfx_t;
typedef struct gfx_seat gfx_seat_t;
typedef struct gfx_handlers gfx_handlers_t;
typedef struct gfx_msg gfx_msg_t;
typedef struct font_bmp font_bmp_t;

struct gfx {
    int width;
    int height;
    int pitch;
    long fd;
    long fi;
    union {
        uint8_t *pixels;
        uint32_t *pixels4;
    };
    uint8_t *backup;
};

struct gfx_seat {
    int mouse_x;
    int mouse_y;
    int btn_status; // left, right, wheel, prev, next
    int kdb_status; // Shift, caps, ctrl, alt, home, num, defl !
};

PACK(struct gfx_msg {
    uint16_t message;
    uint16_t window;
    uint32_t param1;
    uint32_t param2;
});

struct font_bmp {
    const uint8_t *glyphs;
    char glyph_size;
    char width;
    char height;
    char dispx;
    char dispy;
};

struct gfx_handlers {
    bool(*repaint)(gfx_t *gfx, void *arg, gfx_seat_t *seat);
    void(*quit)(gfx_t *gfx, void *arg, gfx_seat_t *seat);
    void(*destroy)(gfx_t *gfx, void *arg, gfx_seat_t *seat);
    void(*expose)(gfx_t *gfx, void *arg, gfx_seat_t *seat);
    void(*resize)(gfx_t *gfx, void *arg);
    void(*key_up)(gfx_t *gfx, void *arg, gfx_seat_t *seat, int key);
    void(*key_down)(gfx_t *gfx, void *arg, gfx_seat_t *seat, int key);
    void(*mse_up)(gfx_t *gfx, void *arg, gfx_seat_t *seat, int btn);
    void(*mse_down)(gfx_t *gfx, void *arg, gfx_seat_t *seat, int btn);
    void(*mse_move)(gfx_t *gfx, void *arg, gfx_seat_t *seat);
    void(*mse_wheel)(gfx_t *gfx, void *arg, gfx_seat_t *seat, int disp);
};


LIBAPI void gfx_clear(gfx_t *gfx, uint32_t color);
LIBAPI void gfx_rect(gfx_t *gfx, int x, int y, int w, int h, uint32_t color);
LIBAPI void gfx_glyph(gfx_t *gfx, const font_bmp_t *font, uint32_t unicode, uint32_t fg, uint32_t bg, int x, int y);

LIBAPI gfx_t *gfx_create_window(void *ctx, int width, int height, int flags);

LIBAPI void gfx_destroy(gfx_t *gfx);
LIBAPI int gfx_map(gfx_t *gfx);
LIBAPI int gfx_unmap(gfx_t *gfx);
LIBAPI int gfx_loop(gfx_t *gfx, void *arg, gfx_handlers_t *handlers);

LIBAPI void gfx_flip(gfx_t *gfx);
LIBAPI int gfx_poll(gfx_t *gfx, gfx_msg_t *msg);

LIBAPI void clipboard_copy(const char *buf, int len);
LIBAPI int clipboard_paste(char *buf, int len);

LIBAPI int keyboard_down(int key, int *status, int *key2);
LIBAPI int keyboard_up(int key, int *status);

LIBAPI font_bmp_t* gfx_fetch_font(int idx);


#define EV_QUIT  0
#define EV_MOUSEMOVE  2
#define EV_BUTTONDOWN  3
#define EV_BUTTONUP  4
#define EV_MOUSEWHEEL  5
#define EV_KEYDOWN  6
#define EV_KEYUP  7
#define EV_TIMER  8
#define EV_DELAY  9
#define EV_RESIZE 10



#endif  /* __GFX_H */
