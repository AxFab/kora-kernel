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
#ifndef _KERNEL_DLIB_H
#define _KERNEL_DLIB_H 1

#include <kernel/core.h>
#include <kora/stddef.h>
#include <kora/mcrs.h>
#include <kora/hmap.h>
#include <kernel/task.h>


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
    inode_t *ino;
    llnode_t node;
    llhead_t sections;
    llhead_t depends;
    llhead_t intern_symbols;
    llhead_t extern_symbols;
    llhead_t relocations;
    char *rpath;
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

struct proc {
    dynlib_t exec;
    HMP_map symbols;
    HMP_map libs_map;
    llhead_t queue;
    llhead_t libraries;
    mspace_t *mspace;
    char *execname;
    inode_t *root;
    inode_t *pwd;
    acl_t *acl;
    char *env;
    bool req_set_uid;
};

proc_t *dlib_process(resx_fs_t *fs, mspace_t *mspace);
bool dlib_resolve_symbols(proc_t *proc, dynlib_t *lib);
void dlib_destroy(dynlib_t *lib);
void dlib_rebase(proc_t *proc, mspace_t *mspace, dynlib_t *lib);
int dlib_openexec(proc_t *proc, const char *execname);
void dlib_unload(proc_t *proc, mspace_t *mspace, dynlib_t *lib);
int dlib_map(dynlib_t *dlib, mspace_t *mspace);
int dlib_map_all(proc_t *proc);

int elf_parse(dynlib_t *dlib);


#endif  /* _KERNEL_DLIB_H */
