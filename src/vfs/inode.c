#include <kernel/vfs.h>
#include <errno.h>

void vfs_createfile(inode_t *ino);
void vfs_cleanfile(inode_t *ino);


/* An inode must be created by the driver using a call to `vfs_inode()' */
inode_t *vfs_inode(unsigned no, ftype_t type, device_t *device, const ino_ops_t *ops)
{
    char tmp[16];
    if (device == NULL) {
        device = kalloc(sizeof(device_t));
        // TODO -- Give UniqueID / Register on
        device->no = atomic_xadd(&__vfs_share->dev_no, 1);
        splock_lock(&__vfs_share->lock);
        ll_append(&__vfs_share->dev_list, &device->node);
        splock_unlock(&__vfs_share->lock);
        kprintf(KL_FSA, "Alloc new device `%d`\n", device->no);
        bbtree_init(&device->btree);
        hmp_init(&device->map, 16);
    }

    splock_lock(&device->lock);
    inode_t *inode = bbtree_search_eq(&device->btree, no, inode_t, bnode);
    if (inode != NULL) {
        assert(inode->no == no && inode->type == type);
        inode = vfs_open_inode(inode);
        splock_unlock(&device->lock);
        return inode;
    }

    inode = (inode_t *)kalloc(sizeof(inode_t));
    inode->no = no;
    inode->type = type;
    inode->dev = device;
    inode->ops = ops;
    inode->rcu = 1;
    kprintf(KL_FSA, "Alloc new inode `%s`\n", vfs_inokey(inode, tmp));
    vfs_createfile(inode);
    atomic_inc(&device->rcu);

    inode->bnode.value_ = no;
    bbtree_insert(&device->btree, &inode->bnode);
    splock_unlock(&device->lock);
    return inode;
}
EXPORT_SYMBOL(vfs_inode, 0);

inode_t *vfs_open_inode(inode_t *ino)
{
    char tmp[16];
    assert(ino);
    atomic_inc(&ino->rcu);
    // if (ino->type == FL_BLK || ino->type == FL_CHR || ino->dev->no == 1)
        kprintf(KL_FSA, "Open inode `%s` (%d)\n", vfs_inokey(ino, tmp), ino->rcu);
    return ino;
}
EXPORT_SYMBOL(vfs_open_inode, 0);

void vfs_close_inode(inode_t *ino)
{
    char tmp[16];
    if (ino == NULL)
        return;
    device_t *device = ino->dev;
    splock_lock(&device->lock);
    // if (ino->type == FL_BLK || ino->type == FL_CHR || ino->dev->no == 1)
        kprintf(KL_FSA, "Close inode `%s` (%d)\n", vfs_inokey(ino, tmp), ino->rcu - 1);
    if (atomic_xadd(&ino->rcu, -1) != 1) {
        splock_unlock(&device->lock);
        return;
    }

    kprintf(KL_FSA, "Release inode `%s`\n", vfs_inokey(ino, tmp));
    bbtree_remove(&device->btree, ino->bnode.value_);
    splock_unlock(&device->lock);
    vfs_cleanfile(ino);
    if (ino->ops->close)
        ino->ops->close(ino);
    kfree(ino);

    if (atomic_xadd(&device->rcu, -1) != 1)
        return;

    assert(device->map.count == 0);
    hmp_destroy(&device->map);
    if (device->devclass)
        kfree(device->devclass);
    if (device->devname)
        kfree(device->devname);
    if (device->model)
        kfree(device->model);
    if (device->vendor)
        kfree(device->vendor);
    splock_lock(&__vfs_share->lock);
    ll_remove(&__vfs_share->dev_list, &device->node);
    splock_unlock(&__vfs_share->lock);
    kprintf(KL_FSA, "Release device `%d`\n", device->no);
    kfree(device);
}
EXPORT_SYMBOL(vfs_close_inode, 0);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int vfs_chmod(fnode_t *node, user_t *user, int mode)
{
    inode_t *dir = vfs_parentof(node);
    if (dir->ops->chmod == NULL) {
        vfs_close_inode(dir);
        errno = EPERM;
        return -1;
    } else if (vfs_access(dir, user, VM_WR) != 0) {
        vfs_close_inode(dir);
        errno = EACCES;
        return -1;
    }

    inode_t *ino = vfs_inodeof(node);
    int ret = dir->ops->chmod(ino, mode);
    vfs_close_inode(dir);
    vfs_close_inode(ino);
    return ret;
}
EXPORT_SYMBOL(vfs_chmod, 0);

int vfs_chown(fnode_t *node, user_t *user, user_t *nacl)
{
    inode_t *dir = vfs_parentof(node);
    if (dir->ops->chown == NULL) {
        vfs_close_inode(dir);
        errno = EPERM;
        return -1;
    } else if (vfs_access(dir, user, VM_WR) != 0) {
        vfs_close_inode(dir);
        errno = EACCES;
        return -1;
    }
    inode_t *ino = vfs_inodeof(node);
    int ret = dir->ops->chown(ino, nacl);
    vfs_close_inode(dir);
    vfs_close_inode(ino);
    return ret;
}
EXPORT_SYMBOL(vfs_chown, 0);

int vfs_utimes(fnode_t *node, user_t *user, xtime_t time, int flags)
{
    inode_t *dir = vfs_parentof(node);
    if (dir->ops->utimes == NULL) {
        vfs_close_inode(dir);
        errno = EPERM;
        return -1;
    } else if (vfs_access(dir, user, VM_WR) != 0) {
        vfs_close_inode(dir);
        errno = EACCES;
        return -1;
    }
    inode_t *ino = vfs_inodeof(node);
    int ret = dir->ops->utimes(ino, time, flags);
    vfs_close_inode(dir);
    vfs_close_inode(ino);
    return ret;
}
EXPORT_SYMBOL(vfs_utimes, 0);

//int vfs_truncate(inode_t *ino, acl_t *acl, xoff_t length)
//{
//
//}
//EXPORT_SYMBOL(vfs_truncate, 0);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// int vfs_readdir(inode_t *ino, void *dir, xstat_t *stat, char *buf, int len)
//{
//}
//EXPORT_SYMBOL(vfs_readdir, 0);

int vfs_readlink(inode_t *ino, char *buf, int len)
{
    assert(len > 0);
    if (ino->ops->readlink == NULL) {
        errno = EPERM;
        return -1;
    }

    return ino->ops->readlink(ino, buf, len);
}
EXPORT_SYMBOL(vfs_readlink, 0);

// int vfs_stat(inode_t *ino, xstat_t *stat)
//{
//}
//EXPORT_SYMBOL(vfs_stat, 0);

int vfs_access(inode_t *ino, user_t *user, int flags)
{
    int us = 2;
    int md = ino->mode >> (us * 3);
    if ((md & flags) == flags)
        return 0;
    errno = EACCES;
    return -1;
}
EXPORT_SYMBOL(vfs_access, 0);


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

//long vfs_read(inode_t *ino, char *buf, size_t len, xoff_t off, int flags)
//{
//
//}
//EXPORT_SYMBOL(vfs_read, 0);
//
//long vfs_write(inode_t *ino, const char *buf, size_t len, xoff_t off, int flags)
//{
//
//}
//EXPORT_SYMBOL(vfs_write, 0);


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

size_t vfs_fetch_page(inode_t *ino, xoff_t off)
{
    if (ino->ops->fetch == NULL) {
        errno = EPERM;
        return 0;
    }
    return ino->ops->fetch(ino, off);
}
EXPORT_SYMBOL(vfs_fetch_page, 0);

int vfs_release_page(inode_t *ino, xoff_t off, size_t page, bool dirty)
{
    if (ino->ops->release == NULL) {
        errno = EPERM;
        return -1;
    }
    ino->ops->release(ino, off, page, dirty);
    return 0;
}
EXPORT_SYMBOL(vfs_release_page, 0);


void vfs_usage(inode_t *ino, int access, int count)
{
    if (ino->fops && ino->fops->usage != NULL)
        ino->fops->usage(ino, access, count);
}
