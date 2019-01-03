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
#include <kernel/cpu.h>
#include <kernel/device.h>
#include <string.h>
#include <mbstring.h>
#include <errno.h>

short *const csl_screen = (short *)0x000B8000;
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
static void csl_autoscroll()
{
    if (csl_cursor > (csl_height - 2) * csl_width) {
        memcpy(csl_screen, &csl_screen[csl_width], 2 * csl_width * (csl_height - 1));
        memset(&csl_screen[csl_width * (csl_height - 1)], 0, csl_width * 2);
        csl_cursor -= csl_width;
    }
}

static void csl_eol()
{
    csl_cursor += csl_width - (csl_cursor % csl_width);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void csl_early_init()
{
    memset(csl_screen, 0, csl_width * csl_height * 2);
    csl_cursor = 0;
    csl_setcursor(csl_height, 0);
    csl_brush = 0x0700;
}

typedef struct ansi_cmd ansi_cmd_t;
struct ansi_cmd {
    int cmd;
    int len;
    int args[6];
};

int ansi_parse(ansi_cmd_t *cmd, const char *str, int len)
{
    char *sv = (char *)str;
    cmd->len = 0;
    cmd->cmd = 0;
    if (len <= 2 || str[0] != '\e' || str[1] != '[')
        return 1;

    sv += 2;
    for (;;) {
        cmd->args[cmd->len] = strtoul(sv, &sv, 10);
        cmd->len++;
        if (*sv != ';')
            break;

        else
            sv++;
    }

    cmd->cmd = *sv;
    sv++;
    return sv - str;
}

void csl_set_color(int value)
{
    switch (value) {
    case 0:
        csl_brush = 0x0700;
        break;
    case 5:
        csl_brush |= 0x8000;
        break;
    case 7:
        csl_brush = (csl_brush & 0x8800) | ((csl_brush & 0x7000) >> 4) | ((
                        csl_brush & 0x0700) << 4);
        break;
    case 25:
        csl_brush &= 0x7FFF;
        break;
    case 30:
        csl_brush = (csl_brush & 0xF000) | 0x000;
        break;
    case 31:
        csl_brush = (csl_brush & 0xF000) | 0x400;
        break;
    case 32:
        csl_brush = (csl_brush & 0xF000) | 0x200;
        break;
    case 33:
        csl_brush = (csl_brush & 0xF000) | 0x600;
        break;
    case 34:
        csl_brush = (csl_brush & 0xF000) | 0x100;
        break;
    case 35:
        csl_brush = (csl_brush & 0xF000) | 0x500;
        break;
    case 36:
        csl_brush = (csl_brush & 0xF000) | 0x300;
        break;
    case 37:
        csl_brush = (csl_brush & 0xF000) | 0x700;
        break;
    case 40:
        csl_brush = (csl_brush & 0x8F00) | 0x0000;
        break;
    case 41:
        csl_brush = (csl_brush & 0x8F00) | 0x4000;
        break;
    case 42:
        csl_brush = (csl_brush & 0x8F00) | 0x2000;
        break;
    case 43:
        csl_brush = (csl_brush & 0x8F00) | 0x6000;
        break;
    case 44:
        csl_brush = (csl_brush & 0x8F00) | 0x1000;
        break;
    case 45:
        csl_brush = (csl_brush & 0x8F00) | 0x5000;
        break;
    case 46:
        csl_brush = (csl_brush & 0x8F00) | 0x3000;
        break;
    case 47:
        csl_brush = (csl_brush & 0x8F00) | 0x7000;
        break;
    case 90:
        csl_brush = (csl_brush & 0xF000) | 0x800;
        break;
    case 91:
        csl_brush = (csl_brush & 0xF000) | 0xC00;
        break;
    case 92:
        csl_brush = (csl_brush & 0xF000) | 0xA00;
        break;
    case 93:
        csl_brush = (csl_brush & 0xF000) | 0xE00;
        break;
    case 94:
        csl_brush = (csl_brush & 0xF000) | 0x900;
        break;
    case 95:
        csl_brush = (csl_brush & 0xF000) | 0xD00;
        break;
    case 96:
        csl_brush = (csl_brush & 0xF000) | 0xB00;
        break;
    case 97:
        csl_brush = (csl_brush & 0xF000) | 0xF00;
        break;
    }
}

void csl_ansi_cmd(ansi_cmd_t *cmd)
{
    int i;
    switch (cmd->cmd) {
    case 'm':
        for (i = 0; i < cmd->len; ++i)
            csl_set_color(cmd->args[i]);
        break;
    case 'H':
    case 'f':
        csl_setcursor(cmd->args[0], cmd->args[1]);
        break;
    default:
        break;
    }
}

int csl_write(inode_t *ino, const char *buf, size_t len, int flags)
{
    int s = len;
    while (len > 0) {
        if (*buf < 0) {
            // This is not ASCII character
            wchar_t wc;
            int lc = MAX(1, mbtowc(&wc, buf, len));
            csl_screen[csl_cursor++] = '?' | csl_brush;
            buf += lc;
            len -= lc;
        } else if (*buf < 0x20) {
            // This is control character
            if (*buf == '\n') {
                csl_eol();
                buf++;
                len--;
            } else if (*buf == '\e') {
                ansi_cmd_t cmd;
                int lc = ansi_parse(&cmd, buf, len);
                csl_ansi_cmd(&cmd);
                buf += lc;
                len -= lc;
            } else {
                buf++;
                len--;
            }
        } else {
            csl_screen[csl_cursor++] = (*buf) | csl_brush;
            buf++;
            len--;
        }
        csl_autoscroll();
    }
    return s;
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

dev_ops_t csl_dops = {
    .ioctl = csl_ioctl,
};

ino_ops_t csl_fops = {

    .write = csl_write,
    .close = NULL
};

void csl_setup()
{
    inode_t *ino = vfs_inode(1, FL_CHR, NULL);
    ino->und.dev->devclass = (char *)"VBA text console";
    ino->und.dev->ops = &csl_dops;
    ino->ops = &csl_fops;
    vfs_mkdev(ino, "csl");
}

void csl_teardown()
{
}

MODULE(csl, csl_setup, csl_teardown);

