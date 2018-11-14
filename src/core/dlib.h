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
#ifndef _SRC_DLIB
#define _SRC_DLIB 1

#include <kernel/core.h>
#include <kora/stddef.h>
#include <kora/mcrs.h>
#include <kora/hmap.h>

typedef struct dynlib dynlib_t;
typedef struct dynsec dynsec_t;
typedef struct dynsym dynsym_t;
typedef struct dynrel dynrel_t;
typedef struct dyndep dyndep_t;


struct dynsec {
    size_t lower;
    size_t upper;
    size_t start;
    size_t end;
    size_t offset;
    char rights;
    llnode_t node;
};

struct dynsym {
    char *name;
    size_t address;
    size_t size;
    int flags;
    llnode_t node;
};

struct dynrel {
    size_t address;
    int type;
    dynsym_t *symbol;
    llnode_t node;
};

struct dyndep {
    char *name;
    inode_t *ino;
    dynlib_t *lib;
    llnode_t node;
};

struct dynlib {
    char *name;
    int rcu;
    size_t entry;
    size_t init;
    size_t fini;
    size_t base;
    size_t length;
    bio_t *io;
    llnode_t node;
    llhead_t sections;
    llhead_t depends;
    llhead_t intern_symbols;
    llhead_t extern_symbols;
    llhead_t relocations;
};


typedef struct process process_t;
struct process {
    llhead_t stage1;
    llhead_t stage2;
    llhead_t stage3;
    llhead_t libraries;
    inode_t **paths;
    int path_sz;
    HMP_map symbols;
};


#endif  /* _SRC_DLIB */
