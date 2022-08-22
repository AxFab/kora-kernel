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
#ifndef _KERNEL_DLIB_H
#define _KERNEL_DLIB_H 1

#include <stddef.h>
#include <kora/mcrs.h>
#include <kora/hmap.h>
#include <kora/llist.h>
#include <kernel/vfs.h>
#include <kernel/memory.h>
#include <kernel/blkmap.h>


#define R_386_32 1
#define R_386_PC32 2
#define R_386_COPY 5
#define R_386_GLOB_DAT 6
#define R_386_JUMP_SLOT 7
#define R_386_RELATIVE 8

#if 0 
typedef struct dynsym dynsym_t;
typedef struct dynlib dynlib_t;
typedef struct dyndep dyndep_t;
typedef struct dynsec dynsec_t;
typedef struct dynrel dynrel_t;

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
    size_t text_off;
    void *iomap;
    size_t iolg;
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
typedef struct proc proc_t;
struct process {
    llhead_t stage1;
    llhead_t stage2;
    llhead_t stage3;
    llhead_t libraries;
    fnode_t **paths;
    int path_sz;
    hmap_t symbols;
};

struct proc {
    dynlib_t exec;
    hmap_t symbols;
    hmap_t libs_map;
    llhead_t queue;
    llhead_t libraries;
    mspace_t *mspace;
    char *execname;
    vfs_t *fs;
    void *acl;
    char *env;
    bool req_set_uid;
};

proc_t *dlib_process(vfs_t *fs, mspace_t *mspace);
bool dlib_resolve_symbols(proc_t *proc, dynlib_t *lib);
void dlib_destroy(dynlib_t *lib);
void dlib_rebase(proc_t *proc, mspace_t *mspace, dynlib_t *lib);
int dlib_openexec(proc_t *proc, const char *execname);
void dlib_unload(proc_t *proc, mspace_t *mspace, dynlib_t *lib);
int dlib_map(dynlib_t *dlib, mspace_t *mspace);
int dlib_map_all(proc_t *proc);
void *dlib_exec_entry(proc_t *proc);


int elf_parse(dynlib_t *dlib);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#else

typedef struct dlib dlib_t;
typedef struct dlsection dlsection_t;
typedef struct dlsym dlsym_t;
typedef struct dlreloc dlreloc_t;
typedef struct dlname dlname_t;

struct dlname
{
    char *name;
    llnode_t node;
};

struct dlsym
{
    char *name;
    size_t address;
    size_t size;
    int flags;
    llnode_t node;
};

struct dlreloc
{
    size_t offset;
    int type;
    dlsym_t *symbol;
    llnode_t node;
};


struct dlsection
{
    size_t offset;
    size_t length;
    size_t csize;
    size_t foff;
    llnode_t node;
    int rights;

    //size_t lower;
    //size_t upper;
    //size_t start;
    //size_t end;
    //size_t offset;
    //char rights;
};

struct dlib
{
    char *name;
    int rcu;
    int page_rcu;
    size_t entry;
    size_t init;
    size_t fini;
    size_t base;
    size_t length;
    size_t text_off;
    splock_t lock;
    llhead_t sections;
    llhead_t intern_symbols;
    llhead_t extern_symbols;
    llhead_t depends;
    llhead_t relocations;
    llnode_t node;
    inode_t *ino;
    size_t *pages;
    char *rpath;
};

typedef struct dlproc dlproc_t;
struct dlproc
{
    dlib_t *exec;
    llhead_t libs;
    hmap_t libs_map;
    hmap_t symbols_map;
};

int dlib_open(dlproc_t *proc, mspace_t *mm, vfs_t *vfs, acl_t *acl, const char *name);

dlproc_t *dlib_proc();
int dlib_destroy(dlproc_t *proc);
dlib_t *dlib_create(const char *name);
void dlib_clean(dlib_t *lib);
int dlib_parse(dlib_t *lib, inode_t *ino);

int dlib_rebase(mspace_t *mm, hmap_t *symbols_map, dlib_t *lib);
int dlib_resolve(hmap_t *symbols_map, dlib_t *lib);
int dlib_relloc(dlib_t *lib);

void *dlib_sym(dlproc_t *proc, const char *symbol);
size_t dlib_fetch_page(dlib_t *lib, size_t off);
void dlib_release_page(dlib_t *lib, size_t off, size_t pg);
char *dlib_name(dlib_t *lib, char *buf, int len);


int elf_parse(dlib_t *lib, blkmap_t *bkm);


#endif

#endif  /* _KERNEL_DLIB_H */
