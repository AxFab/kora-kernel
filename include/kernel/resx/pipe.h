#ifndef __KERNEL_RESX_PIPE_H
#define __KERNEL_RESX_PIPE_H 1

#include <stddef.h>


inode_t *pipe_inode();
pipe_t *pipe_create();
int pipe_erase(pipe_t *pipe, size_t len);
int pipe_read(pipe_t *pipe, char *buf, size_t len, int flags);
int pipe_reset(pipe_t *pipe);
int pipe_resize(pipe_t *pipe, size_t size);
int pipe_write(pipe_t *pipe, const char *buf, size_t len, int flags);
void pipe_destroy(pipe_t *pipe);

#endif  /* __KERNEL_RESX_PIPE_H */
