#include <kernel/core.h>
#include <kernel/scall.h>
#include <kernel/sys/inode.h>
#include <kernel/sys/device.h>
#include <errno.h>


int sys_open(const char *name, int flags, int mode)
{
    return -1;
}
int sys_close(int fd)
{
    return -1;
}
int sys_read(int fd, const struct iovec *vec, unsigned vlen)
{
    return -1;
}
int sys_write(int fd, const struct iovec *vec, unsigned vlen)
{
    return -1;
}
int sys_seek(int fd, off_t offset, int whence)
{
    return -1;
}

int sys_pipe(int *fd, size_t size)
{
    return -1;
}








int sys_window(struct image *img, int features, int events)
{
    // int width = ALIGN_UP(img->width, 16);
    // int height = ALIGN_UP(img->height, 16);
    // size_t size = (size_t)width * height * 4;
    // inode_t * ino = vfs_inode(3, S_FWIN, 0);
    // ino->block = (size_t)width * 4;
    // ino->length = (size_t)width * height * 4;
    // // No operations !
    // task_setfile(kCPU.running, ino);
    // // ALLOC ALL PAGES !
    // // Create a new inode of type frame buffer !
    return 0;
}

