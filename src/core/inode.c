#include <kernel/core.h>
#include <kernel/sys/inode.h>
#include <kora/mcrs.h>
#include <assert.h>
#include <errno.h>

/* An inode must be created by the driver using a call to `vfs_inode()'
 */

inode_t *vfs_inode(int no, int mode, size_t size)
{
    inode_t *inode = (inode_t *)kalloc(MAX(sizeof(inode_t), size));
    inode->no = no;
    inode->mode = mode & S_IFMT;
    switch (mode & S_IFMT) {
    case S_IFREG:
        inode->mode |= 0644;
    case S_IFBLK:
        inode->mode |= 0640;
        inode->pcache = NULL; // Page cache
        break;
    case S_IFCHR:
        inode->mode |= 0640;
        inode->chard = NULL; // Info about i/o
        break;
    case S_IFDIR:
    case S_IFLNK:
        inode->mode |= 0755;
        /* Directory is linked later with mounted point to provide path caching */
        /* Path contains in symbolic link, we'll be read by vfs_read_slink() */
        inode->object = NULL;
        break;
    case S_IFIFO:
        inode->mode |= 0600;
        inode->fifo = NULL; // Structure Fifo
        break;
    case S_IFSOCK:
        inode->mode |= 0600;
        inode->socket = NULL; // Socket structure
        break;
    default:
        kfree(inode);
        return NULL;
    }

    kclock(&inode->btime);
    inode->ctime = inode->btime;
    inode->mtime = inode->btime;
    inode->atime = inode->btime;

    inode->counter = 1;
    inode->links = 0;
    // inode->uid = uid;
    // inode->gid = gid;
    return inode;
}

inode_t *vfs_open(inode_t *ino)
{
    atomic_inc(&ino->counter);
    return ino;
}

void vfs_close(inode_t *ino)
{
    unsigned int cnt = atomic_xadd(&ino->counter, -1);
    if (cnt == 0) {
        // if (ino->fs_ops && ino->fs_ops->close) {
        //   ino->fs_ops->close(ino);
        // }
        kfree(ino);
    }

    if (cnt == ino->links) {
        // Push on LRU !?
        // Someone can access it right now...
    }
}

// void vfs_link(dirent_t *den, inode_t* ino)
// {
//   den->ino = ino;
//   atomic_inc(&ino->counter);
//   atomic_inc(&ino->links);
//   // ll_append(ino->dentries, &den->node);
// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int vfs_read(inode_t *ino, void *buffer, size_t size, off_t offset)
{
    if (!ino->io_ops || !ino->io_ops->read) {
        errno = ENOSYS;
        return -1;
    }

    kprintf(-1, "[VFS ] Read inode(%x, %x,%x)\n", ino, offset, size);
    return ino->io_ops->read(ino, buffer, size, offset);
}

int vfs_write(inode_t *ino, const void *buffer, size_t size, off_t offset)
{
    if (!ino->io_ops || !ino->io_ops->write) {
        errno = ENOSYS;
        return -1;
    }

    kprintf(-1, "[VFS ] Write inode(%x, %x,%x)\n", ino, offset, size);
    return ino->io_ops->write(ino, buffer, size, offset);
}

page_t vfs_page(inode_t *ino, off_t offset)
{
    assert(S_ISREG(ino->mode) || S_ISBLK(ino->mode));
    // block_t *block = ino->pcache;

    return 0;
}

