/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <string.h>

short *TXT_screen = (short *)0x000B8000;
int TXT_width = 80;
int TXT_height = 25;
int TXT_cursor = 0;
short TXT_brush = 0x0700;

static void TXT_ansi_color(int var)
{
    switch (var) {
    case 0:
        TXT_brush = 0x0700;
        break;
    case 5:
        TXT_brush |= 0x8000;
        break;
    case 7:
        TXT_brush = (TXT_brush & 0x8800) | ((TXT_brush & 0x7000) >> 4) | ((
                        TXT_brush & 0x0700) << 4);
        break;
    case 25:
        TXT_brush &= 0x7FFF;
        break;
    case 30:
        TXT_brush = (TXT_brush & 0xF000) | 0x000;
        break;
    case 31:
        TXT_brush = (TXT_brush & 0xF000) | 0x400;
        break;
    case 32:
        TXT_brush = (TXT_brush & 0xF000) | 0x200;
        break;
    case 33:
        TXT_brush = (TXT_brush & 0xF000) | 0x600;
        break;
    case 34:
        TXT_brush = (TXT_brush & 0xF000) | 0x100;
        break;
    case 35:
        TXT_brush = (TXT_brush & 0xF000) | 0x500;
        break;
    case 36:
        TXT_brush = (TXT_brush & 0xF000) | 0x300;
        break;
    case 37:
        TXT_brush = (TXT_brush & 0xF000) | 0x700;
        break;
    case 40:
        TXT_brush = (TXT_brush & 0x8F00) | 0x0000;
        break;
    case 41:
        TXT_brush = (TXT_brush & 0x8F00) | 0x4000;
        break;
    case 42:
        TXT_brush = (TXT_brush & 0x8F00) | 0x2000;
        break;
    case 43:
        TXT_brush = (TXT_brush & 0x8F00) | 0x6000;
        break;
    case 44:
        TXT_brush = (TXT_brush & 0x8F00) | 0x1000;
        break;
    case 45:
        TXT_brush = (TXT_brush & 0x8F00) | 0x5000;
        break;
    case 46:
        TXT_brush = (TXT_brush & 0x8F00) | 0x3000;
        break;
    case 47:
        TXT_brush = (TXT_brush & 0x8F00) | 0x7000;
        break;
    case 90:
        TXT_brush = (TXT_brush & 0xF000) | 0x800;
        break;
    case 91:
        TXT_brush = (TXT_brush & 0xF000) | 0xC00;
        break;
    case 92:
        TXT_brush = (TXT_brush & 0xF000) | 0xA00;
        break;
    case 93:
        TXT_brush = (TXT_brush & 0xF000) | 0xE00;
        break;
    case 94:
        TXT_brush = (TXT_brush & 0xF000) | 0x900;
        break;
    case 95:
        TXT_brush = (TXT_brush & 0xF000) | 0xD00;
        break;
    case 96:
        TXT_brush = (TXT_brush & 0xF000) | 0xB00;
        break;
    case 97:
        TXT_brush = (TXT_brush & 0xF000) | 0xF00;
        break;
    }
}

void TXT_eom()
{
    uint16_t pos = 4096 ; //TXT_cursor + 1;
    outb(0x3d4, 0x0f);
    outb(0x3d5, (uint8_t)pos);
    outb(0x3d4, 0x0e);
    outb(0x3d5, (uint8_t)(pos >> 8));
}

static void TXT_ansi_coord(int row, int col)
{
    TXT_cursor = row * TXT_width + col;
    TXT_eom();
}

int TXT_ansi_escape(const char *msg)
{
    int i;
    int varc = 0;
    int varv[12];
    char *sv = (char *)msg;
    sv += 2;
    for (;;) {
        varv[varc] = strtoul(sv, &sv, 10);
        varc++;
        if (*sv != ';')
            break;

        else
            sv++;
    }

    switch (*sv) {
    case 'm':
        for (i = 0; i < varc; ++i)
            TXT_ansi_color(varv[i]);
        break;
    case 'H':
    case 'f':
        TXT_ansi_coord(varv[0], varv[1]);
        break;
    case 'J': // Erase in display
        if (varv[0] == 1)
            memset(&TXT_screen[TXT_cursor], 0, (TXT_width * TXT_height - TXT_cursor) * 2);

        else if (varv[0] == 2)
            memset(TXT_screen, 0, TXT_cursor * 2);

        else if (varv[0] == 3)
            memset(TXT_screen, 0, TXT_width * TXT_height * 2);
        break;
    default:
        return 0;
    }

    return sv - msg;
}

void TXT_eol()
{
    TXT_cursor += TXT_width - (TXT_cursor % TXT_width);
    if (TXT_cursor > (TXT_height - 2) * TXT_width) {
        memcpy(TXT_screen, &TXT_screen[TXT_width], 2 * TXT_width * (TXT_height - 1));
        memset(&TXT_screen[TXT_width * (TXT_height - 1)], 0, TXT_width * 2);
        TXT_cursor -= TXT_width;
    }
}


void TXT_write(const char *msg, size_t lg)
{
    const char *st = msg;
    for (; msg - st < (int)lg; ++msg) {
        if (*msg == '\r')
            TXT_cursor -= TXT_cursor % TXT_width;

        else if (*msg == '\n')
            TXT_eol();

        else if (*msg == '\t')
            TXT_cursor = (TXT_cursor + 7) & ~7;

        else if (*msg == '\e' && msg[1] == '[')
            msg += TXT_ansi_escape(msg);

        else if (*msg < 0x20) {
            // Ignore
        } else if (*msg < 0x7F)
            TXT_screen[TXT_cursor++] = TXT_brush | *msg;
    }
    TXT_eom();
}


