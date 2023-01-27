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
    uint32_t type:4;
    uint32_t unused: 4;
    uint32_t symbol:24;
});

PACK(struct elf_dynamic
{
    uint32_t unused0;
    uint32_t unused1;
    uint32_t plt_rel_sz;
    uint32_t plt_got;
    uint32_t hash;
    uint32_t str_tab;
    uint32_t sym_tab;
    uint32_t rela;
    uint32_t rela_sz;
    uint32_t rela_ent;
    uint32_t str_sz;
    uint32_t sym_ent;
    uint32_t init;
    uint32_t fini;
    uint32_t soname;
    uint32_t rpath;
    uint32_t symbolic;
    uint32_t rel;
    uint32_t rel_sz;
    uint32_t rel_ent;
    uint32_t plt_rel;
    uint32_t debug;
    uint32_t text_rel;
    uint32_t jmp_rel;
    uint32_t reserved0;
    uint32_t init_array;
    uint32_t fini_array;
    uint32_t init_array_sz;
    uint32_t fini_array_sz;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
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
#define ELF_PH_NOTE 4
#define ELF_PT_SHLIB 5
#define ELF_PH_PHDR 6


#define SHT_NONE     0
#define SHT_PROGBITS 1
#define SHT_SYMTAB   2
#define SHT_STRTAB   3
#define SHT_NOBITS   8
#define SHT_REL      9

#define ELF_DYN_NEEDED  1

int elf_parse(dlib_t *lib, blkmap_t *bkm);

