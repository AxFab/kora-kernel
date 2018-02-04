#include <kernel/core.h>
#include <kernel/sys/inode.h>
#include <kernel/sys/device.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int dev_null_io(inode_t *ino, const void *data, size_t size, off_t offset)
{
    errno = 0;
    return 0;
}

int dev_zero_read(inode_t *ino, void *data, size_t size, off_t offset)
{
    // TODO Set ino->chardev->in_bytes = MAX_IO_BUF_SZ;
    memset(data, 0, size);
    errno = 0;
    return 0;
}

int dev_ones_read(inode_t *ino, void *data, size_t size, off_t offset)
{
    static uint8_t value = 0xFF; // TODO - configurable through ioctl
    // TODO Set ino->chardev->in_bytes = MAX_IO_BUF_SZ;
    memset(data, value, size);
    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int dev_random_read(inode_t *ino, void *data, size_t size, off_t offset)
{
    // TODO Set ino->chardev->in_bytes = MAX_IO_BUF_SZ;
    while (size > 1) {
        *((uint16_t *)data) = rand() & 0xFFFF;
        data = ((uint8_t *)data) + 2;
        size -= 2;
    }

    if (size) {
        *((uint8_t *)data) = rand() & 0xFF;
    }

    errno = 0;
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void dev_mkchr(int no, const char *name, void *read, void *write)
{
    inode_t *ino = vfs_inode(no, S_IFCHR, 0);
    ino->io_ops = (vfs_io_ops_t *)kalloc(sizeof(vfs_io_ops_t));
    ino->io_ops->read = read;
    ino->io_ops->write = write;
    // ino->io_ops.flags = NO_BUF;
    vfs_mkdev(name, ino, "KoraOS", "Special device", name, NULL);
    vfs_close(ino);
}

int DEVFS_setup()
{
    dev_mkchr(1, "null", dev_zero_read, dev_null_io);
    dev_mkchr(2, "zero", dev_zero_read, NULL);
    dev_mkchr(3, "ones", dev_ones_read, NULL);
    dev_mkchr(4, "random", dev_random_read, NULL);
    // dev_mkchr("urandom", dev_urandom_read, NULL);

    // vfs_register("devfs", &dev_fs_ops);
    errno = 0;
    return 0;
}

int DEVFS_teardown()
{
    // vfs_unregister("devfs");
    // TODO -- remove device!
    errno = 0;
    return 0;
}

