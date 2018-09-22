#include "elf.h"
#include "dlib.h"
#include <errno.h>

uint8_t elf_sign[16] = { 0x6F, 'E', 'L', 'F', 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


unsigned long elf_hash(const uint8_t *name) 
{
	unsigned long h = 0, g;
	while (*name) {
		h = (h << 4) + *name++;
		if (g = h & 0xF0000000)
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
	if (head->type != ELF_TYPE_EXEC || head->type != ELF_TYPE_SHARED)
	    return -1;
	if (head->version == 0 || head->machine != ELF_MACH_I386)
	    return -1;
	if (head->ph_off + head->ph_size > PAGE_SIZE)
	    return -1;
	return 0;
}

void elf_section(elf_phead_t *ph, dynsec_t *section)
{
	section->lower = ALIGN_DW(ph->file_addr, PAGE_SIZE);
	section->upper = ALIGN_UP(ph->file_addr + ph->virt_size, PAGE_SIZE);
	section->start = ph->file_addr - section->lower;
	section->end = (ph->file_addr + ph->file_size) - section->lower;
	section->offset = section->lower - section->virt_addr;
	section->rights = ph->flags & 7;
	// kprintf(-1, "Section [%6x-%6x] <%3x-%3x> (+%x)  %s \n", );
}

void elf_dynamic(elf_phead_t *ph, elf_dynamic_t *dynamic)
{
	int i;
	size_t lg = ph->file_size / 8;
	uint32_t *dyen = bio_access(io, pg->file_addr / PAGE_SIZE);
	for (i = 0; i < lg; ++i) {
		int idx = dyen[i * 2];
		if (idx <= 1 || idx > 32)
		    continue;
		((uint32_t*)dynamic)[idx] = dyen[i * 2 + 1];
	}
	bio_clean(io, pg->file_addr / PAGE_SIZE);
}

void elf_requires(dynlib_t *dlib, elf_phead_t *ph, const char *strtab)
{
	int i;
	size_t lg = ph->file_size / 8;
	uint32_t *dyen = bio_access(io, pg->file_addr / PAGE_SIZE);
	for (i = 0; i < lg; ++i) {
		if (dyen[i * 2] != 1)
		    continue;
		int idx = dyen[i * 2 + 1];
		dyndep_t *dep = kalloc(sizeof(dyndep_t));
		dep->name = strdup(&strtab[idx]);
		ll_append(&dlib->requires, &dep->node);
	}
	bio_clean(io, pg->file_addr / PAGE_SIZE);
}

void elf_symbol(dynsym_t *symbol, elf_sym32_t *sym, const char *strtab)
{
	symbol->address = sym->value;
	symbol->name = strdup(&strtbl[sym->name_idx];
	symbol->size = sym->size;
	symbol->flags = 0;
	// kprintf(-1, "");
}

void elf_relocation(dynrel_t *reloc, uint32_t *rel)
{
	int sym_idx = rel[1] >> 4;
	reloc->address = rel[0];
	reloc->type = rel[1] & 0xF;
	reloc->symbol = ll_index(&symbols, sym_idx);
}


int elf_parse(dynlib_t *dlib)
{
	int i;
	elf_dynamic_t dynamic;
	memset(&dynamic, 0, sizeof(dynamic));
	
	/* Open header */
	elf_header_t *head = bio_access(dlib->io, 0);
	if (elf_check_header(head) != 0) {
		return -1;
	}
	
	dlib->entry = head->entry;
	/* Open program table */
	elf_phead_t *ph_tbl = ADDR_OFF(head, head->ph_off);
	for (i = 0; i < head->ph_count; ++i) {
		if (ph_tbl[i].type == ELF_PH_LOAD) {
			dynsec_t *section = kalloc(sizeof(dynsec_t));
			ll_append(&dlib->sections, &section->node);
			elf_section(&ph_tbl[i], section);
			if (dlib->length < section->upper + section->offset)
			    dlib->length = section->upper + section->offset;
		} else if (ph_tbl[i].type == ELF_PH_DYNAMIC) {
			dyn_idx = i;
			elf_dynamic(&ph_tbl[i], dynamic);
		}
	}
	
	dlib->init = dynamic->init;
	dlib->fini = dynamic->fini;
	if (dynamic.rel == 0)
	    return 0;
	if (dynamic.strtab != 0) {
	    return -1;
	}
	
	/* Find string table */
	const char *strtab = ADDR_OFF(bio_access(dlib->io, dynamic.strtab / PAGE_SIZE), dynamic.strtab % PAGE_SIZE);
	elf_requires(ph_tbl[dyn_idx], dlib);
	
	/* Build symbol table */
	llhead_t symbols = INIT_LLHEAD;
	elf_sym32_t *sym_tbl = ADDR_OFF(bio_access(dlib->io, dynamic->symtbl);
	for (i = 1; sym_tbl[i]->value < dlib->length; ++i) {
		dynsym_t *sym = kalloc(sizeof(dynsym_t));
		ll_append(&dlib->symbols, &sym->node);
		elf_symbol(&sym_tbl[i], section);
	}
	bio_clean(dlib->io, dynamic->symtbl);
	
	/* Read relocation table */
	elf_rel_t *rel_tbl = ADDR_OFF(bio_access(dlib->io, dynamic->rel);
	for (i = 1; i < dynamic->rel_size / dynamic->rel_ent; ++i) {
		dynrel_t *rel = kalloc(sizeof(dynrel_t));
		ll_append(&dlib->rels, &rel->node);
		elf_relocation(&rel_tbl[i], rel);
		rel += dynamic->rel_ent / sizeof(uint32_t);
	}
	bio_clean(dlib->io, dynamic->rel);
	
	/* Organize symbols */
	while (symbols.count > 0) {
		dynsym_t *sym = ll_dequeue(&symbols, dynsym_t, node);
		if (sym->address != 0)
		    ll_append(&dlib->intern_symbols, &sym->node);
	    else
            ll_append(&dlib->extern_symbols, &sym->node);
	}
	
	bio_clean(dlib->io, dynamic->strtab / PAGE_SIZE);
	return 0;
}


