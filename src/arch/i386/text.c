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

const short *txt_screen = (short *)0x000B8000;
const int txt_width = 80;
const int txt_height = 25;
int txt_cursor = 0;
short txt_brush = 0x0700;
 
/* -= */

static void txt_setcursor(int row, int col) 
{
    uint16_t pos = row * txt_width + col;
    outb(0x3d4, 0x0f);
    outb(0x3d5, (uint8_t)pos);
    outb(0x3d4, 0x0e);
    outb(0x3d5, (uint8_t)(pos >> 8));
}

void txt_eol()
{
    txt_cursor += txt_width - (txt_cursor % txt_width);
    if (txt_cursor > (txt_height - 2) * txt_width) {
        memcpy(txt_screen, &TXT_screen[txt_width], 2 * txt_width * (txt_height - 1));
        memset(&txt_screen[txt_width * (txt_height - 1)], 0, txt_width * 2);
        txt_cursor -= txt_width;
    }
}

/* -= */

void txt_early_init()
{
	memset(txt_screen, 0, txt_width * txt_height * 2);
	txt_cursor = 0;
	txt_setcursor(txt_height, 0);
	txt_brush = 0x0700;
}

int txt_write(inode_t *ino, const char *buf, int len)
{
}

int txt_ioctl(inode_t *ino, int cmd, void *params)
{
	errno = 0;
	switch (cmd) {
	case CSL_RESET:
	    txt_early_init();
	    return 0;
	default:
	    errno = ENOSYS;
	    return -1;
	}
}


/* -= */

dev_ops_t csl_ops = {
	.write = (dev_write)txt_write,
	.ioctl = (dev_ioctl)txt_ioctl,
};

void txt_setup()
{
	inode_t *ino = vfs_inode(0, S_IFCHR | 0200, NULL, 0);
	vfs_mkdev("csl", ino, NULL, "Text VBA screen", csl_ops);
}

void txt_shutdown()
{
}

MODULE("csl", txt_setup, txt_teardown);
