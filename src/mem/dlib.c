#include <stddef.h>
#include <kernel/stdc.h>
#include <kora/llist.h>
#include <kora/hmap.h>
#include <kora/splock.h>
#include <kernel/memory.h>
#include <kernel/dlib.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static inode_t *dlib_lookfor_at(vfs_t *vfs, user_t *user, const char *name, const char *xpath)
{
    char *rl;
    char *path = kstrdup(xpath);
    char *buf = kalloc(4096);

    for (char *dir = strtok_r(path, ":;", &rl); dir; dir = strtok_r(NULL, ":;", &rl)) {
        snprintf(buf, 4096, "%s/%s", dir, name);
        inode_t *ino = vfs_search_ino(vfs, buf, user, true);
        if (ino != NULL) {
            kfree(buf);
            kfree(path);
            return ino;
        }
    }
    kfree(buf);
    kfree(path);
    return NULL;
}

static inode_t *dlib_lookfor(vfs_t *vfs, user_t *user, const char *name, const char *lpath, const char *rpath, const char *sys, bool req_uid)
{
    inode_t *ino = vfs_search_ino(vfs, name, user, true);
    if (ino != NULL || strchr(name, '/'))
        return ino;

    if (rpath != NULL) {
        ino = dlib_lookfor_at(vfs, user, name, rpath);
        if (ino != NULL)
            return ino;
    }

    if (!req_uid && lpath != NULL) {
        ino = dlib_lookfor_at(vfs, user, name, lpath);
        if (ino != NULL)
            return ino;
    }

    return  dlib_lookfor_at(vfs, user, name, sys);
}

int dlib_open(mspace_t *mm, vfs_t *vfs, user_t *user, const char *name)
{
    dlib_t *lib;
    if (mm->proc == NULL)
        mm->proc = dlib_proc();
    dlproc_t *proc = mm->proc;

    // Open executable
    proc->exec = dlib_create(name);
    const char *path = NULL; // proc_getenv(proc, "PATH");
    const char *lpath = NULL; // proc_getenv(proc, "LD_LIBRARY_PATH");
    inode_t *ino = dlib_lookfor(vfs, user, name, path, NULL, "/usr/bin:/bin", false);
    // proc->exec = dlib_create(name, ino);
    if (ino == NULL || dlib_parse(proc->exec, ino) != 0)
        return -1; // TODO -- proc->exec won't be clean by destroy!
    hmp_put(&proc->libs_map, proc->exec->name, strlen(proc->exec->name), proc->exec);

    // Open shared libraries
    bool req_uid = false;
    llhead_t queue = INIT_LLHEAD;
    ll_enqueue(&queue, &proc->exec->node);
    while (queue.count_ > 0) {
        lib = ll_dequeue(&queue, dlib_t, node);
        ll_append(&proc->libs, &lib->node);
        dlname_t *dep;
        for ll_each(&lib->depends, dep, dlname_t, node)
        {
            dlib_t *sl = hmp_get(&proc->libs_map, dep->name, strlen(dep->name));
            if (sl == NULL) {
                sl = dlib_create(dep->name);
                ino = dlib_lookfor(vfs, user, dep->name, lpath, proc->exec->rpath, "/usr/lib:/lib", req_uid);
                // sl = dlib_create(dep->name, ino);
                if (ino == NULL || dlib_parse(sl, ino) != 0) {
                    dlib_clean(NULL, sl);
                    // Missing library
                    return -1; // TODO -- Clean data on queue!
                }
                hmp_put(&proc->libs_map, dep->name, strlen(dep->name), sl);
                ll_enqueue(&queue, &sl->node);
            }
        }
    }

    // Resolve symbols
    for ll_each(&proc->libs, lib, dlib_t, node)
    {
        if (dlib_resolve(&proc->symbols_map, lib) != 0) {
            // Missing symbols
            return -1;
        }
    }

    // Map all libraries
    for ll_each_reverse(&proc->libs, lib, dlib_t, node)
    {
        if (dlib_rebase(mm, &proc->symbols_map, lib) != 0) {
            // Missing memory space
            return -1;
        }
    }

    //for ll_each(&proc->libs, lib, dlib_t, node) {
    //    if (dlib_relloc(mm, lib) != 0) {
    //        // Relloc error
    //        return -1; // TODO -- Unmap !?
    //    }
    //}

    return 0;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

dlproc_t *dlib_proc()
{
    dlproc_t *proc = kalloc(sizeof(dlproc_t));
    hmp_init(&proc->libs_map, 8);
    hmp_init(&proc->symbols_map, 8);
    splock_init(&proc->lock);
    return proc;
}

int dlib_destroy(dlproc_t *proc)
{
    while (proc->libs.count_ > 0) {
        dlib_t *lib = ll_dequeue(&proc->libs, dlib_t, node);
        hmp_remove(&proc->libs_map, lib->name, strlen(lib->name));
        dlib_clean(proc, lib);
    }

    int ret = 0; // proc->symbols_map.count == 0 ? 0 : -1;
    hmp_destroy(&proc->libs_map);
    hmp_destroy(&proc->symbols_map);
    // TODO -- proc->exec = NULL;
    kfree(proc);
    return ret;
}

dlib_t *dlib_create(const char *name)
{
    dlib_t *lib = kalloc(sizeof(dlib_t));
    lib->name = kstrdup(name);
    return lib;
}

void dlib_clean(dlproc_t *proc, dlib_t *lib)
{
    while (lib->intern_symbols.count_ > 0) {
        dlsym_t *symb = ll_dequeue(&lib->intern_symbols, dlsym_t, node);
        if (proc != NULL)
            hmp_remove(&proc->symbols_map, symb->name, strlen(symb->name));
        kfree(symb->name);
        kfree(symb);
    }
    // TODO -- Symbols / Section / Depends / Reloc...
    kfree(lib->name);
    kfree(lib);
}

int dlib_parse(dlib_t *lib, inode_t *ino)
{
    blkmap_t *blkmap = blk_open(ino, PAGE_SIZE);
    int ret = elf_parse(lib, blkmap);
    return ret;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#if !defined NDEBUG && defined KORA_KRN
static size_t dlib_override_map(const char *name, size_t base)
{
#define _UA(x)  (0x2000000 + (x) * 0x80000)
#define _KA(x)  (0xF0000000 + (x) * 0x80000)
    if (strcmp(name, "vfat.ko") == 0) return _KA(0);
    if (strcmp(name, "isofs.ko") == 0) return _KA(1);
    if (strcmp(name, "ext2.ko") == 0) return _KA(2);
    if (strcmp(name, "ata.ko") == 0) return _KA(3);
    if (strcmp(name, "e1000.ko") == 0) return _KA(4);
    if (strcmp(name, "ps2.ko") == 0) return _KA(5);
    if (strcmp(name, "vga.ko") == 0) return _KA(6);
    if (strcmp(name, "vbox.ko") == 0) return _KA(7);
    if (strcmp(name, "ip4.ko") == 0) return _KA(8);

    if (strcmp(name, "libc.so") == 0) return _UA(0);
    if (strcmp(name, "libopenlibm.so.3") == 0) return _UA(1);
    if (strcmp(name, "libz.so.1") == 0) return _UA(2);
    if (strcmp(name, "libz.so") == 0) return _UA(2);
    if (strcmp(name, "libpng.so") == 0) return _UA(3);
    if (strcmp(name, "libgfx.so") == 0) return _UA(4);
    if (strcmp(name, "libfreetype2.so") == 0) return _UA(5);
    return base;
}
#endif

int dlib_rebase(mspace_t *mm, hmap_t *symbols_map, dlib_t *lib)
{
    // Find map address
    void *base = NULL;

#if !defined NDEBUG && defined KORA_KRN
    lib->base = dlib_override_map(lib->name, lib->base);
    if (lib->base)
        base = mspace_map(mm, lib->base, lib->length, (void *)lib, 0, VMA_CODE | VMA_FIXED | VM_RWX);
#endif

    // Use ASRL
    int max = (mm->upper_bound - lib->length - mm->lower_bound) / PAGE_SIZE;
    while (base == NULL && true) { // TODO -- When to give up
        size_t addr = mm->lower_bound + (rand32() % max) * PAGE_SIZE;
        base = mspace_map(mm, addr, lib->length, (void *)lib, 0, VMA_CODE | VMA_FIXED | VM_RWX);
    }

    if (base == NULL)
        return -1;

    kprintf(-1, "\033[94mRebase lib %s at %p (.text: %p)\033[0m\n", lib->name, base, (char *)base + lib->text_off);
    lib->base = (size_t)base;

    // Splt map protection
    dlsection_t *sc;
    for ll_each(&lib->sections, sc, dlsection_t, node) {
        if (sc->length == 0)
            break;
        mspace_protect(mm, lib->base + sc->offset, sc->length, sc->rights);
    }

    // List symbols
    dlsym_t *symbol;
    for ll_each(&lib->intern_symbols, symbol, dlsym_t, node) {
        symbol->address += (size_t)base;
        hmp_put(symbols_map, symbol->name, strlen(symbol->name), symbol);
    }
    return 0;
}

int dlib_resolve(hmap_t *symbols_map, dlib_t *lib)
{
    dlsym_t *symb;
    dlsym_t *symbol;
    int missing = 0;
    for ll_each(&lib->extern_symbols, symbol, dlsym_t, node)
    {
        if (symbol->flags & 0x80)
            continue;
        symb = hmp_get(symbols_map, symbol->name, strlen(symbol->name));
        if (symb == NULL) {
            missing++;
            kprintf(-1, "\033[31mMissing symbol\033[0m '%s' needed for %s\n", symbol->name, lib->name);
            continue;

        }
        symbol->address = symb->address;
        symbol->size = symb->size;
    }
    return missing == 0 ? 0 : -1;
}

int dlib_relloc(mspace_t *mm, dlib_t *lib)
{
    int pages = lib->length / PAGE_SIZE;
    lib->pages = kalloc(sizeof(size_t) * pages);
    void *ptr = kmap(lib->length, NULL, 0, VMA_PHYS | VM_RW);
    dlsection_t *sc;
    for ll_each(&lib->sections, sc, dlsection_t, node) {
        if (sc->length == 0)
            break;
        memset(ADDR_OFF(ptr, sc->offset + sc->csize), 0, sc->length - sc->csize);
        vfs_read(lib->ino, ADDR_OFF(ptr, sc->offset), sc->csize, sc->foff, 0);
    }

    for (int i = 0; i < pages; ++i) {
        lib->pages[i] = mmu_read(mm, (size_t)ptr + i * PAGE_SIZE);
    }

    dlreloc_t *reloc;
    for ll_each(&lib->relocations, reloc, dlreloc_t, node)
    {
        size_t *place = ADDR_OFF(ptr, reloc->offset);
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
            *place += lib->base;
            break;
        default:
            kprintf(-1, "REL:%d\n", reloc->type);
            break;
        }
    }

    kunmap(ptr, lib->length);
    return 0;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void *dlib_sym(dlproc_t *proc, const char *symbol)
{
    if (proc == NULL)
        return NULL;
    splock_lock(&proc->lock);
    dlsym_t *symb = hmp_get(&proc->symbols_map, symbol, strlen(symbol));
    size_t addr = symb ? symb->address : 0;
    splock_unlock(&proc->lock);
    return (void *)addr;
}

size_t dlib_fetch_page(dlib_t *lib, size_t off)
{
    lib->page_rcu++;
    int idx = off / PAGE_SIZE;
    size_t pg = lib->pages[idx];
    if (pg == 0) {
        // TODO -- !?
    }
    return pg;
}

void dlib_release_page(dlib_t *lib, size_t off, size_t pg)
{

    lib->page_rcu--;
}

char *dlib_name(dlib_t *lib, char *buf, int len)
{
    strncpy(buf, lib->name, len);
    return buf;
}
