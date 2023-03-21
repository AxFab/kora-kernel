#include <stddef.h>
#include <kernel/stdc.h>
#include <kora/llist.h>
#include <kora/hmap.h>
#include <kora/splock.h>
#include <kernel/memory.h>
#include <kernel/dlib.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static inode_t *dlib_lookfor_at(fs_anchor_t *fsanchor, user_t *user, const char *name, const char *xpath)
{
    char *rl;
    char *path = kstrdup(xpath);
    char *buf = kalloc(4096);

    for (char *dir = strtok_r(path, ":;", &rl); dir; dir = strtok_r(NULL, ":;", &rl)) {
        snprintf(buf, 4096, "%s/%s", dir, name);
        inode_t *ino = vfs_search_ino(fsanchor, buf, user, true);
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

static inode_t *dlib_lookfor(fs_anchor_t *fsanchor, user_t *user, const char *name, const char *lpath, const char *rpath, const char *sys, bool req_uid)
{
    inode_t *ino = vfs_search_ino(fsanchor, name, user, true);
    if (ino != NULL || strchr(name, '/'))
        return ino;

    if (rpath != NULL) {
        ino = dlib_lookfor_at(fsanchor, user, name, rpath);
        if (ino != NULL)
            return ino;
    }

    if (!req_uid && lpath != NULL) {
        ino = dlib_lookfor_at(fsanchor, user, name, lpath);
        if (ino != NULL)
            return ino;
    }

    return  dlib_lookfor_at(fsanchor, user, name, sys);
}

int dlib_open(vmsp_t *vmsp, fs_anchor_t *fsanchor, user_t *user, const char *name)
{
    dlib_t *lib;
    if (vmsp->proc == NULL)
        vmsp->proc = dlib_proc();
    dlproc_t *proc = vmsp->proc;

    // Open executable
    const char *path = NULL; // proc_getenv(proc, "PATH");
    const char *lpath = NULL; // proc_getenv(proc, "LD_LIBRARY_PATH");
    inode_t *ino = dlib_lookfor(fsanchor, user, name, path, NULL, "/usr/bin:/bin", false);
    if (ino == NULL)
        return -1;
    proc->exec = dlib_create(name, ino);
    vfs_close_inode(ino);
    if (dlib_parse(proc, proc->exec) != 0) {
        dlib_clean(proc, proc->exec);
        return -1;
    }

    // Open shared libraries
    bool req_uid = false;
    llhead_t queue = INIT_LLHEAD;
    llhead_t ready = INIT_LLHEAD;
    ll_enqueue(&queue, &proc->exec->node);
    while (queue.count_ > 0) {
        lib = ll_dequeue(&queue, dlib_t, node);
        ll_append(&ready, &lib->node);
        dlname_t *dep;
        for ll_each(&lib->depends, dep, dlname_t, node)
        {
            dlib_t *sl = hmp_get(&proc->libs_map, dep->name, strlen(dep->name));
            if (sl == NULL) {
                ino = dlib_lookfor(fsanchor, user, dep->name, lpath, proc->exec->rpath, "/usr/lib:/lib", req_uid);
                if (ino == NULL)
                    return -1;
                sl = dlib_create(dep->name, ino);
                vfs_close_inode(ino);
                if (dlib_parse(proc, sl) != 0) {
                    dlib_clean(proc, proc->exec);
                    return -1;
                }
                ll_enqueue(&queue, &sl->node);
            }
        }
    }

    // Add to the processus
    while (ready.count_ > 0) {
        lib = ll_dequeue(&ready, dlib_t, node);
        ll_append(&proc->libs, &lib->node);
    }
    

    // Resolve symbols
    for ll_each(&proc->libs, lib, dlib_t, node)
    {
        if (lib->resolved == true)
            continue;
        if (dlib_resolve(&proc->symbols_map, lib) != 0) {
            // Missing symbols
            return -1;
        }
        lib->resolved = true;
    }

    // Map all libraries
    for ll_each_reverse(&proc->libs, lib, dlib_t, node)
    {
        if (lib->mapped == true || lib->resolved == false)
            continue;
        if (dlib_rebase(vmsp, &proc->symbols_map, lib) != 0) {
            // Missing memory space
            return -1;
        }
        lib->resolved = false;
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
    might_sleep();
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

dlib_t *dlib_create(const char *name, inode_t *ino)
{
    might_sleep();
    dlib_t *lib = kalloc(sizeof(dlib_t));
    mtx_init(&lib->mtx, mtx_plain);
    lib->name = kstrdup(name);
    if (ino != NULL)
        lib->ino = vfs_open_inode(ino);
    return lib;
}

extern void page_release_kmap_stub(page_t page);

void dlib_clean(dlproc_t *proc, dlib_t *lib)
{
    might_sleep();
    hmp_remove(&proc->libs_map, lib->name, strlen(lib->name));

    while (lib->relocations.count_ > 0) {
        dlreloc_t *rel = ll_dequeue(&lib->relocations, dlreloc_t, node);
        kfree(rel);
    }

    while (lib->intern_symbols.count_ > 0) {
        dlsym_t *symb = ll_dequeue(&lib->intern_symbols, dlsym_t, node);
        if (proc != NULL)
            hmp_remove(&proc->symbols_map, symb->name, strlen(symb->name));
        kprintf(-1, "del symb '%s' %p\n", symb->name, symb);
        kfree(symb->name);
        kfree(symb);
    }

    while (lib->extern_symbols.count_ > 0) {
        dlsym_t *symb = ll_dequeue(&lib->extern_symbols, dlsym_t, node);
        kprintf(-1, "del symb '%s' %p\n", symb->name, symb);
        kfree(symb->name);
        kfree(symb);
    }

    while (lib->sections.count_ > 0) {
        dlsection_t *sec = ll_dequeue(&lib->sections, dlsection_t, node);
        kfree(sec);
    }

    while (lib->depends.count_ > 0) {
        dlname_t *dep = ll_dequeue(&lib->depends, dlname_t, node);
        kfree(dep->name);
        kfree(dep);
    }

    if (lib->ino != NULL)
        vfs_close_inode(lib->ino);


    if (lib->pages != NULL) {
        int size = ALIGN_UP(lib->length, PAGE_SIZE) / PAGE_SIZE;
        for (int i = 0; i < size; ++i) {
            if (lib->pages[i] != 0)
#ifdef KORA_KRN
                page_release(lib->pages[i]);
#else
                page_release_kmap_stub(lib->pages[i]);
#endif
        }
        kfree(lib->pages);
    }

    kfree(lib->name);
    kfree(lib);
}

int dlib_parse(dlproc_t *proc, dlib_t *lib)
{
    blkmap_t *blkmap = blk_open(lib->ino, PAGE_SIZE);
    int ret = elf_parse(lib, blkmap);
    if (ret == 0)
        hmp_put(&proc->libs_map, lib->name, strlen(lib->name), lib);
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

int dlib_rebase(vmsp_t *vmsp, hmap_t *symbols_map, dlib_t *lib)
{
    // Find map address
    size_t base = 0;

#if !defined NDEBUG && defined KORA_KRN
    lib->base = dlib_override_map(lib->name, lib->base);
    if (lib->base)
        base = vmsp_map(vmsp, lib->base, lib->length, (void *)lib, 0, VMA_DLIB | VMA_FIXED | VM_RWX);
#endif
    // Use ASRL
    int max = (vmsp->upper_bound - lib->length - vmsp->lower_bound) / PAGE_SIZE;
    while (base == 0 && true) { // TODO -- When to give up
        size_t addr = vmsp->lower_bound + (rand32() % max) * PAGE_SIZE;
        base = vmsp_map(vmsp, addr, lib->length, (void *)lib, 0, VMA_DLIB | VMA_FIXED | VM_RWX);
    }

    if (base == 0)
        return -1;

    kprintf(-1, "\033[94mRebase lib %s at %p (.text: %p)\033[0m\n", lib->name, base, (char *)base + lib->text_off);
    lib->base = base;

    // Splt map protection
    dlsection_t *sc;
    for ll_each(&lib->sections, sc, dlsection_t, node) {
        if (sc->length == 0)
            break;
        vmsp_protect(vmsp, lib->base + sc->offset, sc->length, sc->rights);
    }

    // List symbols
    dlsym_t *symbol;
    for ll_each(&lib->intern_symbols, symbol, dlsym_t, node) {
        symbol->address += base;
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

int dlib_relloc_page(dlib_t *lib, char *ptr, xoff_t off)
{
    // Assert lib is locked
    dlreloc_t *reloc;
    for ll_each(&lib->relocations, reloc, dlreloc_t, node)
    {
        if ((xoff_t)reloc->offset < off || (xoff_t)reloc->offset >= off + PAGE_SIZE)
            continue;
        if ((xoff_t)reloc->offset >= off + PAGE_SIZE - (xoff_t)sizeof(size_t)) {
            kprintf(-1, "Hope I don't have to support this !\n");
            continue;
        }

        size_t *place = ADDR_OFF(ptr, reloc->offset - off);
        //if (reloc->symbol)
        //    kprintf(-1, "REL:%d at %p (s:%s:%p)\n", reloc->type, place, reloc->symbol->name, reloc->symbol->address);
        //else
        //   kprintf(-1, "REL:%d at %p\n", reloc->type, place);
        switch (reloc->type) {
        case R_386_32: // S + A
            *place += reloc->symbol->address;
            break;
        case R_386_PC32: // S + A - P
            *place += reloc->symbol->address - (size_t)place;
            break;
            // 3: R_386_GOT32     :  G + A - P
            // *place += G(GOT offset) - (size_t)place;
            // 4: R_386_PLT32     :  L + A - P
            // *place += L(PLT offset)  - (size_t)place;
            // 5: R_386_COPY      : none
        case R_386_GLOB_DAT:
        case R_386_JUMP_SLOT:
            *place = reloc->symbol->address;
            break;
        case R_386_RELATIVE:
            *place += lib->base; // B + A
            break;
            // 9: R_386_GOTOFF      : S + A - GOT    *place += reloc->symbol->address - GOT(address)
            // 10: R_386_GOTPC      : GOT + A - P    *place += GOT(address) - (size_t)place;
        default:
            kprintf(-1, "REL:%d at %p\n", reloc->type, place);
            break;
        }
    }
    return 0;
}

#if 0
int dlib_relloc(vmsp_t *vmsp, dlib_t *lib)
{
    // TODO -- Assert mm is currently used!
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
        lib->pages[i] = mmu_read((size_t)ptr + i * PAGE_SIZE);
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
        case 12:
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
#endif

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void dlib_add_symbol(dlproc_t *proc, dlib_t *lib, const char *name, size_t value)
{
    dlsym_t *symbol = kalloc(sizeof(dlsym_t));
    symbol->name = kstrdup(name);
    symbol->address = value;
    splock_lock(&proc->lock);
    if (lib != NULL)
        ll_append(&lib->intern_symbols, &symbol->node);
    hmp_put(&proc->symbols_map, name, strlen(name), symbol);
    splock_unlock(&proc->lock);
}


const char *dlib_rev_ksymbol(dlproc_t *proc, size_t ip, char *buf, int lg)
{
    dlib_t *lib;
    dlsym_t *sym;
    dlsym_t *best = NULL;
    for ll_each(&proc->libs, lib, dlib_t, node) {
        if (ip < lib->base || ip > lib->base + lib->length)
            continue;
        for ll_each(&lib->intern_symbols, sym, dlsym_t, node) {
            if (ip < sym->address)
                continue;
            if (sym->size != 0 && ip < sym->address + sym->size) {
                best = sym;
                break;
            }
            if (best == NULL || best->address < sym->address)
                best = sym;
        }
        if (best != NULL) {
            if (best->address == ip)
                snprintf(buf, lg, "%s", best->name);
            else if (best->size != 0 && ip < best->address + best->size)
                snprintf(buf, lg, "%s (+%ld)", best->name, best->address - ip);
            else
                snprintf(buf, lg, "> %s (+%ld)", best->name, best->address - ip);
            return buf;
        }
    }
    snprintf(buf, lg, "%p", (void*)ip);
    return buf;
}


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

static dlsection_t *dlib_section_at(dlib_t *lib, size_t off)
{
    dlsection_t *sec;
    for ll_each(&lib->sections, sec, dlsection_t, node)
    {
        if (sec->offset <= off && sec->offset + sec->length > off)
            return sec;
    }
    return NULL;
}

size_t dlib_fetch_page(dlib_t *lib, size_t off, bool blocking)
{
    mtx_lock(&lib->mtx);
    int idx = off / PAGE_SIZE;
    if (lib->pages == NULL) {
        if (!blocking) {
            mtx_unlock(&lib->mtx);
            return 0;
        }
        int size = ALIGN_UP(lib->length, PAGE_SIZE) / PAGE_SIZE;
        lib->pages = kalloc(size * sizeof(size_t));
    }
    size_t pg = lib->pages[idx];
    if (pg == 0 && blocking) {
        dlsection_t *sec = dlib_section_at(lib, off);
        assert(sec != NULL);
        size_t foff = off + sec->foff - sec->offset;
        void *ptr = kmap(PAGE_SIZE, NULL, 0, VM_RW | VMA_PHYS);
        pg = mmu_read((size_t)ptr);
        vfs_read(lib->ino, ptr, PAGE_SIZE, foff, 0);
        dlib_relloc_page(lib, ptr, off);
        // kprintf(-1, "New Dlib page, %d of %s\n", idx, lib->name);
        // kdump2(ptr, PAGE_SIZE);
        lib->pages[idx] = pg;
        kunmap(ptr, PAGE_SIZE);
    }
    if (pg != 0)
        lib->page_rcu++;
    mtx_unlock(&lib->mtx);
    return pg;
}

void dlib_release_page(dlib_t *lib, size_t off, size_t pg)
{
    mtx_lock(&lib->mtx);
    lib->page_rcu--;
    // int idx = off / PAGE_SIZE;
    // 
    if (lib->page_rcu == 0) {
        int size = ALIGN_UP(lib->length, PAGE_SIZE) / PAGE_SIZE;
        for (int i = 0; i < size; ++i) {
            size_t page = lib->pages[i];
            if (page != 0)
#ifdef KORA_KRN
                page_release(page);
#else
                page_release_kmap_stub(page);
#endif
        }
        kfree(lib->pages);
        lib->pages = NULL;
    }
    mtx_unlock(&lib->mtx);
}

char *dlib_name(dlib_t *lib, char *buf, int len)
{
    strncpy(buf, lib->name, len);
    return buf;
}
