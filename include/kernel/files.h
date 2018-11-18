/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2018  <Fabien Bavent>
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
typedef struct tty tty_t;
typedef struct font_bmp font_bmp_t;
typedef struct desktop desktop_t;
typedef struct display display_t;
typedef struct pipe pipe_t;

struct surface {
    int width;
    int height;
    int pitch;
    int depth;
    uint8_t *pixels;
    uint8_t *backup;
    llnode_t node;
    void (*flip)(surface_t *screen);
    int x;
    int y;
};


struct font_bmp {
    uint8_t *glyphs;
    char glyph_size;
    char width, height, dispx, dispy;
};

void font_paint(surface_t *sfc, font_bmp_t *data, uint32_t unicode, uint32_t *color, int x, int y);


#define IO_NO_BLOCK  (1 << 0)
#define IO_ATOMIC  (1 << 1)
#define IO_CONSUME  (1 << 2)



void vds_fill(surface_t *win, uint32_t color);
void vds_copy(surface_t *dest, surface_t *src, int x, int y);
surface_t *vds_create_empty(int width, int height, int depth);
surface_t *vds_create(int width, int height, int depth);
void vds_destroy(surface_t *srf);
void vds_mouse(surface_t *scr, int x, int y);


void wmgr_register_screen(surface_t *screen);
surface_t *wmgr_surface(desktop_t *desktop, int width, int height, int depth);
surface_t *wmgr_window(desktop_t *desktop, int width, int height);
void wmgr_event(event_t *ev);
desktop_t *wmgr_desktop();



pipe_t *pipe_create();
void pipe_destroy(pipe_t *pipe);
int pipe_resize(pipe_t *pipe, int size);
int pipe_erase(pipe_t *pipe, int len);
int pipe_write(pipe_t *pipe, const char *buf, int len, int flags);
int pipe_read(pipe_t *pipe, char *buf, int len, int flags);




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


int elf_parse(dynlib_t *dlib);

#endif /* _KERNEL_FILES_H */
