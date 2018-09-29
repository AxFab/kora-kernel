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
#include <string.h>
#include <stdio.h>
#include "dlib.h"
#include "elf.h"
#include <windows.h>

#if defined _WIN32
int open(const char *name, int flags);
#else
int open(const char *name, int flags);
#endif
int read(int fd, char *buf, int len);
int write(int fd, const char *buf, int len);
int lseek(int fd, off_t off, int whence);
void close(int fd);
// sizintte_t fopen(const char *name, const char *mode);
int fwrite(void *buf, int e, int s, size_t fd);
int fseek(size_t fp, off_t off, int whence);
int fclose(size_t fd);


int *__errno_location();




#if !defined(_WIN32)
#define _valloc(s) valloc(s)
#define _vfree(p) free(p)
#else
#define _valloc(s) _aligned_malloc(s, PAGE_SIZE)
#define _vfree(p) _aligned_free(p)
#endif

int vfs_read()
{
    return -1;
}
int vfs_write()
{
    return -1;
}

void *bio_access(void *ptr, int lba)
{
    return ADDR_OFF(ptr, lba * PAGE_SIZE);
}

void bio_clean(void *ptr, int lba)
{
}

bio_t *bio_setup(CSTR name)
{
    FILE *fp;
    fopen_s(&fp, name, "rb");
    fseek(fp, 0, SEEK_END);
    int lg = ftell(fp);
    uint8_t *ptr = _valloc(ALIGN_UP(lg, PAGE_SIZE));
    memset(ptr, 0, ALIGN_UP(lg, PAGE_SIZE));
    fseek(fp, 0, SEEK_SET);
    fread(ptr, 1, lg, fp);
    return (bio_t *)ptr;
}


int vfs_mkdev() {
    printf("VFS\n");
}
int vfs_rmdev() {
    printf("VFS\n");
}
int vfs_inode() {
    printf("VFS\n");
}
int vfs_close() {
    printf("VFS\n");
}

#define SYMBOL(p,n) create_symbol(p,#n,&n,0,0)

void create_symbol(process_t *proc, CSTR name, void *address, size_t size, int flags)
{
    dynsym_t *sym = calloc(1, sizeof(dynsym_t));
    sym->address = address;
    sym->name = strdup(name);
    sym->size = size;
    sym->flags = flags;
    hmp_put(&proc->symbols, name, strlen(name), sym);
}

int main()
{
    process_t process;
    hmp_init(&process.symbols, 16);
    SYMBOL(&process, strlen);
    SYMBOL(&process, strdup);
    SYMBOL(&process, strcat);
    SYMBOL(&process, strcpy);
    SYMBOL(&process, kalloc);
    SYMBOL(&process, kfree);
    SYMBOL(&process, kprintf);

    SYMBOL(&process, irq_enable);
    SYMBOL(&process, irq_disable);


    SYMBOL(&process, strerror);
    SYMBOL(&process, snprintf);
    SYMBOL(&process, __errno_location);

    SYMBOL(&process, open);
    SYMBOL(&process, close);
    SYMBOL(&process, lseek);
    SYMBOL(&process, write);
    SYMBOL(&process, read);

    
    SYMBOL(&process, vfs_mkdev);
    SYMBOL(&process, vfs_rmdev);
    SYMBOL(&process, vfs_inode);
    SYMBOL(&process, vfs_close);

    dynlib_t dlib;
    memset(&dlib, 0, sizeof(dlib));
    dlib.name = strdup("imgdk.km");
    dlib.rcu = 1;
    dlib.io = bio_setup("imgdk.2.so");
    int ret = elf_parse(&dlib);

    // Look for all symbols !
    dynsym_t *sym;
    for ll_each(&dlib.extern_symbols, sym, dynsym_t, node) {
        dynsym_t *ext = hmp_get(&process.symbols, sym->name, strlen(sym->name));
        if (ext != NULL) {
            sym->address = ext->address;
            sym->size = ext->size;
            printf("Extern  %p  %s  \n", sym->address, sym->name);
        } else {
            printf("Missing symbol: %s\n", sym->name);
        }
    }

    // Find a map area
    void *base = VirtualAlloc(NULL, dlib.length, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    printf("Map at %p \n", base);

    dynsec_t *section;
    for ll_each(&dlib.sections, section, dynsec_t, node) {
        size_t start = (size_t)base + section->lower + section->offset;
        printf("Copy section at %p \n", start);
        int length = section->upper - section->lower;
        for (int i = 0, n = length / PAGE_SIZE; i < n; ++i) {
            int lba = i + section->lower / PAGE_SIZE;
            printf("    section page %p from lba %d\n", (size_t)base + section->lower + i * PAGE_SIZE + section->offset, lba);
            uint8_t *ptr = bio_access(dlib.io, lba);
            memcpy((char*)base + section->lower + i * PAGE_SIZE + section->offset, ptr, PAGE_SIZE);
            bio_clean(dlib.io, lba);
        }
        memset((void*)start, 0, section->start);
        memset((void*)(start + section->end), 0, length - section->end);
    }

    for ll_each(&dlib.intern_symbols, sym, dynsym_t, node) {
        sym->address += (size_t)base;
        hmp_put(&process.symbols, sym->name, strlen(sym->name), sym);
    }

    // Do relocation
    dynrel_t *rel;
    for ll_each(&dlib.relocations, rel, dynrel_t, node) {
        void* addr = (char*)base + rel->address;
        size_t value = base;
        size_t prev = ((uint32_t*)addr)[0];
        switch (rel->type) {
        case 8: // BASE + REG
            assert(rel->symbol == NULL);
            value = (size_t)base + prev;
            printf("REL [%p] <= %p (%p)  GLOBAL\n", addr, value, prev);
            break;
        case 6:
        case 7:
            assert(rel->symbol != NULL);
            value = (size_t)rel->symbol->address;
            printf("REL [%p] <= %p (%p)  %s \n", addr, value, prev, rel->symbol->name);
            break;
        case 1:
            assert(rel->symbol != NULL);
            value = (size_t)rel->symbol->address +  prev;
            printf("REL [%p] <= %p (%p)  %s \n", addr, value, prev, rel->symbol->name);
            break;
        default:
            printf("REL INVALID \n");
            break;
        }
        ((uint32_t*)addr)[0] = value;
    }

    for ll_each(&dlib.sections, section, dynsec_t, node) {
        size_t start = (size_t)base + section->lower + section->offset;
        int length = section->upper - section->lower;
        printf("Change rights for section at %p (%dKb) \n", start, length / 1024);
        int old = 0;
        // VirtualProtect(start, length, PAGE_EXECUTE_READWRITE, &old);
    }


    CSTR entry = "imgdk_setup";

    dynsym_t *ext = hmp_get(&process.symbols, entry, strlen(entry));
    void(*start)() = ext->address;
    start();

    return 0;
}
