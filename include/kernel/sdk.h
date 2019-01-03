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
#ifndef _KERNEL_SDK_H
#define _KERNEL_SDK_H 1

#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/device.h>
#include <kernel/files.h>
#include <kernel/memory.h>
#include <kernel/task.h>
#include <kernel/net.h>
#include <kernel/dlib.h>
#include <kernel/syscalls.h>
#include <string.h>
#include <time.h>
#include <errno.h>


#define KAPI(n) { &(n), 0, #n }
struct kdk_api {
    void *address;
    size_t length;
    const char *name;
};

extern proc_t kproc;

void kmod_symbol(CSTR name, void *ptr);
void kmod_symbols(kdk_api_t *kapi);


#endif /* _KERNEL_SDK_H */
