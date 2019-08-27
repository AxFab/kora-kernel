#ifndef __KERNEL_RESX_BIO_H
#define __KERNEL_RESX_BIO_H 1

#include <stddef.h>

bio_t *bio_create(inode_t *ino, int flags, int block, size_t offset);
bio_t *bio_create2(inode_t *ino, int flags, int block, size_t offset, int extra);
void bio_clean(bio_t *io, size_t lba);
void bio_destroy(bio_t *io);
void bio_sync(bio_t *io);
void *bio_access(bio_t *io, size_t lba);


#endif  /* __KERNEL_RESX_BIO_H */
