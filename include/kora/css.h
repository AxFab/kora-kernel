/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#ifndef _KORA_CSS_H
#define _KORA_CSS_H  1

#include <kora/mcrs.h>
#include <stdio.h>

typedef void(*css_setter)(void *, const char *, const char *, const char *);

typedef struct {
    unsigned unit: 4;
    int len: 28;
} css_size_t;

#define CSS_BUF_SIZE 1024

void css_read_file(FILE *stream, void *arg, css_setter setter);
unsigned int css_parse_color(const char *value);
css_size_t css_parse_usize(const char *value);
css_size_t css_parse_size(const char *value);

// Transform a size into pixels given unit-type dpi, dsp and container-size
#define CSS_GET_SIZE(v, dpi, dsp, sz) ( \
    (v).unit == CSS_SIZE_PX ? (v).len : ( \
    (v).unit == CSS_SIZE_PTS ? (v).len * (dpi) / 100 : ( \
    (v).unit == CSS_SIZE_DP ? (int)((v).len * (dsp)) : ( \
    (v).unit == CSS_SIZE_PERC ? (v).len * (sz) / 1000 : ( \
      0 )))))

#define CSS_GET_SIZE_R(v, dpi, dsp, sz) ( \
    (v).unit == CSS_SIZE_PX ? (v).len : ( \
    (v).unit == CSS_SIZE_PTS ? (v).len * (dpi) / 100 : ( \
    (v).unit == CSS_SIZE_DP ? (int)((v).len * (dsp)) : ( \
    (v).unit == CSS_SIZE_PERC ? ((v).len ? 1000 * (sz) / (v).len : 0) : ( \
      0 )))))

#define CSS_SET_PX(v,n)  do { (v).len = (n); (v).unit = CSS_SIZE_PX; } while(0)

enum {
    CSS_SIZE_UNDEF = 0,
    CSS_SIZE_PX,
    CSS_SIZE_PTS,
    CSS_SIZE_DP,
    CSS_SIZE_PERC,

};

#endif  /* _KORA_CSS_H */
