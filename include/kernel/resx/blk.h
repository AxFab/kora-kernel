#ifndef __KERNEL_RESX_BLK_H
#define __KERNEL_RESX_BLK_H 1

#include <stddef.h>


blk_cache_t *blk_create(inode_t *ino, void *read, void *write);
void blk_destroy(blk_cache_t *cache);
void blk_markwr(blk_cache_t *cache, off_t off);
void blk_release(blk_cache_t *cache, off_t off, page_t pg);
void blk_sync(blk_cache_t *cache, off_t off, page_t pg);
page_t blk_fetch(blk_cache_t *cache, off_t off);


#endif  /* __KERNEL_RESX_BLK_H */
