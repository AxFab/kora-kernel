/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#include "elf.h"
#include <kernel/dlib.h>
#include <errno.h>
#include <string.h>

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

void elf_section(elf_phead_t *ph, dynsec_t *section)
{
    section->lower = ALIGN_DW(ph->file_addr, PAGE_SIZE);
    section->upper = ALIGN_UP(ph->file_addr + ph->virt_size, PAGE_SIZE);
    section->start = ph->file_addr - section->lower;
    section->end = (ph->file_addr + ph->file_size) - section->lower;
    section->offset = ph->virt_addr - ph->file_addr;
    section->rights = ph->flags & 7;
    kprintf(-1, "Section [%06x-%06x] <%04x-%04x> (+%5x)  %s \n", section->lower, section->upper, section->start, section->end, section->offset, rights[section->rights]);
}

void elf_dynamic(elf_phead_t *ph, elf_dynamic_t *dynamic, bio_t *io)
{
    unsigned i;
    size_t lg = ph->file_size / 8;
    uint32_t *dyen = ADDR_OFF(bio_access(io, ph->file_addr / PAGE_SIZE), ph->file_addr % PAGE_SIZE);
    for (i = 0; i < lg; ++i) {
        int idx = dyen[i * 2];
        if (idx <= 1 || idx > 32)
            continue;
        ((uint32_t *)dynamic)[idx] = dyen[i * 2 + 1];
    }
    bio_clean(io, ph->file_addr / PAGE_SIZE);
}

void elf_requires(dynlib_t *dlib, elf_phead_t *ph, const char *strtab, bio_t *io)
{
    unsigned i;
    size_t lg = ph->file_size / 8;
    uint32_t *dyen = ADDR_OFF(bio_access(io, ph->file_addr / PAGE_SIZE), ph->file_addr % PAGE_SIZE);
    for (i = 0; i < lg; ++i) {
        if (dyen[i * 2] != 1)
            continue;
        int idx = dyen[i * 2 + 1];
        dyndep_t *dep = kalloc(sizeof(dyndep_t));
        dep->name = strdup(&strtab[idx]);
        kprintf(-1, "Rq:  %s \n", dep->name);
        ll_append(&dlib->depends, &dep->node);
    }
    bio_clean(io, ph->file_addr / PAGE_SIZE);
}

void elf_symbol(dynsym_t *symbol, elf_sym32_t *sym, dynlib_t *lib, elf_dynamic_t *dynamic, const char *strtab)
{
    symbol->address = sym->value;
    const char *name = &strtab[sym->name];
    if (ALIGN_DW((size_t)name, PAGE_SIZE) == ALIGN_DW((size_t)strtab, PAGE_SIZE))
        symbol->name = strdup(name);
    else {
        size_t off = dynamic->str_tab + sym->name;
        name = ADDR_OFF(bio_access(lib->io, off / PAGE_SIZE), off % PAGE_SIZE);
        symbol->name = strdup(name);
        bio_clean(lib->io, off / PAGE_SIZE);
    }
    symbol->size = sym->size;
    symbol->flags = 0;
    // kprintf(-1, "S: %06x  %s \n", symbol->address, symbol->name);
}

void elf_relocation(dynrel_t *reloc, uint32_t *rel, llhead_t *symbols)
{
    int sym_idx = rel[1] >> 8;
    reloc->address = rel[0];
    reloc->type = rel[1] & 0xF;
    if (sym_idx != 0)
        reloc->symbol = ll_index(symbols, sym_idx - 1, dynsym_t, node);
    // kprintf(-1, "R: %06x  %x  %s \n", reloc->address, reloc->type, sym_idx == 0 ? "ABS" : (reloc->symbol == NULL ? "?" : reloc->symbol->name));
}


int elf_parse(dynlib_t *dlib)
{
    unsigned i;
    int dyn_idx;
    elf_dynamic_t dynamic;
    memset(&dynamic, 0, sizeof(dynamic));

    /* Open header */
    elf_header_t *head = bio_access(dlib->io, 0);
    if (elf_check_header(head) != 0) {
        bio_clean(dlib->io, 0);
        return -1;
    }

    dlib->entry = head->entry;
    dlib->base = 0xffffffff;
    /* Open program table */
    elf_phead_t *ph_tbl = ADDR_OFF(head, head->ph_off);
    for (i = 0; i < head->ph_count; ++i) {
        if (ph_tbl[i].type == ELF_PH_LOAD) {
            dynsec_t *section = kalloc(sizeof(dynsec_t));
            ll_append(&dlib->sections, &section->node);
            elf_section(&ph_tbl[i], section);
            if (dlib->base > section->lower + section->offset)
                dlib->base = section->lower + section->offset;
            if (dlib->length < section->upper + section->offset)
                dlib->length = section->upper + section->offset;
        } else if (ph_tbl[i].type == ELF_PH_DYNAMIC) {
            dyn_idx = i;
            elf_dynamic(&ph_tbl[i], &dynamic, dlib->io);
        }
    }

    /* Relocate executable to address zero */
    dynamic.init -= dlib->base;
    dynamic.fini -= dlib->base;
    dynamic.str_tab -= dlib->base;
    dynamic.sym_tab -= dlib->base;
    dynamic.rel -= dlib->base;
    dlib->length -= dlib->base;

    dynsec_t *sec;
    for ll_each(&dlib->sections, sec, dynsec_t, node)
        sec->offset -= dlib->base;


    dlib->init = dynamic.init;
    dlib->fini = dynamic.fini;
    if (dynamic.rel == 0) {
        bio_clean(dlib->io, 0);
        return 0;
    }
    if (dynamic.str_tab == 0/* TODO || dynamic.str_tab > length*/) {
        bio_clean(dlib->io, 0);
        return -1;
    }

    /* Find string table */
    const char *strtab = ADDR_OFF(bio_access(dlib->io, dynamic.str_tab / PAGE_SIZE), dynamic.str_tab % PAGE_SIZE);
    elf_requires(dlib, &ph_tbl[dyn_idx], strtab, dlib->io);

    /* Build symbol table */
    llhead_t symbols = INIT_LLHEAD;
    elf_sym32_t *sym_tbl = ADDR_OFF(bio_access(dlib->io, dynamic.sym_tab / PAGE_SIZE), dynamic.sym_tab % PAGE_SIZE);
    for (i = 1; ; ++i) {
        // TODO -- How to detect the end of table!
        if (sym_tbl[i].value >= dlib->length)
            break;
        if (sym_tbl[i].shndx > head->sh_off)
            break;
        dynsym_t *sym = kalloc(sizeof(dynsym_t));
        ll_append(&symbols, &sym->node);
        elf_symbol(sym, &sym_tbl[i], dlib, &dynamic, strtab);
    }
    bio_clean(dlib->io, dynamic.sym_tab / PAGE_SIZE);

    /* Read relocation table */
    uint32_t *rel_tbl = ADDR_OFF(bio_access(dlib->io, dynamic.rel / PAGE_SIZE), dynamic.rel % PAGE_SIZE);
    int ent_sz = dynamic.rel_ent / sizeof(uint32_t);
    unsigned rel_sz = 0;
    for (i = 0; i < head->sh_count; ++i) {
        size_t sh_offset = head->sh_off + i * sizeof(elf_shead_t);
        elf_shead_t *sh_tbl = ADDR_OFF(bio_access(dlib->io, sh_offset / PAGE_SIZE), sh_offset % PAGE_SIZE);
        if (sh_tbl->type == 9)
            rel_sz += sh_tbl->size / dynamic.rel_ent;
        bio_clean(dlib->io, sh_offset / PAGE_SIZE);
    }

    for (i = 0; i < rel_sz; ++i) {
        int rel_type = rel_tbl[i * ent_sz + 1] & 0xF;
        if (rel_type == 0)
            break;
        dynrel_t *rel = kalloc(sizeof(dynrel_t));
        elf_relocation(rel, &rel_tbl[i * ent_sz], &symbols);
        if (rel->address == 0) {
            kfree(rel);
            break;
        }
        rel->address -= dlib->base;
        ll_append(&dlib->relocations, &rel->node);
        rel += dynamic.rel_ent / sizeof(uint32_t);
    }
    bio_clean(dlib->io, dynamic.rel / PAGE_SIZE);

    /* Organize symbols */
    while (symbols.count_ > 0) {
        dynsym_t *sym = ll_dequeue(&symbols, dynsym_t, node);
        if (sym->address != 0)
            ll_append(&dlib->intern_symbols, &sym->node);
        else
            ll_append(&dlib->extern_symbols, &sym->node);
    }

    bio_clean(dlib->io, dynamic.str_tab / PAGE_SIZE);
    bio_clean(dlib->io, 0);
    return 0;
}


