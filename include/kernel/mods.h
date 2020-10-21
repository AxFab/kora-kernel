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
#ifndef _KERNEL_MODS_H
#define _KERNEL_MODS_H 1

#include <kernel/stdc.h>
#include <kora/llist.h>
#include <kora/mcrs.h>


typedef struct kmodule kmodule_t;
// typedef struct kapi kapi_t;

struct kmodule {
    const char *name;
    void (*setup)();
    void (*teardown)();
    uint32_t version;
    llnode_t node;
};

// struct kapi {
//     const char *name;
//     void *sym;
// };

#define EXPORT_MODULE(n,s,t) \
    kmodule_t kmodule_##n = { \
        .name = #n, \
        .setup = s, \
        .teardown = t, \
        .version = VERS32(0,1,0), }

#define REQUIRE_MODULE(n) \
    const char *kdepsmodule_##n __attribute__(section(".kmod"), used) = #n

#define EXPORT_SYMBOL(n) \
    kapi_t _ksym_##n __attribute__(section(".ksymbols"), used) \
    { .name = #n, .sym = n }


#define kernel_export_symbol(x) module_symbol(#x,(x));

void module_symbol(const char *name, void *ptr);


#endif /* _KERNEL_MODS_H */
