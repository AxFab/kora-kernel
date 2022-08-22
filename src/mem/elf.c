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
#include <kernel/blkmap.h>
#include <errno.h>
#include <string.h>

typedef struct elf_header elf_header_t;
typedef struct elf_phead elf_phead_t;
typedef struct elf_shead elf_shead_t;
typedef struct elf_sym32 elf_sym32_t;
typedef struct elf_reloc32 elf_reloc32_t;

typedef struct elf_dynamic elf_dynamic_t;

PACK(struct elf_header
{
    uint8_t sign[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t ph_off;
    uint32_t sh_off;
    uint32_t flags;
    uint16_t eh_size;
    uint16_t ph_size;
    uint16_t ph_count;
    uint16_t sh_size;
    uint16_t sh_count;
    uint16_t sh_str_ndx;
});

PACK(struct elf_phead
{
    uint32_t type;
    uint32_t file_addr;
    uint32_t virt_addr;
    uint32_t phys_addr;
    uint32_t file_size;
    uint32_t virt_size;
    uint32_t flags;
    uint32_t align;
});

PACK(struct elf_shead
{
    uint32_t name_idx;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t align;
    uint32_t esize;
});

PACK(struct elf_sym32
{
    uint32_t name;
    uint32_t value;
    uint32_t size;
    uint8_t info;
    uint8_t other;
    uint16_t shndx;
});

PACK(struct elf_reloc32
{
    uint32_t offset;
    uint8_t type:4;
    uint8_t unused : 4;
    uint32_t symbol:24;
});

PACK(struct elf_dynamic
{
    size_t unused0;
    size_t unused1;
    size_t plt_rel_sz;
    size_t plt_got;
    size_t hash;
    size_t str_tab;
    size_t sym_tab;
    size_t rela;
    size_t rela_sz;
    size_t rela_ent;
    size_t str_sz;
    size_t sym_ent;
    size_t init;
    size_t fini;
    size_t soname;
    size_t rpath;
    size_t symbolic;
    size_t rel;
    size_t rel_sz;
    size_t rel_ent;
    size_t plt_rel;
    size_t debug;
    size_t text_rel;
    size_t jmp_rel;
    size_t reserved0;
    size_t init_array;
    size_t fini_array;
    size_t init_array_sz;
    size_t fini_array_sz;
    size_t reserved1;
    size_t reserved2;
    size_t reserved3;
});

typedef struct elf_string_helper elf_string_helper_t;
struct elf_string_helper
{
    blkmap_t *bkm;
    size_t offset;
};

#define ELF_TYPE_EXEC  2
#define ELF_TYPE_SHARED  3

#define ELF_MACH_I386  3
#define ELF_MACH_x86_64 62

#define ELF_PH_LOAD  1
#define ELF_PH_DYNAMIC  2
#define ELF_PH_INTERP 3
#define ELF_PH_PHDR 6

#define ELF_DYN_NEEDED  1


uint8_t elf_sign[16] = { 0x7F, 'E', 'L', 'F', 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


unsigned long elf_hash(const uint8_t *name)
{
    unsigned long h = 0, g;
    while (*name) {
        h = (h << 4) + *name++;
        if ((g = h & 0xF0000000))
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

int elf_check_header(elf_header_t *head)
{
    if (head == NULL)
        return -1;
    if (memcmp(head->sign, elf_sign, 16) != 0)
        return -1;
    if (head->type != ELF_TYPE_EXEC && head->type != ELF_TYPE_SHARED)
        return -1;
    if (head->version == 0 || head->machine != ELF_MACH_I386)
        return -1;
    if (head->ph_off + head->ph_size > PAGE_SIZE)
        return -1;
    return 0;
}

char *rights[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx", };

const char *elf_string(elf_string_helper_t *string_table, int idx)
{
    size_t off = string_table->offset + idx;
    void *ptr = blk_map(string_table->bkm, off / PAGE_SIZE, VM_RD);
    int max = PAGE_SIZE - off % PAGE_SIZE;
    char *str = ADDR_OFF(ptr, off % PAGE_SIZE);
    int lg = strnlen(str, max);
    if (lg >= max)
        return NULL;
    return str;
}

void elf_section(elf_phead_t *ph, dlsection_t *section)
{
    int ret = 0;
    section->foff = ph->file_addr;
    section->offset = ph->virt_addr;
    section->csize = ph->file_size;
    section->length = ALIGN_UP(ph->virt_size, PAGE_SIZE);
    section->rights = ph->flags & 7;
    if ((ph->virt_addr & (PAGE_SIZE - 1)) != 0) {
        size_t off = ph->virt_addr & (PAGE_SIZE - 1);
        section->foff -= off;
        section->offset -= off;
        section->csize = off;
        section->length = ALIGN_UP(ph->virt_size + off, PAGE_SIZE);
        ret = -1;
    }
    if (ph->align < PAGE_SIZE)
        ret - 1;
    //section->lower = ALIGN_DW(ph->file_addr, PAGE_SIZE);
    //section->upper = ALIGN_UP(ph->file_addr + ph->virt_size, PAGE_SIZE);
    //section->start = ph->file_addr - section->lower;
    //section->end = (ph->file_addr + ph->file_size) - section->lower;

    // section->offset = ph->virt_addr - ph->file_addr;
    // kprintf(-1, "Section [%06x-%06x] <%04x-%04x> (+%5x)  %s \n", section->lower, section->upper, section->start, section->end, section->offset, rights[(int)section->rights]);
}

void elf_dynamic(dlib_t *lib, elf_phead_t *ph, blkmap_t *bkm, elf_dynamic_t *dynamic)
{
    unsigned i;
    size_t lg = ph->file_size / 8;
    size_t off = ph->file_addr;
    for (i = 0; i < lg; ++i) {
        uint32_t *dyen = ADDR_OFF(blk_map(bkm, off / PAGE_SIZE, VM_RD), off % PAGE_SIZE);
        int idx = *dyen;
        if (idx <= 1 || idx > 32) {
            off += 8;
            continue;
        }
        off += 4;
        dyen = ADDR_OFF(blk_map(bkm, off / PAGE_SIZE, VM_RD), off % PAGE_SIZE);
        int val = *dyen;
        ((uint32_t *)dynamic)[idx] = val;
        off += 4;
    }
    blk_close(bkm);
}

void elf_requires(dlib_t *lib, elf_phead_t *ph, blkmap_t *bkm, elf_string_helper_t *string_table)
{
    unsigned i;
    size_t lg = ph->file_size / 8;
    size_t off = ph->file_addr;
    for (i = 0; i < lg; ++i) {
        uint32_t *dyen = ADDR_OFF(blk_map(bkm, off / PAGE_SIZE, VM_RD), off % PAGE_SIZE);
        if (*dyen != 1) {
            off += 8;
            continue;
        }
        off += 4;
        dyen = ADDR_OFF(blk_map(bkm, off / PAGE_SIZE, VM_RD), off % PAGE_SIZE);
        int idx = *dyen;
        dlname_t *dep = kalloc(sizeof(dlname_t));
        dep->name = strdup(elf_string(string_table, idx));
        // kprintf(-1, "Rq:  %s \n", dep->name);
        ll_append(&lib->depends, &dep->node);
        off += 4;
    }
}

void elf_symbol(dlsym_t *symbol, elf_sym32_t *sym, dlib_t *lib, elf_dynamic_t *dynamic, elf_string_helper_t *string_table)
{
    symbol->address = sym->value;
    symbol->name = strdup(elf_string(string_table, sym->name));
    symbol->size = sym->size;
    symbol->flags = 0;
    if (sym->info >= 32 || sym->shndx > 0x7fff)
        symbol->flags |= 0x80;
    kprintf(-1, "S: %06x  %s [%d-%d-%d-%d]\n", symbol->address, symbol->name, sym->size, sym->info, sym->other, sym->shndx);
}



void elf_relocation(dlreloc_t *reloc, elf_reloc32_t *rel, llhead_t *symbols)
{
    int sym_idx = rel->symbol;
    reloc->offset = rel->offset;
    reloc->type = rel[1].type;// &0xF;
    if (sym_idx != 0) {
        reloc->symbol = ll_index(symbols, sym_idx - 1, dlsym_t, node);
        if (reloc->symbol == NULL)
            kprintf(-1, "Missing RelSym: %06x  %x  (%d) \n", reloc->offset, reloc->type, sym_idx);
    }
    // kprintf(-1, "R efl: %06x  %x  %s \n", reloc->address, reloc->type, sym_idx == 0 ? "ABS" : (reloc->symbol == NULL ? "?" : reloc->symbol->name));
}


int elf_parse(dlib_t *lib, blkmap_t *bkm)
{
    unsigned i;
    int dyn_idx;
    elf_dynamic_t dynamic;
    memset(&dynamic, 0, sizeof(dynamic));

    // Open header
    elf_header_t *head = blk_map(bkm, 0, VM_RD);
    if (elf_check_header(head) != 0) {
        blk_close(bkm);
        return -1;
    }

    lib->entry = head->entry;
    lib->base = 0xffffffff;
    // Open program table
    elf_phead_t *ph_tbl = ADDR_OFF(head, head->ph_off);
    for (i = 0; i < head->ph_count; ++i) {
        if (ph_tbl[i].type == ELF_PH_LOAD) {
            dlsection_t *section = kalloc(sizeof(dlsection_t));
            ll_append(&lib->sections, &section->node);
            elf_section(&ph_tbl[i], section);
            if (lib->base > section->offset)
                lib->base = section->offset;
            if (lib->length < section->offset + section->length)
                lib->length = section->offset + section->length;
        } else if (ph_tbl[i].type == ELF_PH_DYNAMIC) {
            dyn_idx = i;
            elf_dynamic(lib, &ph_tbl[i], blk_clone(bkm), &dynamic);
        }
    }


    if (!dynamic.rel || !dynamic.str_tab || !dynamic.sym_tab) {
        blk_close(bkm);
        return 0;
    } else if (!dynamic.hash || (dynamic.hash - lib->base) > PAGE_SIZE - 16) {
        blk_close(bkm);
        return -1;
    } else if (dynamic.sym_ent != sizeof(elf_sym32_t)
        || dynamic.rel_ent != sizeof(elf_reloc32_t)) {
        blk_close(bkm);
        return -1;
    }


    /* Relocate executable to address zero */
    dynamic.hash -= lib->base;
    dynamic.init -= lib->base;
    dynamic.fini -= lib->base;
    dynamic.str_tab -= lib->base;
    dynamic.sym_tab -= lib->base;
    dynamic.rel -= lib->base;
    lib->length -= lib->base;
    lib->entry -= lib->base;
    
    uint32_t *hash = ADDR_OFF(head, dynamic.hash); // TODO -- Check is on first map!
    // if (dynamic.hash > 0 && dynamic.hash < 4080) {
    // kprintf(-1, "ELF DYN HASH [%08x, %08x, %08x, %08x]\n", hash[0], hash[1], hash[2], hash[3]);
    // }
    dlsection_t *sec;
    for ll_each(&lib->sections, sec, dlsection_t, node)
        sec->offset -= lib->base;


    lib->init = dynamic.init;
    lib->fini = dynamic.fini;

    /* Find string table */
    elf_string_helper_t string_table;
    string_table.bkm = blk_clone(bkm);
    string_table.offset = dynamic.str_tab;

    elf_requires(lib, &ph_tbl[dyn_idx], blk_clone(bkm), &string_table);

    /* Build symbol table */
    llhead_t symbols = INIT_LLHEAD;
    unsigned sym_count = hash[1];
    blkmap_t *bkm_sym = blk_clone(bkm);
    // kprintf(-1, "ELF DYN HASH [%08x, %08x, %08x, %08x]\n", hash[0], hash[1], hash[2], hash[3]);
    for (i = 1; i < sym_count; ++i) {
        size_t off = dynamic.sym_tab + i * dynamic.sym_ent;
        elf_sym32_t *sym_tbl = ADDR_OFF(blk_map(bkm_sym, off / PAGE_SIZE, VM_RD), off % PAGE_SIZE);
        dlsym_t *sym = kalloc(sizeof(dlsym_t));
        ll_append(&symbols, &sym->node);
        elf_symbol(sym, sym_tbl, lib, &dynamic, &string_table);
    }
    // blk_close(bkm_sym);

    /* Browse sections */
    unsigned rel_sz = 0;
    size_t sh_off = head->sh_off + head->sh_str_ndx * sizeof(elf_shead_t);
    elf_shead_t *sh_tbl = ADDR_OFF(blk_map(bkm_sym, sh_off / PAGE_SIZE, VM_RD), sh_off % PAGE_SIZE);
    elf_string_helper_t sh_strings;
    sh_strings.bkm = blk_clone(bkm);
    sh_strings.offset = sh_tbl->offset;

    for (i = 0; i < head->sh_count; ++i) {
        sh_off = head->sh_off + i * sizeof(elf_shead_t);
        sh_tbl = ADDR_OFF(blk_map(bkm_sym, sh_off / PAGE_SIZE, VM_RD), sh_off % PAGE_SIZE);
        char *sh_name = elf_string(&sh_strings, sh_tbl->name_idx);
        if (sh_tbl->type == 9)
            rel_sz += sh_tbl[i].size / dynamic.rel_ent;
        if (sh_tbl->type == 1 && strcmp(sh_name, ".text") == 0)
            lib->text_off = sh_tbl->addr - lib->base;
        // kprintf(-1, "Section %s - %p\n", &sh_strtab[sh_tbl[i].name_idx], sh_tbl[i].addr);
    }

    /* Read relocation table */
    for (i = 0; i < rel_sz; ++i) {
        size_t roff = dynamic.rel + i * dynamic.rel_ent;
        elf_reloc32_t *reloc = ADDR_OFF(blk_map(bkm_sym, roff / PAGE_SIZE, VM_RD), roff % PAGE_SIZE);
        if (reloc->type == 0)
            break;
        dlreloc_t *rel = kalloc(sizeof(dlreloc_t));
        elf_relocation(rel, reloc, &symbols);
        if (rel->offset == 0) {
            kfree(rel);
            break;
        }
        rel->offset -= lib->base;
        ll_append(&lib->relocations, &rel->node);
    }

    /* Organize symbols */
    while (symbols.count_ > 0) {
        dlsym_t *sym = ll_dequeue(&symbols, dlsym_t, node);
        if (sym->address != 0)
            ll_append(&lib->intern_symbols, &sym->node);
        else
            ll_append(&lib->extern_symbols, &sym->node);
    }


    blk_close(string_table.bkm);
    blk_close(sh_strings.bkm);
    blk_close(bkm_sym);
    blk_close(bkm);
    return 0;
}

