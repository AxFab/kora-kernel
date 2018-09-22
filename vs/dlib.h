#ifndef _SRC_DLIB
#define _SRC_DLIB 1

#include <kernel/core.h>
#include <kora/stdef.h>
#include <kora/mcrs.h>

typedef struct dynlib dynlib_t;
typedef struct dynsec dynsec_t;
typedef struct dynsym dynsym_t;
typedef struct dynrel dynrel_t;
typedef struct dyndep dyndep_t;


struct dynsec {
	size_t lower;
	size_t upper;
	size_t start;
	size_t end;
	size_t offset;
	char rights;
	llnode_t node;
};

struct dynsym {
	char *name;
	size_t address;
	size_t size;
	int flags;
	llnode_t node;
};

struct dynrel {
	size_t address;
	int type;
	dynsym_t *symbol;
	llnode_t node;
};

struct dyndep {
	char *name;
	inode_t *ino;
	dynlib_t *lib;
	llnode_t node;
};

struct dynlib {
	char *name;
	int rcu;
	size_t entry;
	size_t init;
	size_t fini;
	size_t base;
	size_t length;
	bio_t *io;
	llnode_t node;
	llhead_t sections;
	llhead_t depends;
	llhead_t intern_symbols;
	llhead_t extern_symbols;
	llhead_t relocations;
};


typedef struct process process_t;
struct process {
	llhead_t stage1;
	llhead_t stage2;
	llhead_t stage3;
	llhead_t libraries;
	inode_t **paths;
	int path_sz;
};


#endif  /* _SRC_DLIB */
