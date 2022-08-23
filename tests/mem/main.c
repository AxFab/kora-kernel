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

struct
{
    int page_used;
    size_t lower;
    size_t upper;
    mspace_t *mm;
    size_t *pages;
} _;

char *pg_map = NULL;
size_t pg_base = 0;
size_t pg_max = 0;

void mmu_enable() 
{
    kMMU.free_pages = 8192;
    kMMU.pages_amount = 8192;
    kMMU.upper_physical_page = 8192;

    kMMU.kspace->directory = 0;
    kMMU.kspace->lower_bound = 128 * _Mib_;
    kMMU.kspace->upper_bound = 256 * _Mib_;
    kMMU.kspace->t_size++;
    kMMU.free_pages--;

    _.lower = 4 * _Mib_;
    _.upper = 64 * _Mib_;

    pg_max = 512;
    pg_map = kalloc(pg_max);
    memset(pg_map, 0, pg_max);
    pg_base = 0x100000;
}

void mmu_leave()
{
    kMMU.kspace->t_size--;
    kMMU.free_pages++;
}

void mmu_context(mspace_t *mm)
{
    _.pages = (size_t *)mm->directory;
}

void mmu_create_uspace(mspace_t *mm) 
{
    mm->lower_bound = 4 * _Mib_;
    mm->upper_bound = 64 * _Mib_;
    int count = (mm->upper_bound - mm->lower_bound) / PAGE_SIZE;
    mm->directory = (size_t)calloc(sizeof(size_t), count);
    mm->t_size++;
    kMMU.free_pages--;
}

void mmu_destroy_uspace(mspace_t *mm) 
{
    int count = (mm->upper_bound - mm->lower_bound) / PAGE_SIZE;
    size_t *pages = (size_t*)mm->directory;
    for (int i = 0; i < count; ++i) {

    }
    mm->t_size--;
    kMMU.free_pages++;
}



void page_release_kmap_stub(page_t page);
void page_release(page_t page)
{
    if (page < pg_base || page >= pg_base + pg_max * PAGE_SIZE)
        page_release_kmap_stub(page);
    int idx = (page - pg_base) / PAGE_SIZE;
    assert(pg_map[idx] != 0);
    pg_map[idx] = 0;
}

size_t page_new()
{
    char *p = memchr(pg_map, 0, pg_max);
    assert(p != NULL); // TODO -- Clean error !?
    int idx = p - pg_map;
    assert(pg_map[idx] == 0);
    pg_map[idx] = 1;
    return pg_base + idx * PAGE_SIZE;
}

/* - */
int mmu_resolve(size_t vaddr, page_t phys, int flags) 
{
    int all = 0;
    int idx = (vaddr - _.lower) / PAGE_SIZE;
    if (phys == 0) {
        phys = page_new();
    }
    _.pages[idx] = phys | (flags & 7);
    return all;
}
/* - */
page_t mmu_read_kmap_stub(size_t address);
page_t mmu_read(size_t vaddr)
{
    size_t pg = mmu_read_kmap_stub(vaddr);
    if (pg != 0)
        return pg;

    int idx = (vaddr - _.lower) / PAGE_SIZE;
    return _.pages[idx] & ~(PAGE_SIZE -1);
}

int mmu_read_flags(size_t vaddr)
{
    int idx = (vaddr - _.lower) / PAGE_SIZE;
    return _.pages[idx] & (PAGE_SIZE - 1);
}

/* - */
page_t mmu_drop(size_t vaddr) 
{
    int idx = (vaddr - _.lower) / PAGE_SIZE;
    size_t phys = _.pages[idx] & ~(PAGE_SIZE - 1);
    _.pages[idx] = 0;
    return phys;
}

/* - */
page_t mmu_protect(size_t vaddr, int flags)
{
    int idx = (vaddr - _.lower) / PAGE_SIZE;
    size_t phys = _.pages[idx] & ~(PAGE_SIZE - 1);
    _.pages[idx] = phys | (flags & 7);
}
/* - */
bool mmu_dirty(size_t vaddr) 
{
    return false;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

char *vfs_inokey(inode_t *ino, char *buf) {}
inode_t *vfs_open_inode(inode_t *ino) {}
void vfs_close_inode(inode_t *ino) {}
size_t vfs_fetch_page(inode_t *ino, xoff_t off) {}
int vfs_release_page(inode_t *ino, xoff_t off, size_t pg, bool dirty) {}
inode_t *vfs_search_ino(vfs_t *vfs, const char *name, user_t *user, bool follow) {}
int vfs_read(inode_t *ino, char *buf, size_t length, xoff_t off, int flags)
{
    return length;
}

void task_fatal(const char *msg, int signum)
{
    printf("%s\n", msg);
}

#define OBJ_MSPACE 1
#define OBJ_ADDRESS 2

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

size_t read_size(char *str)
{
    if (str == NULL)
        return 0;
    float len = strtod(str, &str);
    int c = *str | 0x20;
    if (c == 'k')
        return len * 1024UL;
    if (c == 'm')
        return len * 1024UL * 1024UL;
    if (c == 'g')
        return len * 1024UL * 1024UL * 1024UL;
    if (c == 't')
        return len * 1024UL * 1024UL * 1024UL * 1024UL;
    return len * 1UL;
}

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
            vaddr = (size_t)cli_fetch(name, OBJ_ADDRESS) - vaddr;
        else
            vaddr += (size_t)cli_fetch(name, OBJ_ADDRESS);
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

int do_create(void *ctx, size_t *params)
{
    char *name = (char *)params[0];
    mspace_t *mm = cli_fetch(name, OBJ_MSPACE);
    if (mm != NULL)
        return cli_error("A virtual space of this name already exists\n");
    _.mm = mspace_create();
    mmu_context(_.mm);
    cli_store(name, _.mm, OBJ_MSPACE);
    return 0;
}

int do_open(void *ctx, size_t *params)
{
    char *name = (char *)params[0];
    if (_.mm == NULL)
        return cli_error("A virtual space must already be active\n");
    mspace_t *mm = mspace_open(_.mm);
    cli_store(name, mm, OBJ_MSPACE);
    return 0;
}

int do_close(void *ctx, size_t *params)
{
    char *name = (char *)params[0];
    mspace_t *mm = cli_fetch(name, OBJ_MSPACE);
    if (mm == NULL)
        return cli_error("The virtual space must already exist\n");
    if (_.mm == mm)
        _.mm = NULL;
    cli_remove(name, OBJ_MSPACE);
    mspace_close(_.mm);
}

int do_clone(void *ctx, size_t *params)
{
    char *name = (char *)params[0];
    if (_.mm == NULL)
        return cli_error("A virtual space must already be active\n");
    mspace_t *mm = mspace_clone(_.mm);
    cli_store(name, mm, OBJ_MSPACE);
    return 0;
}

int do_show(void *ctx, size_t *params)
{
    mspace_t *mm = _.mm;
    char *checks = (char *)params[0];
    mspace_display(mm);

    if (checks == NULL)
        return 0;

    int err = 0;
    while (*checks) {
        char x = *(checks++);
        size_t sz = strtod(checks, &checks);
        char c = (*checks++) | 0x20;
        if (c == 'k')
            sz *= 1024UL;
        else if (c == 'm')
            sz *= 1024UL * 1024UL;
        else if (c == 'g')
            sz *= 1024UL * 1024UL * 1024UL;
        else if (c == 't')
            sz *= 1024UL * 1024UL * 1024UL * 1024UL;
        else
            checks--;

        if ((x | 0x20) == 'p' && sz != mm->p_size * PAGE_SIZE) {
            err++;
            cli_warn("Bad number of private page\n");
        } else if ((x | 0x20) == 'v' && sz != mm->v_size * PAGE_SIZE) {
            err++;
            cli_warn("Bad number of virtual page\n");
        } else if ((x | 0x20) == 's' && sz != mm->s_size * PAGE_SIZE) {
            err++;
            cli_warn("Bad number of shared page\n");
        }

        if (*checks == ',')
            checks++;
        else if (*checks != '\0' && *checks != ',')
            return cli_error("Bad checks parameter expect ','\n");

    }
    return err == 0 ? 0 : -1;
}


int do_map(void *ctx, size_t *params)
{
    mspace_t *mm = _.mm;
    size_t sz = read_size((char *)params[0]);
    char *name = (char *)params[1];
    char *type = (char *)params[2];
    char *access = (char *)params[3];
    char *mode = (char *)params[4];

    void *ptr = mspace_map(mm, NULL, sz, NULL, 0, VMA_STACK | VM_RW);
    cli_store(name, ptr, OBJ_ADDRESS);
    printf("Map stack at %p\n", ptr);
    return 0;
}

int do_unmap(void *ctx, size_t *params)
{
    mspace_t *mm = _.mm;
    size_t vaddr = read_address((char *)params[0]);
    size_t sz = read_size((char *)params[1]);
    int ret = mspace_unmap(mm, vaddr, sz);
    return ret;
}

int do_protect(void *ctx, size_t *params)
{
    mspace_t *mm = _.mm;
    size_t vaddr = read_address((char *)params[0]);
    size_t sz = read_size((char *)params[1]);
    int access = read_access((char *)params[2]);
    int ret = mspace_protect(mm, vaddr, sz, access);
    return ret;
}

int do_stack(void *ctx, size_t *params)
{
    mspace_t *mm = _.mm;
    size_t sz = read_size((char *)params[0]);
    char *name = (char *)params[1];

    void *ptr = mspace_map(mm, NULL, sz, NULL, 0, VMA_STACK | VM_RW);
    cli_store(name, ptr, OBJ_ADDRESS);
    printf("Map stack at %p\n", ptr);
    return 0;
}

int do_heap(void *ctx, size_t *params)
{
    mspace_t *mm = _.mm;
    size_t sz = read_size((char *)params[0]);
    char *name = (char *)params[1];

    void *ptr = mspace_map(mm, NULL, sz, NULL, 0, VMA_HEAP | VM_RW);
    cli_store(name, ptr, OBJ_ADDRESS);
    printf("Map heap at %p\n", ptr);
    return 0;
}

int do_exec(void *ctx, size_t *params)
{
    mspace_t *mm = _.mm;
    char *name = (char *)params[0];
    size_t cd = read_size((char *)params[1]);
    size_t dt = read_size((char *)params[2]);

    dlib_t *lib = dlib_create(name);
    dlsection_t *sc = kalloc(sizeof(dlsection_t));
    sc->length = cd;
    sc->rights = 5;
    ll_append(&lib->sections, &sc->node);

    sc = kalloc(sizeof(dlsection_t));
    sc->offset = cd;
    sc->length = dt;
    sc->rights = 6;
    ll_append(&lib->sections, &sc->node);

    lib->length = cd + dt;

    int ret = dlib_relloc(lib);
    ret = dlib_rebase(mm, NULL, lib);
    return ret;
}

int do_touch(void *ctx, size_t *params)
{
    const char *rigths[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx" };
    char* address = (char *)params[0];
    char *mode = (char*)params[1];

    size_t vaddr = read_address(address);
    int access = read_access(mode);

    if (vaddr < _.lower || vaddr >= _.upper)
        return -1;

    int idx = (vaddr - _.lower) / PAGE_SIZE;
    size_t pg = _.pages[idx];

    if ((pg & access) != access) {
        int flt = 0;
        if ((pg & 4) == 0)
            flt |= PGFLT_MISSING;
        if (access & VM_WR)
            flt |= PGFLT_WRITE;

        printf("#PF at %p %s\n", (void*)vaddr, rigths[access & 7]);
        page_fault(_.mm, vaddr, flt);
        pg = _.pages[idx];
    }

    if ((pg & access) != access || (pg & VM_RD) == 0)
        return -1;

    if (access & VM_WR)
        _.pages[idx] |= 0x20;
    return 0;
}


int do_elf(void *ctx, size_t *params)
{
    char tmp[32];
    char *name = (char *)params[0];

    char *lname = strrchr(name, '/');
    dlib_t *lib = dlib_create(lname ? lname + 1 : name);
    blkmap_t *bkm = blk_host_open(name, 4096);
    int ret = elf_parse(lib, bkm);
    printf("Reading elf file %s: %s", dlib_name(lib, tmp, 32), ret == 0 ? "succeed" : "fails");
    dlib_clean(lib);
    return ret;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void initialize()
{
    memory_initialize();
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

    { "CREATE", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_create, 0 },
    { "OPEN", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_open, 0 },
    { "CLONE", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_clone, 0 },
    { "CLOSE", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_close, 0 },
    { "SHOW", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_show, 0 },
    { "MAP", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, ARG_STR }, (void *)do_map, 5 },
    { "UNMAP", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_unmap, 2 },
    { "PROTECT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_protect, 3 },
    { "STACK", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_stack, 2 },
    { "HEAP", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_heap, 2 },
    { "EXEC", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, 0 }, (void *)do_exec, 2 },
    { "TOUCH", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void*)do_touch, 2 },
 
    { "ELF", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_elf, 1 },
    CLI_END_OF_COMMANDS,
};

int main(int argc, char **argv)
{
    return cli_main(argc, argv, &_, initialize, teardown);
}
