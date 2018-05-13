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
#ifndef _KERNEL_SCALL_H
#define _KERNEL_SCALL_H 1

typedef long (*scall_handler)(long a1, long a2, long a3, long a4, long a5);

typedef struct scall scall_t;
struct scall {
    scall_handler handler;
    char name[12];
    bool retrn;
    uint8_t args[5];
};

enum {
    SC_NOARG,
    SC_SIGNED,
    SC_UNSIGNED,
    SC_OCTAL,
    SC_HEX,
    SC_STRING,
    SC_FD,
    SC_STRUCT,
    SC_OFFSET,
    SC_POINTER,
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define PW_SHUTDOWN  1
#define PW_REBOOT  2
#define PW_SLEEP  3
#define PW_HIBERNATE  4


#include <kora/syscalls.h>


#endif  /* _KERNEL_SCALL_H */
