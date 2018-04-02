#include <kernel/core.h>
#include <kernel/scall.h>
#include <kernel/sys/inode.h>
#include <kernel/sys/device.h>
#include <kernel/task.h>
#include <errno.h>


int sys_open(int fd, const char *name, int flags, int mode)
{
    task_t *task = kCPU.running;
    inode_t *dir = task->pwd;
    if (fd >= 0)
        dir = resx_get(task->resx, fd);
    inode_t *ino = vfs_search(task->root, dir, name);
    if (ino == NULL)
        return -1;
    errno = 0;
    return resx_set(task->resx, ino);
}

int sys_close(int fd)
{
    task_t *task = kCPU.running;
    return resx_close(task->resx, fd);
}

int sys_read(int fd, const struct iovec *vec, unsigned vlen)
{
    task_t *task = kCPU.running;
    inode_t *ino = resx_get(task->resx, fd);
    if (ino == NULL)
        return -1;
    int bytes = 0, ret;
    for (int i=0; i<vlen; ++i) {
        switch (ino->mode & S_IFMT) {
        case S_IFREG:
        case S_IFBLK:
            ret = blk_read(ino, vec[i].buffer, vec[i].length, 0);
            if (ret < 0)
                return -1;
            bytes += ret;
            break;
        default:
            errno = ENOSYS;
            return -1;
        }
    }
    return bytes;
}

int sys_write(int fd, const struct iovec *vec, unsigned vlen)
{
    return -1;
}

int sys_seek(int fd, off_t offset, int whence)
{
    return -1;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

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

int sys_syslog(const char *msg)
{
    kprintf(0, "%s\n", msg);
    return 1;
}
