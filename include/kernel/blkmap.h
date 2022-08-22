#ifndef __KERNEL_BLKMAP_H
#define __KERNEL_BLKMAP_H 1

#include <kernel/vfs.h>

typedef struct blkmap blkmap_t;
struct blkmap
{
    inode_t *ino;
    size_t block;
    size_t msize;
    xoff_t off;
    int rights;
    void *ptr;
    void *(*map)(blkmap_t *bkm, size_t no, int rights);
    blkmap_t *(*clone)(blkmap_t *bkm);
    void (*close)(blkmap_t *bkm);
};

blkmap_t *blk_open(inode_t *ino, size_t blocksize);
#ifndef KORA_KRN
blkmap_t *blk_host_open(const char *path, size_t blocksize);
#endif

#define blk_map(b,n,r) (b)->map((b),(n),(r))
#define blk_close(b) (b)->close(b)
#define blk_clone(b) (b)->clone(b)


#endif /* __KERNEL_BLKMAP_H */
