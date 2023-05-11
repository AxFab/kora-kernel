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


// Relocation types
// P: Address pointer
// S: Address of symnol
// A: Value at address
// B: Base library address
#define R_386_32                    1 // word32 (S + A)
#define R_386_PC32                  2 // word32 (S + A - P)
#define R_386_COPY                  5
#define R_386_GLOB_DAT              6 // word32 (S)
#define R_386_JUMP_SLOT             7 // word32 (S)
#define R_386_RELATIVE              8 // word32 (B + A)

#define R_X86_64_NONE             0  /**< @brief @p none none */
#define R_X86_64_64               1  /**< @brief @p word64 S + A */
#define R_X86_64_PC32             2  /**< @brief @p word32 S + A - P */
#define R_X86_64_GOT32            3  /**< @brief @p word32 G + A */
#define R_X86_64_PLT32            4  /**< @brief @p word32 L + A - P */
#define R_X86_64_COPY             5  /**< @brief @p none none */
#define R_X86_64_GLOB_DAT         6  /**< @brief @p word64 S */
#define R_X86_64_JUMP_SLOT        7  /**< @brief @p word64 S */
#define R_X86_64_RELATIVE         8  /**< @brief @p word64 B + A */
#define R_X86_64_GOTPCREL         9  /**< @brief @p word32 G + GOT + A - P */
#define R_X86_64_32               10 /**< @brief @p word32 S + A */
#define R_X86_64_32S              11 /**< @brief @p word32 S + A */
/* vvv These should not appear in a valid file */
#define R_X86_64_16               12 /**< @brief @p word16 S + A */
#define R_X86_64_PC16             13 /**< @brief @p word16 S + A - P */
#define R_X86_64_8                14 /**< @brief @p word8  S + A */
#define R_X86_64_PC8              15 /**< @brief @p word8  S + A - P */
/* ^^^ These should not appear in a valid file */
#define R_X86_64_DTPMOD64         16 /**< @brief @p word64 */
#define R_X86_64_DTPOFF64         17 /**< @brief @p word64 */
#define R_X86_64_TPOFF64          18 /**< @brief @p word64 */
#define R_X86_64_TLSGD            19 /**< @brief @p word32 */
#define R_X86_64_TLSLD            20 /**< @brief @p word32 */
#define R_X86_64_DTPOFF32         21 /**< @brief @p word32 */
#define R_X86_64_GOTTPOFF         22 /**< @brief @p word32 */
#define R_X86_64_TPOFF32          23 /**< @brief @p word32 */
#define R_X86_64_PC64             24 /**< @brief @p word64 S + A - P */
#define R_X86_64_GOTOFF64         25 /**< @brief @p word64 S + A - GOT */
#define R_X86_64_GOTPC32          26 /**< @brief @p word32 GOT + A - P */
/* Large model */
#define R_X86_64_GOT64            27 /**< @brief @p word64 G + A */
#define R_X86_64_GOTPCREL64       28 /**< @brief @p word64 G + GOT - P + A */
#define R_X86_64_GOTPC64          29 /**< @brief @p word64 GOT - P + A */
#define R_X86_64_GOTPLT64         30 /**< @brief @p word64 G + A */
#define R_X86_64_PLTOFF64         31 /**< @brief @p word64 L - GOT + A */
/* ... */
#define R_X86_64_SIZE32           32 /**< @brief @p word32 Z + A */
#define R_X86_64_SIZE64           33 /**< @brief @p word64 Z + A */
#define R_X86_64_GOTPC32_TLSDESC  34 /**< @brief @p word32 */
#define R_X86_64_TLSDESC_CALL     35 /**< @brief @p none */
#define R_X86_64_TLSDESC          36 /**< @brief @p word64*2 */
#define R_X86_64_IRELATIVE        37 /**< @brief @p word64 indirect (B + A) */


#define R_AARCH64_COPY          1024
#define R_AARCH64_GLOB_DAT      1025

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

typedef struct dlib dlib_t;
typedef struct dlsection dlsection_t;
typedef struct dlsym dlsym_t;
typedef struct dlreloc dlreloc_t;
typedef struct dlname dlname_t;
typedef struct dlproc dlproc_t;

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
    int type; // 0extern, 1 global, 2 local
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
    size_t length;
    size_t offset; // From
    size_t foff;
    size_t moff;
    size_t fsize;
    size_t msize;
    llnode_t node;
    int rights;
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
    mtx_t mtx;
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

struct dlproc
{
    dlib_t *exec;
    llhead_t libs;
    hmap_t libs_map;
    hmap_t symbols_map;
    splock_t lock;
};

int dlib_open(vmsp_t *vmsp, fs_anchor_t *fsanchor, user_t *user, const char *name);

dlproc_t *dlib_proc();
int dlib_destroy(dlproc_t *proc);
dlib_t *dlib_create(const char *name, inode_t *ino);
void dlib_clean(dlproc_t *proc, dlib_t *lib);
int dlib_parse(dlproc_t *proc, dlib_t *lib);

int dlib_rebase(vmsp_t *vmsp, hmap_t *symbols_map, dlib_t *lib);
int dlib_resolve(hmap_t *symbols_map, dlib_t *lib);
int dlib_relloc(vmsp_t *vmsp, dlib_t *lib);

void *dlib_sym(dlproc_t *proc, const char *symbol);
size_t dlib_fetch_page(dlib_t *lib, size_t off, bool blocking);
void dlib_release_page(dlib_t *lib, size_t off, size_t pg);
char *dlib_name(dlib_t *lib, char *buf, int len);

void dlib_add_symbol(dlproc_t *proc, dlib_t *lib, const char *name, size_t value);
const char *dlib_rev_ksymbol(dlproc_t *proc, size_t ip, char *buf, int lg);

int elf_parse(dlib_t *lib, blkmap_t *bkm);


#endif  /* _KERNEL_DLIB_H */
