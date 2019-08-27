#ifndef __KERNEL_RESX_DLIB_H
#define __KERNEL_RESX_DLIB_H 1

#include <stddef.h>


int dlib_map(dynlib_t *dlib, mspace_t *mspace);
int dlib_map_all(proc_t *proc);
int dlib_openexec(proc_t *proc, const char *execname);
bool dlib_resolve_symbols(proc_t *proc, dynlib_t *lib);
proc_t *dlib_process(resx_fs_t *fs, mspace_t *mspace);
void dlib_destroy(dynlib_t *lib);
void dlib_rebase(proc_t *proc, mspace_t *mspace, dynlib_t *lib);
void dlib_unload(proc_t *proc, mspace_t *mspace, dynlib_t *lib);
void *dlib_exec_entry(proc_t *proc);

void kmod_dump();
void kmod_init();
void kmod_mount(inode_t *root);
void kmod_register(kmod_t *mod);
void kmod_symbol(const char *name, void *ptr);
void kmod_symbols(kdk_api_t *kapi);

int elf_open(task_t *task, inode_t *ino);
int elf_parse(dynlib_t *dlib);
int elf_parse(dynlib_t *dlib);

#endif  /* __KERNEL_RESX_DLIB_H */
