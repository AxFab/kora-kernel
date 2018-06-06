/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#include <kernel/cpu.h>
#include <kernel/device.h>
#include <string.h>
#include <errno.h>

short * const csl_screen = (short *)0x000B8000;
const int csl_width = 80;
const int csl_height = 25;
int csl_cursor = 0;
short csl_brush = 0x0700;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void csl_setcursor(int row, int col)
{
    uint16_t pos = row * csl_width + col;
    outb(0x3d4, 0x0f);
    outb(0x3d5, (uint8_t)pos);
    outb(0x3d4, 0x0e);
    outb(0x3d5, (uint8_t)(pos >> 8));
}

void csl_eol()
{
    csl_cursor += csl_width - (csl_cursor % csl_width);
    if (csl_cursor > (csl_height - 2) * csl_width) {
        memcpy(csl_screen, &csl_screen[csl_width], 2 * csl_width * (csl_height - 1));
        memset(&csl_screen[csl_width * (csl_height - 1)], 0, csl_width * 2);
        csl_cursor -= csl_width;
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void csl_early_init()
{
    memset(csl_screen, 0, csl_width * csl_height * 2);
    csl_cursor = 0;
    csl_setcursor(csl_height, 0);
    csl_brush = 0x0700;
}

int csl_write(inode_t *ino, const char *buf, int len)
{
}

int csl_ioctl(inode_t *ino, int cmd, void *params)
{
    errno = 0;
    switch (cmd) {
    // case CSL_RESET:
    //     csl_early_init();
    //     return 0;
    default:
        errno = ENOSYS;
        return -1;
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

chardev_t csl_dev = {
    .dev = {
        .ioctl = (dev_ioctl)csl_ioctl,
    },
    .class = "VBA text console",
    .write = (chr_write)csl_write,
};

void csl_setup()
{
    inode_t *ino = vfs_inode(0, S_IFCHR | 0200, NULL, 0);
    vfs_mkdev("csl", &csl_dev.dev, ino);
}

void csl_teardown()
{
}

MODULE(csl, csl_setup, csl_teardown);

