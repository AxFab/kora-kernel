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


static inline int isalpha(int a)
{
    return (((unsigned)a | 32) - 'a') < 26;
}

static inline int isdigit(int a)
{
    return ((unsigned)a - '0') < 10;
}

static inline int islower(int a)
{
    return ((unsigned)a - 'a') < 26;
}

static inline int isupper(int a)
{
    return ((unsigned)a - 'A') < 26;
}

static inline int isprint(int a)
{
    return ((unsigned)a - 0x20) < 0x5f;
}

static inline int isgraph(int a)
{
    return ((unsigned)a - 0x21) < 0x5e;
}

static inline int isspace(int a)
{
    return (a) == ' ' || (unsigned)a - '\t' < 5;
}

// - - - - - - - - - - - - - - -

static inline int isalnum(int a)
{
    return (((unsigned)a | 32) - 'a') < 26 || ((unsigned)a - '0') < 10;
}

// - - - - - - - - - - - - - - -

static inline int tolower(int a)
{
    return ((unsigned)a - 'A') < 26 ? (a | 0x20) : a;
}

static inline int toupper(int a)
{
    return ((unsigned)a - 'a') < 26 ? (a & ~0x20) : a;
}

// - - - - - - - - - - - - - - -

int isblank(int);


#endif /* _CTYPE_H */
