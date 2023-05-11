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
    //vmsp_t *mm;
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

void __create_mmu_dir(vmsp_t *vmsp)
{
    size_t len = (vmsp->upper_bound - vmsp->lower_bound) / PAGE_SIZE;
    mmu_dir_t *dir = malloc(sizeof(mmu_dir_t) + sizeof(size_t) * len);
    memset(dir->pages, 0, sizeof(size_t) * len);
    dir->len = len;
    dir->dir = page_new();
    vmsp->t_size++;
    vmsp->directory = (size_t)dir;
}

void mmu_enable() 
{
    __mmu.kspace->lower_bound = 16 * _Mib_;
    __mmu.kspace->upper_bound = 16 * _Mib_ + _.ksp_size;

    _.usp_lower = 32 * _Mib_ + _.ksp_size;
    _.usp_upper = 32 * _Mib_ + _.ksp_size + _.usp_size;
    printf("Setup kernel space at [%p-%p] and user space at [%p-%p]\n",
        (void *)__mmu.kspace->lower_bound, (void *)__mmu.kspace->upper_bound, (void *)_.usp_lower, (void *)_.usp_upper);
    
    size_t base = 4 * _Mib_ + 16 * _Kib_;
    size_t lg = 64;
    for (unsigned p = 0; p < _.pages_count; ) {
        int len = MIN(_.pages_count - p, lg);
        page_range(base, len * PAGE_SIZE);
        p += len;
        base = ALIGN_UP(base + len * PAGE_SIZE, _Mib_);
        lg *= 2;
    }

    // __mmu.free_pages = _.pages_count;
    // __mmu.pages_amount = _.pages_count;
    // __mmu.upper_physical_page = base / PAGE_SIZE;

    //_.pg_len = ALIGN_UP(__mmu.upper_physical_page, 8) / 8;
    //_.pg_map = malloc(_.pg_len);
    //memset(_.pg_map, 0, _.pg_len);
    //_.pg_base = 4 * _Mib_ / PAGE_SIZE; // First page !?

    // Create Initial directory
    __create_mmu_dir(__mmu.kspace);
}

void mmu_leave()
{
    __mmu.kspace->t_size--;
    if (__mmu.kspace->proc)
        dlib_destroy(__mmu.kspace->proc);

    mmu_destroy_uspace(__mmu.kspace);
}

//void mmu_context(vmsp_t *mm)
//{
//    //_.pages = (size_t *)mm->directory;
//}

vmsp_t *__mmu_set_vmsp = NULL;
void mmu_create_uspace(vmsp_t *vmsp)
{
    vmsp->lower_bound = _.usp_lower;
    vmsp->upper_bound = _.usp_upper;
    __create_mmu_dir(vmsp);
    __mmu_set_vmsp = vmsp;
    // TODO -- Prepare for mmu_set() !
}

void mmu_destroy_uspace(vmsp_t *vmsp)
{
    mmu_dir_t *dir = (void *)vmsp->directory;
    int leak = 0;
    for (unsigned i = 0; i < dir->len; ++i) {
        if (dir->pages[i] != 0)
            leak++;
    }
     
    if (leak > 0)
        cli_warn("Leaking %d page(s) on closing memory space", leak);
    page_release(dir->dir);
    free(dir);
    vmsp->t_size--;
}



void page_release_kmap_stub(size_t page);
size_t mmu_read_kmap_stub(size_t address);

static size_t __mmu_set(mmu_dir_t *dir, size_t idx, size_t phys, int flags)
{
    assert(idx < dir->len);
    dir->pages[idx] = phys | 8 | (flags & (VM_RWX | VM_UNCACHABLE));
    return phys;
}

size_t mmu_set(size_t directory, size_t vaddr, size_t phys, int flags)
{
    assert(__mmu.uspace);
    assert(__mmu_set_vmsp && __mmu_set_vmsp->directory == directory);
    mmu_dir_t *dir = (void *)directory;
    size_t idx = (vaddr - __mmu.uspace->lower_bound) / PAGE_SIZE;
    return __mmu_set(dir, idx, phys, flags);
}

/* - */
size_t mmu_resolve(size_t vaddr, size_t phys, int flags)
{
    vmsp_t *vmsp = memory_space_at(vaddr);
    assert(vaddr >= vmsp->lower_bound && vaddr < vmsp->upper_bound);
    assert(vmsp == __mmu.kspace || vmsp == __mmu.uspace);
    if (phys == 0)
        phys = page_new();

    mmu_dir_t *dir = (void *)vmsp->directory;
    size_t idx = (vaddr - vmsp->lower_bound) / PAGE_SIZE;
    return __mmu_set(dir, idx, phys, flags);
}

/* - */
size_t mmu_read(size_t vaddr)
{
    size_t pg = mmu_read_kmap_stub(vaddr);
    if (pg != 0)
        return pg;

    vmsp_t *vmsp = memory_space_at(vaddr);
    assert(vaddr >= vmsp->lower_bound && vaddr < vmsp->upper_bound);
    assert(vmsp == __mmu.kspace || vmsp == __mmu.uspace);

    mmu_dir_t *dir = (void *)vmsp->directory;
    size_t idx = (vaddr - vmsp->lower_bound) / PAGE_SIZE;
    assert(idx < dir->len);
    return dir->pages[idx] & ~(PAGE_SIZE-1);
}

int mmu_read_flags(size_t vaddr)
{
    vmsp_t *vmsp = memory_space_at(vaddr);
    assert(vaddr >= vmsp->lower_bound && vaddr < vmsp->upper_bound);
    assert(vmsp == __mmu.kspace || vmsp == __mmu.uspace);

    mmu_dir_t *dir = (void *)vmsp->directory;
    size_t idx = (vaddr - vmsp->lower_bound) / PAGE_SIZE;
    assert(idx < dir->len);
    return dir->pages[idx] & (VM_RWX | VM_UNCACHABLE);
}

/* - */
size_t mmu_drop(size_t vaddr)
{
    vmsp_t *vmsp = memory_space_at(vaddr);
    assert(vaddr >= vmsp->lower_bound && vaddr < vmsp->upper_bound);
    assert(vmsp == __mmu.kspace || vmsp == __mmu.uspace);

    mmu_dir_t *dir = (void *)vmsp->directory;
    size_t idx = (vaddr - vmsp->lower_bound) / PAGE_SIZE;
    assert(idx < dir->len);
    size_t phys = dir->pages[idx] & ~(PAGE_SIZE - 1);
    dir->pages[idx] = 0;
    return phys;
}

/* - */
size_t mmu_protect(size_t vaddr, int flags)
{
    vmsp_t *vmsp = memory_space_at(vaddr);
    assert(vaddr >= vmsp->lower_bound && vaddr < vmsp->upper_bound);
    assert(vmsp == __mmu.kspace || vmsp == __mmu.uspace);

    mmu_dir_t *dir = (void *)vmsp->directory;
    size_t idx = (vaddr - vmsp->lower_bound) / PAGE_SIZE;
    assert(idx < dir->len);
    size_t phys = 0;
    if (dir->pages[idx] != 0) {
        phys = dir->pages[idx] & ~(PAGE_SIZE - 1);
        dir->pages[idx] = phys | 8 | (flags & (VM_RWX | VM_UNCACHABLE));
    }
    return phys;

}
/* - */
bool mmu_dirty(size_t vaddr)
{
    vmsp_t *vmsp = memory_space_at(vaddr);
    assert(vaddr >= vmsp->lower_bound && vaddr < vmsp->upper_bound);
    assert(vmsp == __mmu.kspace || vmsp == __mmu.uspace);

    mmu_dir_t *dir = (void *)vmsp->directory;
    size_t idx = (vaddr - vmsp->lower_bound) / PAGE_SIZE;
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
    } else if (strcmp(type, "FILECPY") == 0) {
        flags = VMA_FILECPY;
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

    vmsp_display(__mmu.kspace);
    if (__mmu.uspace)
        vmsp_display(__mmu.uspace);

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

size_t read_address2(char *address)
{
    if (*address != '@')
        return cli_read_size(address);
    char buf[32];
    strncpy(buf, address, 32);
    char *p = strchr(buf, '+');
    char *n = strchr(buf, '-');
    if (p != NULL) {
        *p = '\0';
        return (size_t)cli_fetch(buf, ST_ADDRESS) + cli_read_size(&p[1]);
    }
    if (n != NULL) {
        *n = '\0';
        return (size_t)cli_fetch(buf, ST_ADDRESS) - cli_read_size(&n[1]);
    }
    return (size_t)cli_fetch(buf, ST_ADDRESS);
}

int do_touch(void *ctx, size_t *params)
{
    const char *rigths[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx" };
    char *address = (char *)params[0];
    char *mode = (char *)params[1];

    size_t vaddr = read_address2(address);// == '@' ? (size_t)cli_fetch(address, ST_ADDRESS) : cli_read_size(address);
    int access = (__parse_flags(mode) & VM_RWX) | VM_RD;

    int pg = 0;
    mmu_dir_t *dir = NULL;
    size_t idx = 0;

    vmsp_t *vmsp = memory_space_at(vaddr);
    if (vmsp != NULL) {
        dir = (void *)vmsp->directory;
        idx = (vaddr - vmsp->lower_bound) / PAGE_SIZE;
        pg = dir->pages[idx] & (VM_RWX | 8);
    }

    if ((int)(pg & access) != access) {
        printf("#PF at %p %s\n", (void *)vaddr, rigths[access]);
        bool missing = (pg & 8) == 0;
        bool writing = access & VM_WR;
        int ret = vmsp_resolve(vmsp, vaddr, missing, writing);
        if (ret != 0) {
            errno = ENOMEM;
            return -1;
        }
        if (vmsp != NULL)
            pg = dir->pages[idx] & (VM_RWX | 8);
    }

    if ((int)(pg & access) != access || (pg & VM_RD) == 0) {
        errno = EAGAIN;
        return -1;
    }

    if (access & VM_WR && vmsp != NULL)
        dir->pages[idx] |= 0x200;
    return 0;
}




//int do_exec(void *ctx, size_t *params)
//{
//    vmsp_t *mm = __mmu.uspace;
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

    dlproc_t *proc = dlib_proc();
    dlib_t *lib = dlib_create("kernel", NULL);
    ll_append(&proc->libs, &lib->node);
    lib->base = 0; 
    lib->length = 4 * _Mib_;
    __mmu.kspace->proc = proc;
    size_t no = 0x1000 / 0x40;
    dlib_add_symbol(proc, lib, "__errno_location", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "kprintf", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "memset", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "strdup", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "irq_disable", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "vfs_close_inode", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "vfs_mkdev", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "vfs_rmdev", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "vfs_inode", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "irq_register", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "irq_enable", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "__divdi3", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "itimer_create", (no++ * 0x40));
    dlib_add_symbol(proc, lib, "vfs_write", (no++ * 0x40));
    return 0;
}

int do_quit()
{
    memory_sweep();
    
    // mspace_sweep(__mmu.kspace);

    memory_info();

    return alloc_check();
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int __mmap(vmsp_t *vmsp, size_t *params)
{
    char *type = (char *)params[0];
    size_t length = cli_read_size((char *)params[1]);
    char *arg = (char *)params[2];
    char *name = (char *)params[3];

    if (vmsp == NULL)
        return cli_error("No user-space selected");

    int flags = __parse_type(type);
    if (flags == -1)
        return -1;

    flags |= __parse_flags(arg);

    size_t ptr = vmsp_map(vmsp, 0, length, NULL, 0, flags);
    if (name != NULL)
        cli_store(name, (void *)ptr, ST_ADDRESS);
    return ptr == 0 ? -1 : 0;
}

int __mmapx(vmsp_t *vmsp, size_t *params)
{
    char *type = (char *)params[0];
    size_t length = cli_read_size((char *)params[1]);
    char *arg = (char *)params[2];
    char *iname = (char *)params[3];
    size_t off = cli_read_size((char *)params[4]);
    char *name = (char *)params[5];

    if (vmsp == NULL)
        return cli_error("No user-space selected");

    int flags = __parse_typex(type);
    if (flags == -1)
        return -1;
    if (flags != VMA_PHYS && name == NULL)
        return cli_error("Unspecified file name");

    flags |= __parse_flags(arg);

    void *ino = NULL;
    if (iname != NULL && *iname != '-') {
        ino = vfs_search_ino(NULL, iname, NULL, true);
        if (ino == NULL)
            return cli_error("Can't open file '%s'", iname);
    }

    size_t ptr = vmsp_map(vmsp, 0, length, ino, off, flags);
    if (ino != NULL)
        vfs_close_inode(ino);
    if (name && ptr != 0)
        cli_store(name, (void *)ptr, ST_ADDRESS);
    return ptr == 0 ? -1 : 0;
}

int __munmap(vmsp_t *vmsp, size_t *params)
{
    char *bname = (char *)params[0];
    size_t base = read_address2(bname);
    size_t length = cli_read_size((char *)params[1]);

    if (vmsp == NULL)
        return cli_error("No user-space selected");

    return vmsp_unmap(vmsp, base, length);
}

int __mprotect(vmsp_t *vmsp, size_t *params)
{
    char *bname = (char *)params[0];
    size_t base = read_address2(bname);
    size_t length = cli_read_size((char *)params[1]);
    char *arg = (char *)params[2];

    if (vmsp == NULL)
        return cli_error("No user-space selected");

    int flags = __parse_flags(arg);
    return vmsp_protect(vmsp, base, length, flags); // !?
}

int __dlib(vmsp_t *vmsp, size_t *params)
{
    char *name = (char *)params[0];
    int ret = dlib_open(vmsp, NULL, NULL, name);
    if (ret != 0)
        cli_error("Error on loading library %s", name);
    return ret;
}

int __sym(vmsp_t *vmsp, size_t *params)
{
    char *symbol = (char *)params[0];
    char *name = (char *)params[1];

    void *ptr = dlib_sym(vmsp->proc, symbol);
    if (ptr)
        kprintf(-1, "Find symbol '%s' at %p\n", symbol, ptr);
    else 
        kprintf(-1, "Unable to find symbol '%s'\n", symbol);
    if (name)
        cli_store(name, ptr, ST_ADDRESS);
    //int ret = vmsp_resolve(__mmu.kspace, (size_t)ptr, true, false);
    //ret = vmsp_resolve(__mmu.kspace, (size_t)ptr, false, true);
    return ptr ? 0 : -1;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int do_kmap(void *ctx, size_t *params)
{
    return __mmap(__mmu.kspace, params);
}

int do_kmapx(void *ctx, size_t *params)
{
    return __mmapx(__mmu.kspace, params);
}

int do_kunmap(void *ctx, size_t *params)
{
    return __munmap(__mmu.kspace, params);
}

int do_kprotect(void *ctx, size_t *params)
{
    return __mprotect(__mmu.kspace, params);
}

int do_kdlib(void *ctx, size_t *params)
{
    return __dlib(__mmu.kspace, params);
}

int do_ksym(void *ctx, size_t *params)
{
    return __sym(__mmu.kspace, params);
}

int do_mmap(void *ctx, size_t *params)
{
    return __mmap(__mmu.uspace, params);
}

int do_mmapx(void *ctx, size_t *params)
{
    return __mmapx(__mmu.uspace, params);
}

int do_munmap(void *ctx, size_t *params)
{
    return __munmap(__mmu.uspace, params);
}

int do_mprotect(void *ctx, size_t *params)
{
    return __mprotect(__mmu.uspace, params);
}

int do_mdlib(void *ctx, size_t *params)
{
    return __dlib(__mmu.uspace, params);
}

int do_msym(void *ctx, size_t *params)
{
    return __sym(__mmu.uspace, params);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int do_uspace_create(void *ctx, size_t *params)
{
    char *store = (char *)params[0];
    vmsp_t *usp = vmsp_create();
    cli_store(store, usp, ST_MSPACE);
    __mmu.uspace = usp;
    return 0;
}

int do_uspace_open(void *ctx, size_t *params)
{
    char *store = (char *)params[0];
    if (__mmu.uspace == NULL)
        return cli_error("No user-space selected");
    vmsp_t *usp = vmsp_open(__mmu.uspace);
    cli_store(store, usp, ST_MSPACE);
    __mmu.uspace = usp;
    return 0;
}

int do_uspace_select(void *ctx, size_t *params)
{
    char *store = (char *)params[0];
    vmsp_t *usp = cli_fetch(store, ST_MSPACE);
    if (usp == NULL)
        return cli_error("No user-space selected");
    __mmu.uspace = usp;
    return 0;
}

int do_uspace_clone(void *ctx, size_t *params)
{
    char *store = (char *)params[0];
    if (__mmu.uspace == NULL)
        return cli_error("No user-space selected");
    vmsp_t *usp = vmsp_clone(__mmu.uspace);
    cli_store(store, usp, ST_MSPACE);
    __mmu.uspace = usp;
    return 0;
}

int do_uspace_close(void *ctx, size_t * params)
{
    char *store = (char *)params[0];
    vmsp_t *usp = cli_fetch(store, ST_MSPACE);
    if (usp == NULL)
        return cli_error("No user-space selected");
    if (usp != __mmu.uspace)
        return cli_error("Wromg user-space selected");
    vmsp_close(usp);
    __mmu.uspace = NULL;
    cli_remove(store, ST_MSPACE);
    return 0;
}

typedef struct pagesbuf
{
    int count;
    size_t pages[0];
} pagesbuf_t;
#define ST_PAGESBUF 5

int do_mmu_read(void *ctx, size_t *params)
{
    size_t vaddr = cli_read_size((char *)params[0]);
    int count = cli_read_size((char *)params[1]);
    char *store = (char *)params[2];

    pagesbuf_t *ptr = malloc(sizeof(pagesbuf_t) + count * sizeof(size_t));
    ptr->count = count;
    for (int i = 0; i < count; ++i) {
        ptr->pages[i] = mmu_read(vaddr);
        vaddr += PAGE_SIZE;
    }

    cli_store(store, ptr, ST_PAGESBUF);
    return 0;
}

int do_mmu_release(void *ctx, size_t *params)
{
    char *store = (char *)params[0];

    pagesbuf_t *ptr = cli_fetch(store, ST_PAGESBUF);
    for (int i = 0; i < ptr->count; ++i) {
        page_release(ptr->pages[i]);
    }
    cli_remove(store, ST_PAGESBUF);
    free(ptr);
    return 0;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


int do_del(void *cfg, size_t *params)
{
    char *name = (char *)params[0];
    void *address = cli_fetch(name, ST_ADDRESS);
    if (address == NULL)
        return cli_error("Unknwon address");
    cli_remove(name, ST_ADDRESS);
    return 0;
}

void initialize()
{
    // memory_initialize();
}

void teardown()
{
}


cli_cmd_t __commands[] = {
    { "START", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_start, 3 },
    { "QUIT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_quit, 0 },

    { "KMAP", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_kmap, 2 },
    { "KMAPX", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR }, (void *)do_kmapx, 5 },
    { "KUNMAP", "", { ARG_STR, ARG_STR, 0, 0, 0, 0 }, (void *)do_kunmap, 2 },
    { "KPROTECT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_kprotect, 3 },
    { "KDLIB", "", { ARG_STR, 0, 0, 0, 0, 0 }, (void *)do_kdlib, 1 },
    { "KSYM", "", { ARG_STR, ARG_STR, 0, 0, 0, 0 }, (void *)do_ksym, 1 },

    { "MMAP", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_mmap, 2 },
    { "MMAPX", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR }, (void *)do_mmapx, 5 },
    { "MUNMAP", "", { ARG_STR, ARG_STR, 0, 0, 0, 0 }, (void *)do_munmap, 2 },
    { "MPROTECT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_mprotect, 3 },
    { "MDLIB", "", { ARG_STR, 0, 0, 0, 0, 0 }, (void *)do_mdlib, 1 },
    { "MSYM", "", { ARG_STR, ARG_STR, 0, 0, 0, 0 }, (void *)do_msym, 1 },

    { "USPACE_CREATE", "", { ARG_STR, 0, 0, 0, 0, 0 }, (void *)do_uspace_create, 1 },
    { "USPACE_OPEN", "", { ARG_STR, 0, 0, 0, 0, 0 }, (void *)do_uspace_open, 1 },
    { "USPACE_SELECT", "", { ARG_STR, 0, 0, 0, 0, 0 }, (void *)do_uspace_select, 1 },
    { "USPACE_CLONE", "", { ARG_STR, 0, 0, 0, 0, 0 }, (void *)do_uspace_clone, 1 },
    { "USPACE_CLOSE", "", { ARG_STR, 0, 0, 0, 0, 0 }, (void *)do_uspace_close, 1 },
    
    { "SHOW", "", { ARG_STR, 0, 0, 0, 0, 0 }, (void *)do_show, 0 },
    { "TOUCH", "", { ARG_STR, ARG_STR, 0, 0, 0, 0 }, (void *)do_touch, 2 },

    // !?
    { "MMU_READ", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_mmu_read, 0 },
    { "MMU_RELEASE", "", { ARG_STR, 0, 0, 0, 0, 0 }, (void *)do_mmu_release, 0 },

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


    //{ "ELF", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_elf, 1 },
    { "DEL", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_del, 1 },
    CLI_END_OF_COMMANDS,
};

int main(int argc, char **argv)
{
    return cli_main(argc, argv, &_, initialize, teardown);
}
