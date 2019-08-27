#ifndef __KERNEL_RESX__MEMORY_H
#define __KERNEL_RESX__MEMORY_H 1

#include <stddef.h>

int mspace_protect(mspace_t *mspace, size_t address, size_t length, int flags);
int mspace_unmap(mspace_t *mspace, size_t address, size_t length);
mspace_t *mspace_clone(mspace_t *model);
mspace_t *mspace_create();
mspace_t *mspace_open(mspace_t *mspace);
void mspace_close(mspace_t *mspace);
void mspace_display();
void *mspace_map(mspace_t *mspace, size_t address, size_t length, inode_t *ino, off_t offset, int flags);
vma_t *mspace_search_vma(mspace_t *mspace, size_t address);

int page_fault(mspace_t *mspace, size_t address, int reason);
page_t page_get(int zone, int count);
page_t page_new();
void page_range(long long base, long long length);
void page_range(long long base, long long length);
void page_release(page_t paddress);
void page_sweep(mspace_t *mspace, size_t address, size_t length, bool clean);
void page_teardown();

void memory_info();
void memory_initialize();
void memory_sweep();


#endif  /* __KERNEL_RESX__MEMORY_H */
