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
#ifndef _CTYPE_H
#define _CTYPE_H 1

static inline int isspace(char a)
{
    return a > 0 && a <= 0x20;
}

static inline int isdigit(char a)
{
    return a >= '0' && a <= '9';
}

// #define isalpha(a) ((((unsigned)(a)|32)-'a') < 26)
// #define isdigit(a) (((unsigned)(a)-'0') < 10)
// #define islower(a) (((unsigned)(a)-'a') < 26)
// #define isupper(a) (((unsigned)(a)-'A') < 26)
// #define isprint(a) (((unsigned)(a)-0x20) < 0x5f)
// #define isgraph(a) (((unsigned)(a)-0x21) < 0x5e)
// #define isspace(a) ((a)==' '||((unsigned)(a)-'\t'<5))



#endif /* _CTYPE_H */
