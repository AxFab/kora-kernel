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
#include "elf32.h"


uint8_t elf_sign[16] = { 0x7F, 'E', 'L', 'F', 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

bool elf_trace = false;

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
    if (elf_trace)
        kprintf(-1, "\nFile format elf32-i386\n");
    return 0;
}

char *rights[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx", };

char *elf_string(elf_string_helper_t *string_table, int idx)
{
    size_t off = string_table->offset + idx;
    void *ptr = blk_map(string_table->bkm, off / PAGE_SIZE, VM_RD);
    int max = PAGE_SIZE - off % PAGE_SIZE;
    char *str = ADDR_OFF(ptr, off % PAGE_SIZE);
    int lg = strnlen(str, max);
    //if (str[0] == 'f' && str[1] == 'l' && str[2] == 'o') {
    char *buf;
    if (lg == max) {
        blkmap_t *bkm2 = blk_clone(string_table->bkm);
        char *str2 = blk_map(bkm2, off / PAGE_SIZE + 1, VM_RD);
        int ex = strnlen(str2, PAGE_SIZE);
        buf = kalloc(lg + ex + 1);
        memcpy(buf, str, lg);
        buf[lg] = '\0';
        strncat(buf, str2, ex + 1);
        blk_close(bkm2);
    } else {
        buf = kalloc(lg + 1);
        memcpy(buf, str, lg);
        buf[lg] = '\0';
    }
    return buf;
}

int elf_section(elf_phead_t *ph, dlsection_t *section)
{
    section->foff = ph->file_addr;
    section->moff = ph->virt_addr;
    section->fsize = ph->file_size;
    section->msize = ph->virt_size;
    section->offset = ph->virt_addr;
    section->length = ALIGN_UP(ph->virt_size, PAGE_SIZE);
    section->rights = ph->flags & 7;
    if ((ph->virt_addr & (PAGE_SIZE - 1)) != 0) {
        size_t off = ph->virt_addr & (PAGE_SIZE - 1);
        section->offset -= off;
        section->length = ALIGN_UP(ph->virt_size + off, PAGE_SIZE);
        return -1;
    }
    if (ph->align < PAGE_SIZE)
        return -1;

    if (elf_trace)
        kprintf(-1, "Section [%06x-%06x] <%04x-%04x> (+%5x)  %s \n", section->offset, section->length, section->foff, section->fsize, section->offset, rights[(int)section->rights]);
    return 0;
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
        dep->name = elf_string(string_table, idx);
        // kprintf(-1, "Rq:  %s \n", dep->name);
        ll_append(&lib->depends, &dep->node);
        off += 4;
    }
}

void elf_symbol(dlsym_t *symbol, elf_sym32_t *sym, dlib_t *lib, elf_dynamic_t *dynamic, elf_string_helper_t *string_table)
{
    symbol->address = sym->value;
    symbol->name = elf_string(string_table, sym->name);
    symbol->size = sym->size;
    symbol->flags = 0;

    if (sym->info == 0x10)
        symbol->type = 0;
    else if (sym->info < 0x10)
        symbol->type = 2;
    else if (sym->info < 0x20)
        symbol->type = 1;
    else 
        symbol->type = 3;

    if (sym->info >= 32 || sym->shndx > 0x7fff)
        symbol->flags |= 0x80; // !!?

    const char *type = "             ";
    if (sym->info == 0x10)
        type = "        *UND*";
    else if (sym->info == 0x11)
        type = "g     O .    ";
    else if (sym->info == 0x12)
        type = "g     F .    ";
    else if (sym->info == 0x20)
        type = " w      *UND*";
    else if (sym->info == 0x00)
        type = "l       .    ";
    else if (sym->info == 0x01)
        type = "l     O .    ";
    else if (sym->info == 0x02)
        type = "l     F .    ";
    else if (sym->info == 0x04)
        type = "l    df *ABS*";
    else 
        type = "             ";
    if (elf_trace)
        kprintf(-1, "%08x %s %08x %s \n", sym->value, type, sym->size, symbol->name);
}


void elf_relocation(dlreloc_t *reloc, elf_reloc32_t *rel, llhead_t *symbols)
{
    int sym_idx = rel->symbol;
    reloc->offset = rel->offset;
    reloc->type = rel->type;// &0xF;
    if (sym_idx != 0 && sym_idx < 0x10000) {
        reloc->symbol = ll_index(symbols, sym_idx - 1, dlsym_t, node);
        if (reloc->symbol == NULL)
            kprintf(-1, "Missing RelSym: %06x  %x  (%d) \n", reloc->offset, reloc->type, sym_idx);
    }
    // kprintf(-1, "R efl: %06x  %x  %s \n", reloc->offset, reloc->type, sym_idx == 0 ? "ABS" : (reloc->symbol == NULL ? "?" : reloc->symbol->name));
}


static void elf_read_cross_page(blkmap_t *bkm, char *buf, size_t len, xoff_t off, int flags)
{
    char *ptr = ADDR_OFF(blk_map(bkm, off / PAGE_SIZE, flags), off % PAGE_SIZE);
    if (off % PAGE_SIZE + len <= PAGE_SIZE) {
        memcpy(buf, ptr, len);
        return;
    }
    size_t disp = (size_t)(PAGE_SIZE - off % PAGE_SIZE);
    memcpy(buf, ptr, disp);
    ptr = blk_map(bkm, off / PAGE_SIZE + 1, flags);
    memcpy(ADDR_OFF(buf, disp), ptr, len - disp);
}


int elf_parse(dlib_t *lib, blkmap_t *bkm)
{
    unsigned i, j;
    int dyn_idx;
    elf_dynamic_t dynamic;
    might_sleep();
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
    if (elf_trace)
        kprintf(-1, "Program Header:\n");

    elf_phead_t *ph_tbl = ADDR_OFF(head, head->ph_off);
    for (i = 0; i < head->ph_count; ++i) {
        const char *sname = "   ....";
        if (ph_tbl[i].type == ELF_PH_LOAD) {
            sname = "   LOAD";
            dlsection_t *section = kalloc(sizeof(dlsection_t));
            ll_append(&lib->sections, &section->node);
            elf_section(&ph_tbl[i], section);
            if (lib->base > section->offset)
                lib->base = section->offset;
            if (lib->length < section->offset + section->length)
                lib->length = section->offset + section->length;
        } else if (ph_tbl[i].type == ELF_PH_DYNAMIC) {
            sname = "DYNAMIC";
            dyn_idx = i;
            elf_dynamic(lib, &ph_tbl[i], blk_clone(bkm), &dynamic);
        } else if (ph_tbl[i].type == ELF_PH_PHDR) {
            sname = "   PHDR";
        } else if (ph_tbl[i].type == ELF_PH_INTERP) {
            sname = " INTERP";
        }

        if (elf_trace) {
            kprintf(-1, " %s off    0x%08x vaddr 0x%08x paddr 0x%08x\n", sname, ph_tbl[i].file_addr, ph_tbl[i].virt_addr, ph_tbl[i].phys_addr);
            kprintf(-1, "         filesz 0x%08x memsz 0x%08x flags %s\n", ph_tbl[i].file_size, ph_tbl[i].virt_size, "---");
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
    if (elf_trace) {
        kprintf(-1, "Dynamic Section:\n");
        kprintf(-1, "  HASH                 0x%08x\n", dynamic.hash);
        kprintf(-1, "  STRTAB               0x%08x\n", dynamic.str_tab);
        kprintf(-1, "  SYMTAB               0x%08x\n", dynamic.sym_tab);
        kprintf(-1, "  STRSZ                0x%08x\n", dynamic.str_sz);
        kprintf(-1, "  SYMENT               0x%08x\n", dynamic.sym_ent);
        kprintf(-1, "  PLTGOT               0x%08x\n", dynamic.plt_got);
        kprintf(-1, "  PLTRELSZ             0x%08x\n", dynamic.plt_rel_sz);
        kprintf(-1, "  PLTREL               0x%08x\n", dynamic.plt_rel);
        kprintf(-1, "  JMPREL               0x%08x\n", dynamic.jmp_rel);
        kprintf(-1, "  REL                  0x%08x\n", dynamic.rel);
        kprintf(-1, "  RELSZ                0x%08x\n", dynamic.rel_sz);
        kprintf(-1, "  RELENT               0x%08x\n", dynamic.rel_ent);
        // kprintf(-1, "  RELCOUNT             0x%08x\n", dynamic.text_rel); TODO -- Fix that !
    }

    dynamic.hash -= lib->base;
    dynamic.init -= lib->base;
    dynamic.fini -= lib->base;
    dynamic.str_tab -= lib->base;
    dynamic.sym_tab -= lib->base;
    dynamic.rel -= lib->base;
    lib->length -= lib->base;
    lib->entry -= lib->base;
    
    uint32_t *hash = ADDR_OFF(head, dynamic.hash);
    // if (dynamic.hash > 0 && dynamic.hash < 4080) {
    // kprintf(-1, "ELF DYN HASH [%08x, %08x, %08x, %08x]\n", hash[0], hash[1], hash[2], hash[3]);
    // }
    if (lib->base != 0) {
        dlsection_t *sec;
        for ll_each(&lib->sections, sec, dlsection_t, node)
        {
            sec->offset -= lib->base;
            sec->moff -= lib->base;
        }
    }


    lib->init = dynamic.init;
    lib->fini = dynamic.fini;

    /* Find string table */
    elf_string_helper_t string_table;
    string_table.bkm = blk_clone(bkm);
    string_table.offset = dynamic.str_tab;

    blkmap_t *bkm_req = blk_clone(bkm);
    elf_requires(lib, &ph_tbl[dyn_idx], bkm_req, &string_table);
    blk_close(bkm_req);

    /* Build symbol table */
    llhead_t symbols = INIT_LLHEAD;
    blkmap_t *bkm_sym = blk_clone(bkm);
    blkmap_t *bkm_sec = blk_clone(bkm);

    /* Browse sections */
    unsigned rel_sz = 0;
    size_t sh_off = head->sh_off + head->sh_str_ndx * sizeof(elf_shead_t);
    elf_shead_t sh_tbl;
    elf_read_cross_page(bkm_sec, (char *)&sh_tbl, sizeof(elf_shead_t), sh_off, VM_RD);// = ADDR_OFF(blk_map(bkm_sec, sh_off / PAGE_SIZE, VM_RD), sh_off % PAGE_SIZE);
    elf_string_helper_t sh_strings;
    sh_strings.bkm = blk_clone(bkm);
    sh_strings.offset = sh_tbl.offset;

    if (elf_trace) {
        kprintf(-1, "Sections:\n");
        kprintf(-1, "Idx Name             Size      VMA/LMA   File off  Align\n");
    }

    for (i = 1; i < head->sh_count; ++i) {
        sh_off = head->sh_off + i * sizeof(elf_shead_t);
        elf_read_cross_page(bkm_sec, (char *)&sh_tbl, sizeof(elf_shead_t), sh_off, VM_RD);
        //sh_tbl = ADDR_OFF(blk_map(bkm_sec, sh_off / PAGE_SIZE, VM_RD), sh_off % PAGE_SIZE);
        char *sh_name = elf_string(&sh_strings, sh_tbl.name_idx);
        if (elf_trace)
            kprintf(-1, " %2d %-16s %08x  %08x  %08x  .\n", i - 1, sh_name, sh_tbl.size, sh_tbl.addr, sh_tbl.offset);
        if (sh_tbl.type == 9)
            rel_sz += sh_tbl.size / dynamic.rel_ent;
        if (sh_tbl.type == 1 && strcmp(sh_name, ".text") == 0)
            lib->text_off = sh_tbl.addr - lib->base;
        if (sh_name != NULL)
            kfree(sh_name);
        // kprintf(-1, "Section %s - %p\n", &sh_strtab[sh_tbl[i].name_idx], sh_tbl[i].addr);
    }


#ifdef WIN32
    //for (i = 1; i < head->sh_count; ++i) {
    //    sh_off = head->sh_off + i * sizeof(elf_shead_t);
    //    elf_read_cross_page(bkm_sec, (char *)&sh_tbl, sizeof(elf_shead_t), sh_off, VM_RD);
    //    kprintf(-1, "Section table %d - %d - %p, %p\n", i, sh_tbl.type, sh_tbl.offset, sh_tbl.size);
    //}
#endif

    unsigned sym_count = hash[1];
    if (elf_trace)
        kprintf(-1, "Symbols table:\n"); // TODO -- Dynamic table !?
    elf_string_helper_t str_strings;
    str_strings.bkm = blk_clone(bkm);
    str_strings.offset = dynamic.str_tab;

    elf_sym32_t sym_tbl;
    // = ADDR_OFF(dlib->iomap, dynamic.sym_tab);
    // kprintf(-1, "ELF DYN HASH [%08x, %08x, %08x, %08x]\n", hash[0], hash[1], hash[2], hash[3]);
    for (i = 1; i < sym_count; ++i) {
        size_t off = dynamic.sym_tab + i * dynamic.sym_ent;
        elf_read_cross_page(bkm_sym, (char *)&sym_tbl, sizeof(elf_sym32_t), off, VM_RD);

        dlsym_t *sym = kalloc(sizeof(dlsym_t));
        ll_append(&symbols, &sym->node);
        elf_symbol(sym, &sym_tbl, lib, &dynamic, &str_strings);
#ifdef WIN32
        //kprintf(-1, "Symbol %p, %s on %s\n", sym->address, sym->name, lib->name);
#endif
    }

    blk_close(str_strings.bkm);


    // blk_close(bkm_sym);
    /* Read relocation table */
    elf_reloc32_t reloc;
    for (i = 1; i < head->sh_count; ++i) {
        sh_off = head->sh_off + i * sizeof(elf_shead_t);
        elf_read_cross_page(bkm_sec, (char *)&sh_tbl, sizeof(elf_shead_t), sh_off, VM_RD);
        if (sh_tbl.type != SHT_REL)
            continue;
        size_t sz = sh_tbl.size / sizeof(elf_reloc32_t);
        for (j = 0; j < sz; ++j) {
            size_t roff = sh_tbl.offset + j * sizeof(elf_reloc32_t);
            elf_read_cross_page(bkm_sym, (char *)&reloc, sizeof(elf_reloc32_t), roff, VM_RD);
            if (reloc.type == 0)
                continue;

            dlreloc_t *rel = kalloc(sizeof(dlreloc_t));
            elf_relocation(rel, &reloc, &symbols);
            if (rel->offset == 0) {
                kfree(rel);
                break;
            }
            rel->offset -= lib->base;
#ifdef WIN32
            //if (rel->symbol)
            //    kprintf(-1, "REL:%d at %p (s:%s:%p)\n", rel->type, rel->offset, rel->symbol->name, rel->symbol->address);
            //else
            //    kprintf(-1, "REL:%d at %p\n", rel->type, rel->offset);
#endif

            ll_append(&lib->relocations, &rel->node);
        }
    }

    /* Organize symbols */
    while (symbols.count_ > 0) {
        dlsym_t *sym = ll_dequeue(&symbols, dlsym_t, node);
        if (/*sym->type != 0 || */sym->address != 0) {
            // kprintf(-1, "Intern symbol %p, %s on %s\n", sym->address, sym->name, lib->name);
            ll_append(&lib->intern_symbols, &sym->node);
        }
        else {
            // kprintf(-1, "Extern symbol %p, %s on %s\n", sym->address, sym->name, lib->name);
            ll_append(&lib->extern_symbols, &sym->node);
        }
    }


    blk_close(string_table.bkm);
    blk_close(sh_strings.bkm);
    blk_close(bkm_sym);
    blk_close(bkm_sec);
    blk_close(bkm);
    might_sleep();
    return 0;
}
