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
#include <kernel/dlib.h>
#include <kora/hmap.h>
#include <kora/llist.h>
#include <string.h>
#include <errno.h>

proc_t kproc;


const char *proc_getenv(proc_t *proc, const char *name);


proc_t *dlib_process(vfs_t *fs, mspace_t *mspace)
{
    proc_t *proc = kalloc(sizeof(proc_t));
    // proc->execname = strdup(execname);
    proc->fs = fs;
    proc->acl = NULL;
    proc->env = "PATH=/usr/bin:/bin\n"
                "OS=Kora\n"
                "HOME=/home/root\n"
                "HOSTNAME=my-home-pc\n"
                "ARCH=x86\n"
                "LANG=en_US.UTF=8\n"
                "TZ=Europe/Paris\n"
                "PWD=/home/root\n"
                "USER=root\n"
                "UID=9fe14c6\n"
                "LD_LIBRARY_PATH=\n"
                "SHELL=krish\n";
    hmp_init(&proc->libs_map, 16);
    hmp_init(&proc->symbols, 16);
    proc->mspace = mspace;
    // TODO
    return proc;
}

fsnode_t *dlib_lookfor_all(proc_t *proc, const char *libname, const char *xpath)
{
    char *rl, *rd;
    char *dirlist;
    char *dirname;
    char *path = strdup(xpath);
    char *buf = kalloc(4096);
    for (dirlist = strtok_r(path, ";", &rl); dirlist; dirlist = strtok_r(NULL, ";", &rl)) {
        for (dirname = strtok_r(dirlist, ":", &rd); dirname; dirname = strtok_r(NULL, ":", &rd)) {

            snprintf(buf, 4096, "%s/%s", dirname, libname);
            fsnode_t *node = vfs_search(proc->fs, buf, proc->acl, true);
            if (node != NULL) {
                kfree(buf);
                kfree(path);
                return node;
            }
        }
    }
    kfree(buf);
    kfree(path);
    return NULL;
}


fsnode_t *dlib_lookfor(proc_t *proc, const char *libname, const char *env, const char *sys)
{
    fsnode_t *node = NULL;

    // if EXEC, not SHARED
    node = vfs_search(proc->fs, libname, proc->acl, true);

    if (node != NULL || strchr(libname, '/'))
        return node;

    if (proc->exec.rpath != NULL) {
        node = dlib_lookfor_all(proc, libname, proc->exec.rpath);
        if (node != NULL)
            return node;
    }

    if (!proc->req_set_uid) {
        const char *lpath = proc_getenv(proc, env);
        if (lpath != NULL) {
            node = dlib_lookfor_all(proc, libname, lpath);
            if (node != NULL)
                return node;
        }
    }

    return dlib_lookfor_all(proc, libname, sys);
}


int dlib_open(proc_t *proc, dynlib_t *dlib)
{
    assert(dlib->ino != NULL);
    // Read Elf file
    if (elf_parse(dlib) != 0) {
        assert(errno != 0);
        return -1;
    }

    // Look for dependancies
    dyndep_t *dep;
    for ll_each(&dlib->depends, dep, dyndep_t, node) {
        fsnode_t *node = dlib_lookfor(proc, dep->name, "LD_LIBRARY_PATH", "/usr/lib:/lib");
        if (node == NULL) {
            kprintf(-1, "\033[31mMissing library\033[0m %s\n", dep->name);
            // TODO - Missing library!
            errno = ENOENT;
            return -1;
        }

        dynlib_t *lib = hmp_get(&proc->libs_map, (char *)node->ino, sizeof(void *));
        if (lib != NULL) {
            lib->rcu++;
            dep->lib = lib;
            vfs_close_fsnode(node);
            continue;
        }

        lib = kalloc(sizeof(dynlib_t));
        lib->ino = vfs_open_inode(node->ino);
        lib->name = strdup(dep->name);
        dep->lib = lib;
        hmp_put(&proc->libs_map, (char *)lib->ino, sizeof(void *), lib);
        ll_enqueue(&proc->queue, &lib->node);
        vfs_close_fsnode(node);
    }
    return 0;
}


int dlib_openexec(proc_t *proc, const char *execname)
{
    dynlib_t *lib;
    // Look for executable file
    fsnode_t *node = dlib_lookfor(proc, execname, "PATH", "/usr/bin:/bin");
    if (node == NULL)
        return -1;
    // kprintf(-1, "Locate %s ino: %s\n", execname, vfs_inokey(node->ino, tmp));
    proc->exec.ino = vfs_open_inode(node->ino);
    proc->exec.name = strdup(execname);
    vfs_close_fsnode(node);

    // Load exec with libraries
    ll_enqueue(&proc->queue, &proc->exec.node);
    while (proc->queue.count_ > 0) {
        lib = ll_dequeue(&proc->queue, dynlib_t, node);
        ll_append(&proc->libraries, &lib->node);
        if (dlib_open(proc, lib) != 0) {
            dlib_destroy(&proc->exec);
            return -1;
        }
    }

    // Add libraries to process memory space
    for ll_each_reverse(&proc->libraries, lib, dynlib_t, node)
        dlib_rebase(proc, proc->mspace, lib);

    // Resolve symbols -- Might be done lazy!
    for ll_each(&proc->libraries, lib, dynlib_t, node) {
        if (!dlib_resolve_symbols(proc, lib)) {
            dlib_destroy(&proc->exec);
            // Missing symbols !?
            return -1;
        }
    }
    return 0;
}


void dlib_destroy(dynlib_t *lib)
{

}


void dlib_rebase(proc_t *proc, mspace_t *mspace, dynlib_t *lib)
{
    dynsym_t *symbol;
    void *base = NULL;
    // ASRL
    if (lib->base != 0)
        base = mspace_map(mspace, lib->base, lib->length, NULL, 0, VMA_EXEC | VM_RW | VMA_FIXED);

    while (base == NULL) {
        size_t addr = rand32() % (mspace->upper_bound - mspace->lower_bound);
        addr += mspace->lower_bound;
        addr = ALIGN_DW(addr, PAGE_SIZE);

        if (strcmp(lib->name, "vfat.ko") == 0)
            addr = 0xf8000000;
        else if (strcmp(lib->name, "isofs.ko") == 0)
            addr = 0xf7000000;
        else if (strcmp(lib->name, "ext2.ko") == 0)
            addr = 0xf6000000;
        else if (strcmp(lib->name, "ata.ko") == 0)
            addr = 0xf5000000;
        else if (strcmp(lib->name, "e1000.ko") == 0)
            addr = 0xf4000000;
        else if (strcmp(lib->name, "ps2.ko") == 0)
            addr = 0xf3000000;
        else if (strcmp(lib->name, "vga.ko") == 0)
            addr = 0xf2000000;
        else if (strcmp(lib->name, "vbox.ko") == 0)
            addr = 0xf1000000;
        else if (strcmp(lib->name, "ip4.ko") == 0)
            addr = 0xf0000000;
        else if (strcmp(lib->name, "libc.so") == 0)
            addr = 0x10000000;
        else if (strcmp(lib->name, "libopenlibm.so.3") == 0)
            addr = 0x11000000;
        else if (strcmp(lib->name, "libz.so.1") == 0)
            addr = 0x12000000;
        else if (strcmp(lib->name, "libpng.so") == 0)
            addr = 0x13000000;
        else if (strcmp(lib->name, "libgfx.so") == 0)
            addr = 0x14000000;
        else if (strcmp(lib->name, "libfreetype2.so") == 0)
            addr = 0x15000000;

        base = mspace_map(mspace, addr, lib->length, NULL, 0, VMA_EXEC | VM_RW | VMA_FIXED);
    }

    // List symbols
    kprintf(-1, "\033[94mRebase lib %s at %p (.text: %p)\033[0m\n", lib->name, base, (char *)base + lib->text_off);
    lib->base = (size_t)base;
    for ll_each(&lib->intern_symbols, symbol, dynsym_t, node) {
        symbol->address += (size_t)base;
        // kprintf(-1, " -> %s in %s at %p\n", symbol->name, lib->name, symbol->address);
        hmp_put(&proc->symbols, symbol->name, strlen(symbol->name), symbol);
    }
}

void *dlib_exec_entry(proc_t *proc)
{
    return (void *)(proc->exec.base + proc->exec.entry);
}

void *dlib_symbol_address(proc_t *proc, const char *name)
{
    dynsym_t *symbol;
    symbol = hmp_get(&proc->symbols, name, strlen(name));
    return symbol ? (void *)symbol->address : NULL;
}

void dlib_unload(proc_t *proc, mspace_t *mspace, dynlib_t *lib)
{
    mspace_unmap(mspace, lib->base, lib->length);
}


bool dlib_resolve_symbols(proc_t *proc, dynlib_t *lib)
{
    dynsym_t *sym;
    dynsym_t *symbol;
    int missing = 0;
    for ll_each(&lib->extern_symbols, symbol, dynsym_t, node) {
        if (symbol->flags & 0x80)
            continue;
        // Resolve symbol
        sym = hmp_get(&proc->symbols, symbol->name, strlen(symbol->name));
        if (sym == NULL) {
            missing++;
            kprintf(-1, "\033[31mMissing symbol\033[0m '%s' needed for %s\n", symbol->name, lib->name);
            continue;
            // return false;
        }
        symbol->address = sym->address;
        symbol->size = sym->size;
        // kprintf(-1, " <- %s at %p\n", symbol->name, symbol->address);
    }
    return missing == 0;
}

int dlib_map(dynlib_t *dlib, mspace_t *mspace)
{
    dynsec_t *sec;
    for ll_each(&dlib->sections, sec, dynsec_t, node) {

        // kprintf(-1, "Section %4x - %4x - %4x - %4x - %4x , %o\n", sec->lower, sec->upper,
        //         sec->start, sec->end, sec->offset, sec->rights);

        // Copy sections
        void *sbase = (void *)(dlib->base + sec->lower + sec->offset);
        size_t slen = sec->upper - sec->lower;
        memset(sbase, 0, slen);
        // kprintf(-1, "Section zero <%p,%x>\n", sbase, slen);
        unsigned i, n = (sec->upper - sec->lower) / PAGE_SIZE;
        for (i = 0; i < n; ++i) {
            if (i < sec->start / PAGE_SIZE)
                continue;
            if (i > sec->end / PAGE_SIZE)
                continue;
            size_t start = i == sec->start / PAGE_SIZE ? sec->start & (PAGE_SIZE - 1) : 0;
            size_t end = i == sec->end / PAGE_SIZE ? sec->end & (PAGE_SIZE - 1) : PAGE_SIZE;
            void *dst = (void *)(dlib->base + sec->lower + sec->offset + start + i * PAGE_SIZE);
            int lg = end - start;
            if (lg > 0) {
                uint8_t *page = kmap(PAGE_SIZE, dlib->ino, i * PAGE_SIZE + sec->lower, VMA_FILE | VM_RD);
                void *src = ADDR_OFF(page, start);
                // kprintf(-1, "Section copy <%p,%x (%p)>\n", dst, lg, ADDR_OFF(dst, lg));
                memcpy(dst, src, lg);
                kunmap(page, PAGE_SIZE);
            }
        }

        // kprintf(-1, "Section : %p - %x\n", sbase, slen);
        // kdump(sbase, slen);
    }

    // Relocations
    dynrel_t *reloc;
    // dynsym_t *symbol;
    for ll_each(&dlib->relocations, reloc, dynrel_t, node) {

        // kprintf(-1, "R: %06x  %x  %p  %s \n", reloc->address, reloc->type, reloc->symbol == NULL ? NULL : (void*)reloc->symbol->address, reloc->symbol == NULL ? "-" : reloc->symbol->name);
        size_t *place = (size_t *)(dlib->base + reloc->address);
        switch (reloc->type) {
        case R_386_32:
            *place += reloc->symbol->address;
            break;
        case R_386_PC32:
            *place += reloc->symbol->address - (size_t)place;
            break;
        case R_386_GLOB_DAT:
        case R_386_JUMP_SLOT:
            *place = reloc->symbol->address;
            break;
        case R_386_RELATIVE:
            *place += dlib->base;
            break;
        default:
            kprintf(-1, "REL:%d\n", reloc->type);
            break;
        }
        // kprintf(-1, " `- %08x  %x  \n", place, *place);

        // kprintf(-1, " -> %s at %p\n", reloc->symbol->name, reloc->symbol->address );
        // hmp_put(&proc->symbols, symbol->name, strlen(symbol->name), symbol);
        // TODO - Do not replace first occurence of a symbol.
    }

    // Change map access rights
    for ll_each(&dlib->sections, sec, dynsec_t, node) {
        size_t sbase = dlib->base + sec->lower + sec->offset;
        size_t slen = sec->upper - sec->lower;
        mspace_protect(mspace, sbase, slen, sec->rights & VM_RWX);
    }

    return 0;
}

int dlib_map_all(proc_t *proc)
{
    dynlib_t *lib;
    for ll_each(&proc->libraries, lib, dynlib_t, node)
        dlib_map(lib, proc->mspace);
    return 0;
}

const char *proc_getenv(proc_t *proc, const char *name)
{
    return NULL;
}
