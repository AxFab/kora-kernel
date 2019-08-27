#ifndef __KERNEL_RESX_STREAM_H
#define __KERNEL_RESX_STREAM_H 1

#include <stddef.h>


int resx_rm(resx_t *resx, int fd);
resx_t *resx_create();
resx_t *resx_open(resx_t *resx);
stream_t *resx_get(resx_t *resx, int fd);
stream_t *resx_set(resx_t *resx, inode_t *ino);
void resx_close(resx_t *resx);


#endif  /* __KERNEL_RESX_STREAM_H */
