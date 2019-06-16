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
#include <kernel/files.h>
#include <kernel/task.h>
#include <kernel/input.h>
#include <kernel/syscalls.h>
#include <kernel/device.h>

extern font_bmp_t font_7x13;
extern desktop_t *kDESK;


#define _K4(n) {n,n,n,n}
#define _K2(l,u) {l,u,l,u}
#define _KL(n) {(n)|0x20,n,(n)|0x20,n}


int keyboard[128][4] = {

    _K4(0), _K4(0), _K2('1', '!'), _K2('2', '@'),
    _K2('3', '#'), _K2('4', '$'), _K2('5', '%'), _K2('6', '^'),
    _K2('7', '&'), _K2('8', '*'), _K2('9', '('), _K2('0', ')'),
    _K2('-', '_'), _K2('=', '+'), _K4(8), _K4('\t'),

    _KL('Q'), _KL('W'), _KL('E'), _KL('R'),
    _KL('T'), _KL('Y'), _KL('U'), _KL('I'),
    _KL('O'), _KL('P'), _K2('[', '{'), _K2(']', '}'),
    _K4(10), _K4(0), _KL('A'), _KL('S'),

    // 0x20
    _KL('D'), _KL('F'), _KL('G'), _KL('H'),
    _KL('J'), _KL('K'), _KL('L'),  _K2(';', ':'),
    _K2('\'', '"'), _K2('`', '~'), _K4(0), _K2('\\', '|'),
    _KL('Z'), _KL('X'), _KL('C'), _KL('V'),

    // 0x30
    _KL('B'), _KL('N'), _KL('M'), _K2(',', '<'),
    _K2('.', '>'), _K2('/', '?'), _K4(0), _K4('*'),
    _K4(0), _K4(' '), _K4(0), _K4(0),
    _K4(0), _K4(0), _K4(0), _K4(0),
    // 0x40
    _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0),
    _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0),

    _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0),
    _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0),

    // 0x60
    _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0),
    _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0),

    _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0),
    _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0), _K4(0),

};


void tty_start(inode_t *ino)
{
    tty_t *tty = (tty_t *)ino->info;
    inode_t *win = wmgr_create_window(NULL, 640, 420);
    tty_window(tty, win, &font_7x13);

    event_t event;
    for (;;) {
        vfs_read(win, (char *)&event, sizeof(event), 0, 0);
        switch (event.message) {
        case EV_KEYDOWN: {
            int status = event.param2 >> 16;
            int shift = (status & 8 ? 1 : 0);
            if (status & 4)
                shift = 1 - status;
            int unicode = keyboard[event.param1 & 0x0FF][shift];
            // kprintf0(-1, "EV %x) %x %x  [%x] \n", event.type, event.param1, status, unicode);
            if (unicode != 0)
                tty_input(tty, unicode);

        }
        break;
        case EV_RESIZE:
            tty_resize(tty, event.param1, event.param2);
            break;
        }
    }
}

extern tty_t *slog;
void tty_main(inode_t *tty)
{
    vfs_puts(tty, "Hello, secret message\n");
    vfs_puts(tty, "Hello, welcome on Kora Tty\n");

    char buf[50];
    int idx = 0;
    char ch;
    for (;;) {
        int ret = vfs_read(tty, &ch, 1, 0, 0);
        if (ret < 1) {
            kprintf(-1, "Error on TTY, waiting 5 sec \n");
            sys_sleep(SEC_TO_KTIME(5));
        } else if (ch == '\n') {
            buf[idx++] = '\0';
            kprintf(-1, "Exec %s \n", buf);
        } else
            buf[idx++] = ch;
    }
}


void exec_task();
void fake_shell_task();
void main_clock();

void wmgr_render(screen_t *screen);

const char *basename_args[3] = {
    "bin/basename", "-n", "/usr/include/string.h"
};

const char *krish_args[3] = {
    "usr/bin/krish", "-n", "/lib/kernel/ata.km"
};

void wmgr_main()
{
    inode_t *dev;
    for (;;) {
        dev = vfs_search(kSYS.dev_ino, kSYS.dev_ino, "fb0", NULL);
        if (dev != NULL)
            break;
        sys_sleep(MSEC_TO_KTIME(50));
    }

    // We found the screen
    framebuffer_t *fb = (framebuffer_t *)dev->info;
    gfx_clear(fb, RGB(112, 146, 190));
    dev->ops->flip(dev);

    kprintf(-1, "Desktop %dx%d -- #%x\n", fb->width, fb->height, RGB(112, 146, 190));

    desktop_t *desk = kalloc(sizeof(desktop_t));
    desk->ox = desk->oy = DESK_PADDING;
    splock_init(&desk->lock);
    desk->screen = kalloc(sizeof(screen_t));
    desk->screen->sz.w = fb->width;
    desk->screen->sz.h = fb->height;
    desk->screen->frame = fb;
    desk->screen->ino = dev;
    desk->screen->desk = desk;
    desk->pointer_sprites = kalloc(sizeof(framebuffer_t *));
    desk->pointer_sprites[0] = gfx_create(64, 64, 4, NULL);
    // gfx_shadow(desk->pointer_sprites[0], 32, 32, 16, 0xFFa60000);

    // Start applications
    kDESK = desk;
    inode_t *tty0 = tty_inode(slog);
    task_create(tty_start, tty0, "Tty.0");
    inode_t *tty = tty_inode(NULL);
    task_create(tty_start, tty, "Tty.1");
    task_create(tty_main, tty, "Tty.1.prg");
    task_create(exec_task, basename_args, "App basename");
    task_create(exec_task, krish_args, "App Krish");
    task_create(fake_shell_task, NULL, "Fake shell");
    task_create(main_clock, NULL, "Clock");


    // Start render loop
    for (;;) {
        wmgr_render(kDESK->screen);
        sys_sleep(MSEC_TO_KTIME(50));
    }
}

