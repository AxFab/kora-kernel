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
#ifndef _KERNEL_FILES_H
#define _KERNEL_FILES_H 1

#include <kernel/core.h>
#include <kora/llist.h>




typedef struct map_page map_page_t;
typedef struct map_cache map_cache_t;

typedef struct surface surface_t;
typedef struct line line_t;
typedef struct font_bmp font_bmp_t;
typedef struct display display_t;


struct framebuffer {
    int width, height;
    int pitch, depth;
    uint8_t *pixels;
    uint8_t *backup;
    rwlock_t lock;
};

struct rect {
    int x, y, w, h;
};

struct window {
    int no;
    desktop_t *desk;
    rect_t sz, rq;
    framebuffer_t *frame;
    pipe_t *pipe;
    int queries;
    rwlock_t lock;

    int grid, rq_grid;
    int ml, mt, mr, mb;
    llnode_t node;
    uint32_t color;
};

struct pointer {
    int x, y;
    int size, idx;
};

struct screen {
    int no;
    desktop_t *desk;
    rect_t sz;
    framebuffer_t *frame;
    int queries;
    rwlock_t lock;

    inode_t *ino;
    pointer_t pointer;
    window_t *fullscreen;
};


struct desktop {
    int no, ox, oy;
    pointer_t pointer;
    llhead_t windows;
    splock_t lock;
    screen_t *screen;
    framebuffer_t **pointer_sprites;

    int kbd_status;
    int btn_status;
    int last_key;
    clock64_t last_kbd;

    window_t *over_win;
    window_t *grab_win;
    int grab_type;
    int grab_x;
    int grab_y;
};

struct font_bmp {
    const uint8_t *glyphs;
    char glyph_size;
    char width, height, dispx, dispy;
};

void font_paint(framebuffer_t *fb, const font_bmp_t *data, uint32_t unicode, uint32_t *color, int x, int y);


#define IO_NO_BLOCK  (1 << 4)
#define IO_ATOMIC  (1 << 5)
#define IO_CONSUME  (1 << 6)




framebuffer_t *gfx_create(int width, int height, int depth, void *pixels);
void gfx_rect(framebuffer_t *fb, int x, int y, int w, int h, uint32_t color);
void gfx_clear(framebuffer_t *fb, uint32_t color);
void gfx_slide(framebuffer_t *sfc, int height, uint32_t color);
void gfx_copy(framebuffer_t *dest, framebuffer_t *src, int x, int y, int w, int h);
void gfx_copy_blend(framebuffer_t *dest, framebuffer_t *src, int x, int y);
void gfx_flip(framebuffer_t *fb);
void gfx_resize(framebuffer_t *fb, int w, int h, void *pixels);


void wmgr_input(inode_t *ino, int type, int param, pipe_t *pipe);
inode_t *wmgr_create_window(desktop_t *desk, int width, int height);


pipe_t *pipe_create();
void pipe_destroy(pipe_t *pipe);
int pipe_resize(pipe_t *pipe, size_t size);
int pipe_erase(pipe_t *pipe, size_t len);
int pipe_write(pipe_t *pipe, const char *buf, size_t len, int flags);
int pipe_read(pipe_t *pipe, char *buf, size_t len, int flags);
int pipe_reset(pipe_t *pipe);
inode_t *pipe_inode();



map_cache_t *map_create(inode_t *ino, void *read, void *write);
void map_destroy(map_cache_t *cache);
page_t map_fetch(map_cache_t *cache, off_t off);
void map_sync(map_cache_t *cache, off_t off, page_t pg);
void map_release(map_cache_t *cache, off_t off, page_t pg);



void ioblk_init(inode_t *ino);
void ioblk_sweep(inode_t *ino);
void ioblk_release(inode_t *ino, off_t off);
/* Find the page mapping the content of a block inode */
page_t ioblk_page(inode_t *ino, off_t off);
/* Synchronize a page mapping the content of a block inode */
int ioblk_sync(inode_t *ino, off_t off);
void ioblk_dirty(inode_t *ino, off_t off);
int ioblk_read(inode_t *ino, char *buf, int len, off_t off);
int ioblk_write(inode_t *ino, const char *buf, int len, off_t off);


tty_t *tty_create(int count);
void tty_window(tty_t *tty, inode_t *ino, const font_bmp_t *font);
int tty_write(tty_t *tty, const char *buf, int len);
int tty_puts(tty_t *tty, const char *buf);
inode_t *tty_inode(tty_t *tty);
void tty_input(tty_t *tty, int unicode);
void tty_resize(tty_t *tty, int width, int height);


#define RGB(r,g,b) ((((r) &0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff))

#define DESK_PADDING 10
#define DESK_BORDER 2

int elf_parse(dynlib_t *dlib);

#endif /* _KERNEL_FILES_H */
