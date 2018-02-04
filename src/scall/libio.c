#include <kernel/core.h>
#include <kernel/scall.h>
#include <kernel/sys/inode.h>
#include <kernel/sys/device.h>
#include <errno.h>


int sys_open(const char *name, int flags, int mode)
{
    return -1;
}
int sys_close(unsigned fd)
{
    return -1;
}
int sys_read(unsigned fd, const struct iovec *vec, unsigned vlen)
{
    return -1;
}
int sys_write(unsigned fd, const struct iovec *vec, unsigned vlen)
{
    return -1;
}
int sys_seek(unsigned fd, off_t offset, int whence)
{
    return -1;
}

int sys_pipe(unsigned *fd, size_t size)
{
    return -1;
}
