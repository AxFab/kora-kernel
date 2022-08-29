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
#include "../cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <kora/mcrs.h>
#include <kernel/memory.h>
#include <kernel/dlib.h>
#include <kernel/stdc.h>
#include <kernel/blkmap.h>
#include <assert.h>
#include <errno.h>


#define ST_MSPACE 1
#define ST_ADDRESS 2

int alloc_check();


struct
{
    // Config
    size_t ksp_size;
    size_t usp_size;
    size_t pages_count;
    
    size_t usp_lower;
    size_t usp_upper;

    //int page_used;
    //mspace_t *mm;
    //size_t *pages;

    //// Page bitmap
    //char *pg_map; 
    //size_t pg_len;
    //size_t pg_base;

} _;

typedef struct mmu_dir
{
    size_t dir;
    size_t len;
    size_t pages[0];
} mmu_dir_t;

void __create_mmu_dir(mspace_t *mspace)
{
    size_t len = (mspace->upper_bound - mspace->lower_bound) / PAGE_SIZE;
    mmu_dir_t *dir = malloc(sizeof(mmu_dir_t) + sizeof(size_t) * len);
    memset(dir->pages, 0, sizeof(size_t) * len);
    dir->len = len;
    dir->dir = page_new();
    mspace->t_size++;
    mspace->directory = (size_t)dir;
}

void mmu_enable() 
{
    kMMU.kspace->directory = 0;
    kMMU.kspace->lower_bound = 16 * _Mib_;
    kMMU.kspace->upper_bound = 16 * _Mib_ + _.ksp_size;

    _.usp_lower = 32 * _Mib_ + _.ksp_size;
    _.usp_upper = 32 * _Mib_ + _.ksp_size + _.usp_size;
    printf("Setup kernel space at [%p-%p] and user space at [%p-%p]\n",
        (void *)kMMU.kspace->lower_bound, (void *)kMMU.kspace->upper_bound, (void *)_.usp_lower, (void *)_.usp_upper);
    
    size_t base = 4 * _Mib_ + 16 * _Kib_;
    size_t lg = 64;
    for (unsigned p = 0; p < _.pages_count; ) {
        int len = MIN(_.pages_count - p, lg);
        page_range(base, len * PAGE_SIZE);
        p += len;
        base = ALIGN_UP(base + len * PAGE_SIZE, _Mib_);
        lg *= 2;
    }

    // kMMU.free_pages = _.pages_count;
    // kMMU.pages_amount = _.pages_count;
    // kMMU.upper_physical_page = base / PAGE_SIZE;

    //_.pg_len = ALIGN_UP(kMMU.upper_physical_page, 8) / 8;
    //_.pg_map = malloc(_.pg_len);
    //memset(_.pg_map, 0, _.pg_len);
    //_.pg_base = 4 * _Mib_ / PAGE_SIZE; // First page !?

    // Create Initial directory
    __create_mmu_dir(kMMU.kspace);
}

void mmu_leave()
{
    mmu_destroy_uspace(kMMU.kspace);
}

//void mmu_context(mspace_t *mm)
//{
//    //_.pages = (size_t *)mm->directory;
//}

void mmu_create_uspace(mspace_t *mspace)
{
    mspace->lower_bound = _.usp_lower;
    mspace->upper_bound = _.usp_upper;
    __create_mmu_dir(mspace);
}

void mmu_destroy_uspace(mspace_t *mspace)
{
    mmu_dir_t *dir = (void *)mspace->directory;
    int leak = 0;
    for (unsigned i = 0; i < dir->len; ++i) {
        if (dir->pages[i] != 0)
            leak++;
    }
     
    if (leak > 0)
        cli_warn("Leaking %d page(s) on closing memory space", leak);
    page_release(dir->dir);
    free(dir);
    mspace->t_size--;
}



void page_release_kmap_stub(size_t page);
size_t mmu_read_kmap_stub(size_t address);


/* - */
size_t mmu_resolve(mspace_t *mspace, size_t vaddr, size_t phys, int flags)
{
    assert(vaddr >= mspace->lower_bound && vaddr < mspace->upper_bound);
    assert(mspace == kMMU.kspace || mspace == kMMU.uspace);
    if (phys == 0)
        phys = page_new();

    mmu_dir_t *dir = (void *)mspace->directory;
    size_t idx = (vaddr - mspace->lower_bound) / PAGE_SIZE;
    assert(idx < dir->len);
    if (dir->pages[idx] != 0)
        cli_warn("Try to overwrite a resolved page");
    dir->pages[idx] = phys | 8 | (flags & (VM_RWX | VM_UNCACHABLE));
    mspace->p_size++;
    if (flags & VM_SHARED)
        mspace->s_size++;
    return phys;
}

/* - */
size_t mmu_read(mspace_t *mspace, size_t vaddr)
{
    assert(vaddr >= mspace->lower_bound && vaddr < mspace->upper_bound);
    assert(mspace == kMMU.kspace || mspace == kMMU.uspace);

    mmu_dir_t *dir = (void *)mspace->directory;
    size_t idx = (vaddr - mspace->lower_bound) / PAGE_SIZE;
    assert(idx < dir->len);
    return dir->pages[idx] & ~(PAGE_SIZE-1);
}

int mmu_read_flags(mspace_t *mspace, size_t vaddr)
{
    assert(vaddr >= mspace->lower_bound && vaddr < mspace->upper_bound);
    assert(mspace == kMMU.kspace || mspace == kMMU.uspace);

    mmu_dir_t *dir = (void *)mspace->directory;
    size_t idx = (vaddr - mspace->lower_bound) / PAGE_SIZE;
    assert(idx < dir->len);
    return dir->pages[idx] & (VM_RWX | VM_UNCACHABLE);
}

/* - */
size_t mmu_drop(mspace_t *mspace, size_t vaddr)
{
    assert(vaddr >= mspace->lower_bound && vaddr < mspace->upper_bound);
    assert(mspace == kMMU.kspace || mspace == kMMU.uspace);

    mmu_dir_t *dir = (void *)mspace->directory;
    size_t idx = (vaddr - mspace->lower_bound) / PAGE_SIZE;
    assert(idx < dir->len);
    size_t phys = dir->pages[idx] & ~(PAGE_SIZE - 1);
    dir->pages[idx] = 0;
    return phys;
}

/* - */
size_t mmu_protect(mspace_t *mspace, size_t vaddr, int flags)
{
    assert(vaddr >= mspace->lower_bound && vaddr < mspace->upper_bound);
    assert(mspace == kMMU.kspace || mspace == kMMU.uspace);

    mmu_dir_t *dir = (void *)mspace->directory;
    size_t idx = (vaddr - mspace->lower_bound) / PAGE_SIZE;
    assert(idx < dir->len);
    size_t phys = 0;
    if (dir->pages[idx] != 0) {
        phys = dir->pages[idx] & ~(PAGE_SIZE - 1);
        dir->pages[idx] = phys | 8 | (flags & (VM_RWX | VM_UNCACHABLE));
    }
    return phys;

}
/* - */
bool mmu_dirty(mspace_t *mspace, size_t vaddr)
{
    assert(vaddr >= mspace->lower_bound && vaddr < mspace->upper_bound);
    assert(mspace == kMMU.kspace || mspace == kMMU.uspace);

    mmu_dir_t *dir = (void *)mspace->directory;
    size_t idx = (vaddr - mspace->lower_bound) / PAGE_SIZE;
    assert(idx < dir->len);
    return dir->pages[idx] & 0x200;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static int __parse_type(const char *type)
{
    int flags = 0;
    if (strcmp(type, "HEAP") == 0) {
        flags = VMA_HEAP;
    } else if (strcmp(type, "STACK") == 0) {
        flags = VMA_STACK;
    } else if (strcmp(type, "ANON") == 0) {
        flags = VMA_ANON;
    } else if (strcmp(type, "PIPE") == 0) {
        flags = VMA_PIPE;
    } else {
        return cli_error("Unsupported type %s", type);
    }
    return flags;
}

static int __parse_typex(const char *type)
{
    int flags = 0;
    if (strcmp(type, "PHYS") == 0) {
        flags = VMA_PHYS;
    } else if (strcmp(type, "FILE") == 0) {
        flags = VMA_FILE;
    //} else if (strcmp(type, "EXEC") == 0) {
    //    flags = VMA_CODE;
    } else {
        return cli_error("Unsupported type %s", type);
    }
    return flags;
}

static int __parse_flags(const char *arg)
{
    int flags = 0;
    if (arg) {
        for (; *arg; arg++) {
            if ((*arg | 0x20) == 'r')
                flags |= VM_RD;
            if ((*arg | 0x20) == 'w')
                flags |= VM_WR;
            if ((*arg | 0x20) == 'x')
                flags |= VM_EX;
            if ((*arg | 0x20) == 's')
                flags |= VM_SHARED;
            if ((*arg | 0x20) == 'a')
                flags |= VM_RESOLVE;
            if ((*arg | 0x20) == 'u')
                flags |= VM_UNCACHABLE;
            if ((*arg | 0x20) == 'c')
                flags |= VMA_CLEAN;
            if ((*arg | 0x20) == 'f')
                flags |= VMA_FIXED;
        }
    }
    return flags;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

size_t read_address(char *address)
{ 
    size_t vaddr = 0;
    if (address[0] == '@') {
        char name[16];
        int i = 0;
        while (isalnum(*(++address)))
            name[i++] = *address;
        name[i++] = '\0';
        char s = *address;
        if (s == '+' || s == '-') {
            vaddr = strtof(++address, &address);
            char c = (*address) | 0x20;
            if (c == 'k')
                vaddr *= 1024UL;
            if (c == 'm')
                vaddr *= 1024UL * 1024UL;
            if (c == 'g')
                vaddr *= 1024UL * 1024UL * 1024UL;
            if (c == 't')
                vaddr *= 1024UL * 1024UL * 1024UL * 1024UL;
        }
        if (s == '-')
            vaddr = (size_t)cli_fetch(name, ST_ADDRESS) - vaddr;
        else
            vaddr += (size_t)cli_fetch(name, ST_ADDRESS);
    } else if (address[1] == '0') {
        vaddr = strtof(address, &address);
    }
    return vaddr;
}

int read_access(char *mode)
{
    int access = 0;
    for (; *mode; ++mode) {
        if (*mode == 'r')
            access |= VM_RD;
        else if (*mode == 'w')
            access |= VM_WR;
        else if (*mode == 'x')
            access |= VM_EX;
    }
    return access;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int do_show(void *ctx, size_t *params)
{
    // char *checks = (char *)params[0];

    mspace_display(kMMU.kspace);
    if (kMMU.uspace)
        mspace_display(kMMU.uspace);

    //if (checks == NULL)
    //    return 0;

    //int err = 0;
    //while (*checks) {
    //    char x = *(checks++);
    //    size_t sz = strtod(checks, &checks);
    //    char c = (*checks++) | 0x20;
    //    if (c == 'k')
    //        sz *= 1024UL;
    //    else if (c == 'm')
    //        sz *= 1024UL * 1024UL;
    //    else if (c == 'g')
    //        sz *= 1024UL * 1024UL * 1024UL;
    //    else if (c == 't')
    //        sz *= 1024UL * 1024UL * 1024UL * 1024UL;
    //    else
    //        checks--;

    //    if ((x | 0x20) == 'p' && sz != mm->p_size * PAGE_SIZE) {
    //        err++;
    //        cli_warn("Bad number of private page\n");
    //    } else if ((x | 0x20) == 'v' && sz != mm->v_size * PAGE_SIZE) {
    //        err++;
    //        cli_warn("Bad number of virtual page\n");
    //    } else if ((x | 0x20) == 's' && sz != mm->s_size * PAGE_SIZE) {
    //        err++;
    //        cli_warn("Bad number of shared page\n");
    //    }

    //    if (*checks == ',')
    //        checks++;
    //    else if (*checks != '\0' && *checks != ',')
    //        return cli_error("Bad checks parameter expect ','\n");

    //}
    //return err == 0 ? 0 : -1;
    return 0;
}

int do_touch(void *ctx, size_t *params)
{
    const char *rigths[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx" };
    char *address = (char *)params[0];
    char *mode = (char *)params[1];

    size_t vaddr = cli_read_size(address);
    int access = __parse_flags(mode) & VM_RWX;

    int pg = 0;
    mmu_dir_t *dir = NULL;
    size_t idx = 0;

    mspace_t *mspace = NULL;
    if (vaddr >= kMMU.kspace->lower_bound && vaddr < kMMU.kspace->upper_bound)
        mspace = kMMU.kspace;
    else
        mspace = kMMU.uspace;

    if (mspace != NULL) {
        dir = (void *)mspace->directory;
        idx = (vaddr - mspace->lower_bound) / PAGE_SIZE;
        pg = dir->pages[idx] & (VM_RWX | 8);
    }

    if ((int)(pg & access) != access) {
        int flt = 0;
        if ((pg & 8) == 0)
            flt |= PGFLT_MISSING;
        if (access & VM_WR)
            flt |= PGFLT_WRITE;

        printf("#PF at %p %s\n", (void *)vaddr, rigths[access]);
         if (page_fault(vaddr, flt) != 0) {
            errno = ENOMEM;
            return -1;
        }
        if (mspace != NULL)
            pg = dir->pages[idx] & (VM_RWX | 8);
    }

    if ((int)(pg & access) != access || (pg & VM_RD) == 0) {
        errno = EAGAIN;
        return -1;
    }

    if (access & VM_WR && mspace != NULL)
        dir->pages[idx] |= 0x200;
    return 0;
}




//int do_exec(void *ctx, size_t *params)
//{
//    mspace_t *mm = kMMU.uspace;
//    char *name = (char *)params[0];
//    size_t cd = cli_read_size((char *)params[1]);
//    size_t dt = cli_read_size((char *)params[2]);
//
//    dlib_t *lib = dlib_create(name);
//    dlsection_t *sc = kalloc(sizeof(dlsection_t));
//    sc->length = cd;
//    sc->rights = 5;
//    ll_append(&lib->sections, &sc->node);
//
//    sc = kalloc(sizeof(dlsection_t));
//    sc->offset = cd;
//    sc->length = dt;
//    sc->rights = 6;
//    ll_append(&lib->sections, &sc->node);
//
//    lib->length = cd + dt;
//
//    int ret = dlib_relloc(mm, lib);
//    ret = dlib_rebase(mm, NULL, lib);
//    return ret;
//}
//
//int do_elf(void *ctx, size_t *params)
//{
//    char tmp[32];
//    char *name = (char *)params[0];
//
//    char *lname = strrchr(name, '/');
//    dlib_t *lib = dlib_create(lname ? lname + 1 : name);
//    blkmap_t *bkm = blk_open(name, 4096);
//    int ret = elf_parse(lib, bkm);
//    printf("Reading elf file %s: %s", dlib_name(lib, tmp, 32), ret == 0 ? "succeed" : "fails");
//    dlib_clean(lib);
//    return ret;
//}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

llhead_t __kernel_symbols;
void __add_symbol(dlproc_t *proc, const char *name, size_t ptr)
{
    dlsym_t *symbol = kalloc(sizeof(dlsym_t));
    symbol->name = kstrdup(name);
    symbol->address = ptr;
    ll_append(&__kernel_symbols, &symbol->node);
    splock_lock(&proc->lock);
    hmp_put(&proc->symbols_map, name, strlen(name), symbol);
    splock_unlock(&proc->lock);
}

int do_start(void *ctx, size_t *params)
{
    size_t ksize = cli_read_size((char *)params[0]);
    size_t usize = cli_read_size((char *)params[1]);
    size_t pages = cli_read_size((char *)params[2]);

    _.ksp_size = ksize;
    _.usp_size = usize;
    _.pages_count = pages;
    memory_initialize();
    memory_info();

    llist_init(&__kernel_symbols);
    dlproc_t *proc = dlib_proc();
    kMMU.kspace->proc = proc;
    size_t no = 0x1000 / 0x40;
    __add_symbol(proc, "__errno_location", (no++ * 0x40));
    __add_symbol(proc, "kprintf", (no++ * 0x40));
    __add_symbol(proc, "memset", (no++ * 0x40));
    __add_symbol(proc, "strdup", (no++ * 0x40));
    __add_symbol(proc, "irq_disable", (no++ * 0x40));
    __add_symbol(proc, "vfs_close_inode", (no++ * 0x40));
    __add_symbol(proc, "vfs_mkdev", (no++ * 0x40));
    __add_symbol(proc, "vfs_rmdev", (no++ * 0x40));
    __add_symbol(proc, "vfs_inode", (no++ * 0x40));
    __add_symbol(proc, "irq_register", (no++ * 0x40));
    __add_symbol(proc, "irq_enable", (no++ * 0x40));
    __add_symbol(proc, "__divdi3", (no++ * 0x40));
    return 0;
}

int do_quit()
{
    memory_sweep();

    for (;;) {
        dlsym_t *symbol = ll_dequeue(&__kernel_symbols, dlsym_t, node);
        if (symbol == NULL)
            break;
        kfree(symbol->name);
        kfree(symbol);
    }

    if (kMMU.kspace->proc)
        dlib_destroy(kMMU.kspace->proc);
    return alloc_check();
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int __mmap(mspace_t *msp, size_t *params)
{
    char *type = (char *)params[0];
    size_t length = cli_read_size((char *)params[1]);
    char *arg = (char *)params[2];

    if (msp == NULL)
        return cli_error("No user-space selected");

    int flags = __parse_type(type);
    if (flags == -1)
        return -1;

    flags |= __parse_flags(arg);

    void *ptr = mspace_map(msp, 0, length, NULL, 0, flags);
    return ptr == NULL ? -1 : 0;
}

int __mmapx(mspace_t *msp, size_t *params)
{
    char *type = (char *)params[0];
    size_t length = cli_read_size((char *)params[1]);
    char *arg = (char *)params[2];
    char *name = (char *)params[3];
    size_t off = cli_read_size((char *)params[4]);

    if (msp == NULL)
        return cli_error("No user-space selected");

    int flags = __parse_typex(type);
    if (flags == -1)
        return -1;
    if (flags != VMA_PHYS && name == NULL)
        return cli_error("Unspecified file name");

    flags |= __parse_flags(arg);

    void *ino = NULL;
    if (name != NULL && *name != '-') {
        ino = vfs_search_ino(NULL, name, NULL, true);
        if (ino == NULL)
            return cli_error("Can't open file '%s'", name);
    }

    void *ptr = mspace_map(msp, 0, length, ino, off, flags);
    return ptr == NULL ? -1 : 0;
}

int __munmap(mspace_t *msp, size_t *params)
{
    size_t base = cli_read_size((char *)params[0]);
    size_t length = cli_read_size((char *)params[1]);

    if (msp == NULL)
        return cli_error("No user-space selected");

    return mspace_unmap(msp, base, length);
}

int __mprotect(mspace_t *msp, size_t *params)
{
    size_t base = cli_read_size((char *)params[0]);
    size_t length = cli_read_size((char *)params[1]);
    char *arg = (char *)params[2];

    if (msp == NULL)
        return cli_error("No user-space selected");

    int flags = __parse_flags(arg);
    return mspace_protect(msp, base, length, flags);
}

int __dlib(mspace_t *msp, size_t *params)
{
    char *name = (char *)params[0];
    int ret = dlib_open(msp, NULL, NULL, name);
    if (ret != 0)
        cli_error("Error on loading library %s", name);
    return ret;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int do_kmap(void *ctx, size_t *params)
{
    return __mmap(kMMU.kspace, params);
}

int do_kmapx(void *ctx, size_t *params)
{
    return __mmapx(kMMU.kspace, params);
}

int do_kunmap(void *ctx, size_t *params)
{
    return __munmap(kMMU.kspace, params);
}

int do_kprotect(void *ctx, size_t *params)
{
    return __mprotect(kMMU.kspace, params);
}

int do_kdlib(void *ctx, size_t *params)
{
    return __dlib(kMMU.kspace, params);
}

int do_mmap(void *ctx, size_t *params)
{
    return __mmap(kMMU.uspace, params);
}

int do_mmapx(void *ctx, size_t *params)
{
    return __mmapx(kMMU.uspace, params);
}

int do_munmap(void *ctx, size_t *params)
{
    return __munmap(kMMU.uspace, params);
}

int do_mprotect(void *ctx, size_t *params)
{
    return __mprotect(kMMU.uspace, params);
}

int do_mdlib(void *ctx, size_t *params)
{
    return __dlib(kMMU.uspace, params);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int do_uspace_create(void *ctx, size_t *params)
{
    char *store = (char *)params[0];
    mspace_t *usp = mspace_create();
    cli_store(store, usp, ST_MSPACE);
    kMMU.uspace = usp;
    return 0;
}

int do_uspace_open(void *ctx, size_t *params)
{
    char *store = (char *)params[0];
    if (kMMU.uspace == NULL)
        return cli_error("No user-space selected");
    mspace_t *usp = mspace_open(kMMU.uspace);
    cli_store(store, usp, ST_MSPACE);
    kMMU.uspace = usp;
    return 0;
}

int do_uspace_select(void *ctx, size_t *params)
{
    char *store = (char *)params[0];
    mspace_t *usp = cli_fetch(store, ST_MSPACE);
    if (usp == NULL)
        return cli_error("No user-space selected");
    kMMU.uspace = usp;
    return 0;
}

int do_uspace_clone(void *ctx, size_t *params)
{
    char *store = (char *)params[0];
    if (kMMU.uspace == NULL)
        return cli_error("No user-space selected");
    mspace_t *usp = mspace_clone(kMMU.uspace);
    cli_store(store, usp, ST_MSPACE);
    kMMU.uspace = usp;
    return 0;
}

int do_uspace_close(void *ctx, size_t *params)
{
    char *store = (char *)params[0];
    mspace_t *usp = cli_fetch(store, ST_MSPACE);
    if (usp == NULL)
        return cli_error("No user-space selected");
    if (usp != kMMU.uspace)
        return cli_error("Wromg user-space selected");
    mspace_close(usp);
    kMMU.uspace = NULL;
    cli_remove(store, ST_MSPACE);
    return 0;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void initialize()
{
    // memory_initialize();
}

void teardown()
{
}

void do_restart(void *ctx, size_t *param)
{
    teardown();
    // TODO -- Check all is clean
    initialize();
}


cli_cmd_t __commands[] = {
    { "RESTART", "", { 0, 0, 0, 0, 0 }, (void *)do_restart, 1 },
    { "START", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_start, 3 },
    { "QUIT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_quit, 0 },

    { "KMAP", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_kmap, 2 },
    { "KMAPX", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR }, (void *)do_kmapx, 5 },
    { "KUNMAP", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_kunmap, 2 },
    { "KPROTECT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_kprotect, 3 },
    { "KDLIB", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_kdlib, 1 },

    { "MMAP", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_mmap, 2 },
    { "MMAPX", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR }, (void *)do_mmapx, 5 },
    { "MUNMAP", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_munmap, 2 },
    { "MPROTECT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_mprotect, 3 },
    { "MDLIB", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_mdlib, 1 },

    { "USPACE_CREATE", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_uspace_create, 1 },
    { "USPACE_OPEN", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_uspace_open, 1 },
    { "USPACE_SELECT", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_uspace_select, 1 },
    { "USPACE_CLONE", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_uspace_clone, 1 },
    { "USPACE_CLOSE", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_uspace_close, 1 },
    

    { "SHOW", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_show, 0 },

    //{ "CREATE", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_create, 0 },
    //{ "OPEN", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_open, 0 },
    //{ "CLONE", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_clone, 0 },
    //{ "CLOSE", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_close, 0 },
    //{ "MAP", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR }, (void *)do_map, 5 },
    //{ "UNMAP", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_unmap, 2 },
    //{ "PROTECT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_protect, 3 },
    //{ "STACK", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_stack, 2 },
    //{ "HEAP", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_heap, 2 },
    //{ "EXEC", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, 0 }, (void *)do_exec, 2 },
    { "TOUCH", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void*)do_touch, 2 },
 
    //{ "ELF", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_elf, 1 },
    CLI_END_OF_COMMANDS,
};

int main(int argc, char **argv)
{
    return cli_main(argc, argv, &_, initialize, teardown);
}
