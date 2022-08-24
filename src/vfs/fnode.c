#include <kernel/vfs.h>
#include <errno.h>
#include <fcntl.h>

fnode_t *vfs_fsnode_from(fnode_t *parent, const char *name)
{
    char tmp[16];
    assert(parent && parent->mode == FN_OK && parent->ino);
    device_t *device = parent->ino->dev;
    mtx_lock(&parent->mtx);
    splock_lock(&device->lock);
    int len = 4 + strnlen(name, 256);
    if (len >= 260) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    char *key = kalloc(len + 1); // TODO -- Annoying alloc
    *((uint32_t *)key) = parent->ino->no;
    strcpy(&key[4], name);
    fnode_t *node = hmp_get(&device->map, key, len);
    if (node == NULL) {
        node = kalloc(sizeof(fnode_t));
        kprintf(KL_FSA, "Alloc new fsnode `%s/%s`\n", vfs_inokey(parent->ino, tmp), name);
        mtx_init(&node->mtx, mtx_plain);
        node->parent = vfs_open_fnode(parent);
        ll_append(&parent->clist, &node->cnode);
        hmp_put(&device->map, key, len, node);
        strcpy(node->name, name);
    } else if (ll_contains(&device->llru, &node->nlru)) {
        ll_remove(&device->llru, &node->nlru);
        kprintf(KL_FSA, "Remove fsnode from LRU `%s/%s`\n", vfs_inokey(parent->ino, tmp), name);
    }

    splock_unlock(&device->lock);
    mtx_unlock(&parent->mtx);
    kfree(key);
    atomic_inc(&node->rcu);
    // kprintf(KL_FSA, "Open fsnode `%s/%s` (%d)\n", vfs_inokey(parent->ino, tmp), name, node->rcu);
    return node;
}

// /!\ We must have the mutex on parent, and lock on device
static void vfs_destroy_fsnode(device_t *dev, fnode_t *node, char *key)
{
    char tmp[16];
    
    if (node->parent == NULL) {
        kprintf(KL_FSA, "Release zombie fsnode `%s`\n", node->name);
        splock_unlock(&dev->lock);
        mtx_destroy(&node->mtx);
        vfs_close_inode(node->ino);
        kfree(node);
        return;
    }

    int len = 4 + strlen(node->name);
    *((uint32_t *)key) = node->parent->ino->no;
    strcpy(&key[4], node->name);
    hmp_remove(&dev->map, key, len);
    splock_unlock(&dev->lock);

    mtx_lock(&node->parent->mtx);
    if (node->is_mount) {
        ll_remove(&node->parent->mnt, &node->nmt);
    }

    ll_remove(&node->parent->clist, &node->cnode);
    mtx_unlock(&node->parent->mtx);
    kprintf(KL_FSA, "Release fsnode `%s/%s`\n", vfs_inokey(node->parent->ino, tmp), node->name);
    mtx_destroy(&node->mtx);
    vfs_close_fnode(node->parent);
    vfs_close_inode(node->ino);
    kfree(node);
}

void vfs_dev_scavenge(device_t *dev, int max)
{
    char *key = kalloc(256 + 4); // TODO Annoying alloc
    if (max < 0)
        max = dev->llru.count_;
    while (max-- > 0) {
        splock_lock(&dev->lock);

        fnode_t *node = ll_dequeue(&dev->llru, fnode_t, nlru);
        if (node == NULL) {
            splock_unlock(&dev->lock);
            break;
        }

        vfs_destroy_fsnode(dev, node, key);
    }
    kfree(key);
}

fnode_t *vfs_open_fnode(fnode_t *node)
{
    assert(node);
    atomic_inc(&node->rcu);
    // kprintf(KL_FSA, "Open fsnode `%s/%s` (%d)\n", node->parent ? vfs_inokey(node->parent->ino, tmp) : "", node->name, node->rcu);
    return node;
}

void vfs_close_fnode(fnode_t *node)
{
    char tmp[16];
    assert(node);
    // kprintf(KL_FSA, "Close fsnode `%s/%s` (%d)\n", node->parent ? vfs_inokey(node->parent->ino, tmp) : "", node->name, node->rcu);
    if (atomic_xadd(&node->rcu, -1) == 1) {
        device_t *device = node->parent ? node->parent->ino->dev : node->ino->dev;
        splock_lock(&device->lock);
        if (node->mode == FN_EMPTY || node->is_mount) {
            char *key = kalloc(256 + 4); // TODO Annoying alloc
            vfs_destroy_fsnode(device, node, key);
            kfree(key);
        } else if (node->rcu == 0) {
            kprintf(KL_FSA, "Add fsnode to LRU `%s/%s`\n", node->parent ? vfs_inokey(node->parent ->ino, tmp) : "", node->name);
            ll_enqueue(&device->llru, &node->nlru);
            splock_unlock(&device->lock);
        }
    }
}

int vfs_clear_fsnode(fnode_t *node, int mode)
{
    char tmp[16];
    assert(mode != FN_OK);
    char *key = kalloc(256 + 4); // TODO Annoying alloc
    int ret = 0;
    inode_t *pino = vfs_parentof(node);
    device_t *dev = pino->dev;
    vfs_close_inode(pino);

    // mtx_lock(&node->mtx); TODO -- Keep lock, destroy child later (move list)...
    fnode_t *child = ll_first(&node->clist, fnode_t, cnode);
    while (child) {
        mtx_unlock(&node->mtx);
        if (child->rcu != 0 || (child->mode != FN_EMPTY && child->mode != FN_NOENTRY)) {
            vfs_fnode_zombify(child);
            //kprintf(-1, "Error, unable to clean child fsnode\n");
            //ret = -1;
        } else {
            splock_lock(&dev->lock);
            ll_remove(&dev->llru, &child->nlru);
            vfs_destroy_fsnode(dev, child, key);
            // splock_unlock(&dev->lock);
        }
        mtx_lock(&node->mtx);
        child = ll_first(&node->clist, fnode_t, cnode);
    }
    
    kprintf(KL_FSA, "Unlink fsnode `%s/%s`\n", vfs_inokey(node->parent->ino, tmp), node->name);
    vfs_close_inode(node->ino);
    node->ino = NULL;
    node->mode = mode;
    // mtx_unlock(&node->mtx);
    kfree(key);
    return ret;
}

void vfs_resolve(fnode_t *node, inode_t *ino)
{
    // TODO -- Assert node->mtx is locked
    char tmp1[16];
    char tmp2[16];
    assert(node && ino && node->mode != FN_OK);
    assert(ino->rcu > 0);
    node->ino = vfs_open_inode(ino);
    node->mode = FN_OK;
    kprintf(KL_FSA, "Resolve fnode `%s/%s` to `%s`\n", vfs_inokey(node->parent->ino, tmp1), node->name, vfs_inokey(node->ino, tmp2));
}

int vfs_fnode_bellow(fnode_t *root, fnode_t *dir)
{
    while (dir && dir != root)
        dir = dir->parent;
    return dir == NULL ? -1 : 0;
}

void vfs_fnode_zombify(fnode_t *node)
{
    char tmp[16];
    fnode_t *parent = node->parent;
    assert(!node->is_mount);

    mtx_lock(&parent->mtx);

    device_t *dev = parent->ino->dev;
    splock_lock(&dev->lock);
    char *key = kalloc(256 + 4);
    int len = 4 + strlen(node->name);
    *((uint32_t *)key) = node->parent->ino->no;
    strcpy(&key[4], node->name);
    hmp_remove(&dev->map, key, len);
    splock_unlock(&dev->lock);


    ll_remove(&node->parent->clist, &node->cnode);
    mtx_unlock(&node->parent->mtx);

    inode_t *ino = vfs_parentof(node);
    kprintf(KL_FSA, "Zombify fsnode `%s/%s`\n", vfs_inokey(ino, tmp), node->name);
    vfs_close_fnode(parent);
    vfs_close_inode(ino);
    node->parent = NULL;
    // node->zombie = true; (parent = NULL);
}
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *vfs_parentof(fnode_t *node)
{
    assert(node && node->parent && node->parent->mode == FN_OK && node->parent->ino);
    return vfs_open_inode(node->parent->ino); // TODO -- Should we use RCU !?
}

inode_t *vfs_inodeof(fnode_t *node)
{
    assert(node && node->mode == FN_OK && node->ino);
    return vfs_open_inode(node->ino); // TODO -- Should we use RCU !?
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int vfs_open_access(int options)
{
    int access = 0;
    if (options & O_RDONLY)
        access = VM_RD;
    if (options & O_WRONLY)
        access = access == 0 ? VM_WR : -1;
    if (options & O_RDWR)
        access = access == 0 ? VM_RD | VM_WR : -1;
    if (access < 0)
        errno = EINVAL;
    else if (access == 0)
        access = VM_RD;
    return access;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *vfs_mkdir(vfs_t *vfs, const char *name, user_t *user, int mode)
{
    errno = 0;
    // TODO -- Check last entry can't be . or .. -> EINVAL
    fnode_t *node = vfs_search(vfs, name, user, false, false);
    if (node == NULL) {
        assert(errno != 0);
        return NULL;
    }

    mtx_lock(&node->mtx);
    if (node->mode != FN_EMPTY && node->mode != FN_NOENTRY) {
        mtx_unlock(&node->mtx);
        errno = EEXIST;
        return NULL;
    }

    inode_t *dir = vfs_parentof(node);
    assert(dir != NULL);
    if (dir->ops->mkdir == NULL) {
        mtx_unlock(&node->mtx);
        errno = EPERM;
        return NULL;
    }

    assert(irq_ready());
    inode_t *ino = dir->ops->mkdir(dir, node->name, mode & (~vfs->umask & 07777), user);
    if (ino != NULL)
        vfs_resolve(node, ino);
    mtx_unlock(&node->mtx);
    vfs_close_inode(dir);
    vfs_close_fnode(node);
    assert((ino != NULL) != (errno != 0));
    return ino;
}
EXPORT_SYMBOL(vfs_mkdir, 0);

int vfs_rmdir(vfs_t *vfs, const char *name, user_t *user)
{
    errno = 0;
    fnode_t *node = vfs_search(vfs, name, user, true, false); // TODO !!?
    if (node == NULL) {
        assert(errno != 0);
        return -1;
    }

    mtx_lock(&node->mtx);
    if (node->mode == FN_NOENTRY) {
        mtx_unlock(&node->mtx);
        errno = ENOENT;
        return -1;
    } 
    
    if (node->parent && node->parent->mode == FN_OK && node->ino->dev != node->parent->ino->dev) {
        mtx_unlock(&node->mtx);
        errno = EBUSY;
        return -1;
    }

    inode_t *dir = vfs_parentof(node);
    assert(dir != NULL);
    if (dir->ops->rmdir == NULL) {
        mtx_unlock(&node->mtx);
        errno = ENOSYS;
        return -1;
    }
    int ret = dir->ops->rmdir(dir, node->name);
    vfs_close_inode(dir);
    if (ret == 0)
        vfs_clear_fsnode(node, FN_NOENTRY);
    mtx_unlock(&node->mtx);
    vfs_close_fnode(node);
    assert((ret == 0) != (errno != 0));
    return ret;
}
EXPORT_SYMBOL(vfs_rmdir, 0);

int vfs_create(fnode_t *node, void *user, int mode)
{
    assert(node != NULL);
    mtx_lock(&node->mtx);
    if (node->mode != FN_EMPTY && node->mode != FN_NOENTRY) {
        mtx_unlock(&node->mtx);
        errno = EEXIST;
        return -1;
    }

    assert(node->parent);
    inode_t *dir = vfs_parentof(node);
    if (dir->dev->flags & FD_RDONLY) {
        mtx_unlock(&node->mtx);
        vfs_close_inode(dir);
        errno = EROFS;
        return -1;
    }
    if (dir->ops->create == NULL) {
        mtx_unlock(&node->mtx);
        vfs_close_inode(dir);
        errno = ENOSYS;
        return -1;
    }

    inode_t *ino = dir->ops->create(dir, node->name, user, mode);
    mtx_unlock(&node->mtx);
    vfs_close_inode(dir);
    if (ino != NULL)
        vfs_resolve(node, ino);
    vfs_close_inode(ino);
    return ino ? 0 : -1;
}
EXPORT_SYMBOL(vfs_create, 0);

inode_t *vfs_open(vfs_t *vfs, const char *name, user_t *user, int mode, int flags) 
{
    int access = vfs_open_access(flags);
    if (access <= 0)
        return NULL;

    errno = 0;
    fnode_t *node = vfs_search(vfs, name, user, false, false);
    if (node == NULL)
        return NULL;
    
    bool created = false;
    if (vfs_lookup(node) != 0) {
        if (flags & O_CREAT) {
            created = true;
            int ret = vfs_create(node, NULL, mode & (~vfs->umask & 0777));
            if (ret != 0) {
                if (unlikely(node->mode == FN_OK)) {
                    if (flags & O_EXCL) {
                        vfs_close_fnode(node);
                        return NULL;
                    }
                } else {
                    vfs_close_fnode(node);
                    assert(errno);
                    return NULL;
                }
            }
        } else {
            vfs_close_fnode(node);
            errno = ENOENT;
            return NULL;
        }
    } else if (flags & O_EXCL) {
        vfs_close_fnode(node);
        errno = EEXIST;
        return NULL;
    }

    inode_t *ino = vfs_inodeof(node);
    vfs_close_fnode(node);
    if (!created && vfs_access(ino, user, access) != 0) {
        vfs_close_inode(ino);
        errno = EACCES;
        return NULL;
    }

    if (ino->type == FL_DIR && (access & VM_WR || flags & O_TRUNC)) {
        vfs_close_inode(ino);
        errno = EISDIR;
        return NULL;
    }

    if (flags & O_TRUNC) {
        if (!created && vfs_access(ino, user, VM_RW) != 0) {
            vfs_close_inode(ino);
            errno = EPERM;
            return NULL;
        }
        if (vfs_truncate(ino, 0) != 0) {
            vfs_close_inode(ino);
            assert(errno);
            return NULL;
        }
    }

    return ino;
}
EXPORT_SYMBOL(vfs_open, 0);

int vfs_unlink(vfs_t *vfs, const char *name, user_t *user)
{
    errno = 0;
    fnode_t *node = vfs_search(vfs, name, user, true, false);
    if (node == NULL) {
        assert(errno != 0);
        return -1;
    }

    mtx_lock(&node->mtx);
    if (node->mode == FN_NOENTRY) {
        mtx_unlock(&node->mtx);
        errno = ENOENT;
        return -1;
    }

    inode_t *dir = vfs_parentof(node);
    assert(dir != NULL);
    if (dir->ops->unlink == NULL) {
        mtx_unlock(&node->mtx);
        vfs_close_inode(dir);
        errno = ENOSYS;
        return -1;
    }
    int ret = dir->ops->unlink(dir, node->name);
    vfs_close_inode(dir);
    if (ret == 0)
        vfs_clear_fsnode(node, FN_NOENTRY);
    mtx_unlock(&node->mtx);
    vfs_close_fnode(node);
    assert((ret == 0) != (errno != 0));
    return ret;
}
EXPORT_SYMBOL(vfs_unlink, 0);

int vfs_link(vfs_t *vfs, const char *name, user_t *user, fnode_t *target)
{
    errno = 0;
    // TODO -- Check last entry can't be . or .. -> EINVAL
    fnode_t *node = vfs_search(vfs, name, user, false, false);
    if (node == NULL) {
        assert(errno != 0);
        return -1;
    }

    inode_t *ino = vfs_inodeof(target);
    if (ino->type == FL_DIR) {
        vfs_close_inode(ino);
        vfs_close_fnode(node);
        errno = EPERM;
        return -1;
    }

    mtx_lock(&node->mtx);
    if (node->mode != FN_EMPTY && node->mode != FN_NOENTRY) {
        mtx_unlock(&node->mtx);
        vfs_close_inode(ino);
        vfs_close_fnode(node);
        errno = EEXIST;
        return -1;
    }

    inode_t *dir = vfs_parentof(node);
    assert(dir != NULL);
    if (dir->ops->link == NULL) {
        mtx_unlock(&node->mtx);
        vfs_close_inode(ino);
        vfs_close_inode(dir);
        vfs_close_fnode(node);
        errno = EPERM;
        return -1;
    }
    if (dir->dev != ino->dev) {
        mtx_unlock(&node->mtx);
        vfs_close_inode(ino);
        vfs_close_inode(dir);
        vfs_close_fnode(node);
        errno = EXDEV;
        return -1;
    }

    assert(irq_ready());
    int ret = dir->ops->link(dir, node->name, ino);
    if (ret == 0)
        vfs_resolve(node, ino);
    mtx_unlock(&node->mtx);
    vfs_close_inode(ino);
    vfs_close_inode(dir);
    vfs_close_fnode(node);
    return ret;
}
EXPORT_SYMBOL(vfs_link, 0);

inode_t *vfs_symlink(vfs_t *vfs, const char *name, user_t *user, const char *path) 
{
    errno = 0;
    fnode_t *node = vfs_search(vfs, name, user, false, false);
    if (node == NULL) {
        assert(errno != 0);
        return NULL;
    }

    mtx_lock(&node->mtx);
    if (node->mode != FN_EMPTY && node->mode != FN_NOENTRY) {
        mtx_unlock(&node->mtx);
        vfs_close_fnode(node);
        errno = EEXIST;
        return NULL;
    }

    assert(node->parent && node->parent->ino);
    inode_t *dir = vfs_parentof(node);
    if (dir->dev->flags & FD_RDONLY) {
        mtx_unlock(&node->mtx);
        vfs_close_fnode(node);
        vfs_close_inode(dir);
        errno = EROFS;
        return NULL;
    }
    if (dir->ops->symlink == NULL) {
        mtx_unlock(&node->mtx);
        vfs_close_fnode(node);
        vfs_close_inode(dir);
        errno = ENOSYS;
        return NULL;
    }

    inode_t *ino = dir->ops->symlink(dir, node->name, user, path);
    vfs_close_inode(dir);
    if (ino != NULL)
        vfs_resolve(node, ino);
    mtx_unlock(&node->mtx);
    vfs_close_fnode(node);
    return ino;
}
EXPORT_SYMBOL(vfs_symlink, 0);

//int vfs_readlink(vfs_t *vfs, fnode_t *node, char *buf, size_t len) {}
//EXPORT_SYMBOL(vfs_readlink, 0);

int vfs_rename(fnode_t* src, fnode_t* dst, user_t* user)
{
    errno = 0;
    inode_t* dir_s = vfs_parentof(src);
    if (dir_s == NULL) {
        errno = ENOENT;
        return -1;
    }
    inode_t* dir_d = vfs_parentof(dst);
    if (dir_d == NULL) {
        vfs_close_inode(dir_s);
        errno = ENOENT;
        return -1;
    }

    if (dir_s->dev != dir_d->dev) {
        vfs_close_inode(dir_s);
        vfs_close_inode(dir_d);
        errno = EXDEV;
        return -1;
    }

    if (dir_d->ops->rename == NULL) {
        vfs_close_inode(dir_s);
        vfs_close_inode(dir_d);
        errno = EXDEV;
        return -1;
    }

    inode_t* ino = vfs_inodeof(src);
    mtx_lock(&dir_s->dev->dual_lock);
    mtx_lock(&dst->mtx);
    mtx_lock(&src->mtx);
    int ret = dir_d->ops->rename(dir_s, src->name, dir_d, dst->name);
    if (ret == 0) {
        vfs_resolve(dst, ino);
        vfs_clear_fsnode(src, FN_NOENTRY);
    }
    mtx_unlock(&dst->mtx);
    mtx_unlock(&src->mtx);
    mtx_unlock(&dir_s->dev->dual_lock);
    vfs_close_inode(ino);
    vfs_close_inode(dir_s);
    vfs_close_inode(dir_d);
    errno = 0;
    return 0;
}
EXPORT_SYMBOL(vfs_rename, 0);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

//inode_t *vfs_mkfifo(vfs_t *vfs, const char *path, acl_t *acl, int mode) {}
//EXPORT_SYMBOL(vfs_mkfifo, 0);

inode_t *vfs_mount(vfs_t *vfs, const char *dev, const char *fstype, const char *path, user_t *user, const char *options)
{
    assert(fstype != NULL && path != NULL);
    fnode_t *node = vfs_search(vfs, path, user, false, false);
    if (node == NULL)
        return NULL;

    if (vfs_lookup(node) == 0) {
        vfs_close_fnode(node);
        errno = EEXIST;
        return NULL;
    }

    mtx_lock(&node->mtx);
    if (node->mode != FN_NOENTRY) {
        mtx_unlock(&node->mtx);
        errno = EEXIST;
        return NULL;
    }

    splock_lock(&__vfs_share->lock);
    fsreg_t *fs = hmp_get(&__vfs_share->fs_hmap, fstype, strlen(fstype));
    splock_unlock(&__vfs_share->lock);
    if (fs == NULL || fs->mount == NULL) {
        mtx_unlock(&node->mtx);
        vfs_close_fnode(node);
        errno = ENOSYS;
        return NULL;
    }

    inode_t *dev_ino = NULL;
    if (dev != NULL) {
        fnode_t *dev_node = vfs_search(vfs, dev, user, true, true);

        if (dev_node == NULL) {
            mtx_unlock(&node->mtx);
            vfs_close_fnode(node);
            errno = ENODEV;
            return NULL;
        }

        dev_ino = vfs_inodeof(dev_node);
        vfs_close_fnode(dev_node);
        if (dev_ino == NULL) {
            mtx_unlock(&node->mtx);
            vfs_close_fnode(node);
            errno = ENODEV;
            return NULL;
        }
    }

    inode_t *ino = fs->mount(dev_ino, options);
    if (dev_ino)
        vfs_close_inode(dev_ino);
    if (ino == NULL) {
        mtx_unlock(&node->mtx);
        vfs_close_fnode(node);
        assert(errno != 0);
        return NULL;
    }

    if (ino->type != FL_DIR) {
        mtx_unlock(&node->mtx);
        vfs_close_fnode(node);
        vfs_close_inode(ino);
        errno = ENOTDIR;
        return NULL;
    }

    fnode_t *dir = node->parent;
    splock_lock(&dir->lock);
    ll_append(&dir->mnt, &node->nmt);
    splock_unlock(&dir->lock);

    vfs_resolve(node, ino);
    node->is_mount = true;
    mtx_unlock(&node->mtx);
    errno = 0;
    splock_lock(&__vfs_share->lock);
    ll_append(&__vfs_share->mnt_list, &node->nlru);
    splock_unlock(&__vfs_share->lock);
    // vfs_open_fnode(node);
    return ino;
}
EXPORT_SYMBOL(vfs_mount, 0);

int vfs_umount_at(fnode_t *node, user_t *user, int flags)
{
    splock_lock(&__vfs_share->lock);
    bool is_mounted = ll_contains(&__vfs_share->mnt_list, &node->nlru);
    splock_unlock(&__vfs_share->lock);
    if (!is_mounted /* || acl_cap(CAP_MOUNT) != 0 */) {
        errno = EPERM;
        return -1;
    }

    inode_t *dir = vfs_parentof(node);
    if (vfs_access(dir, NULL, VM_EX | VM_WR) != 0) {
        errno = EACCES;
        vfs_close_inode(dir);
        return -1;
    }

    vfs_close_inode(dir);
    splock_lock(&__vfs_share->lock);
    ll_remove(&__vfs_share->mnt_list, &node->nlru);
    splock_unlock(&__vfs_share->lock);
    vfs_close_fnode(node);
    return 0;
}

int vfs_umount(vfs_t *vfs, const char *path, user_t *user, int flags)
{
    fnode_t *node = vfs_search(vfs, path, user, true, true);
    if (node == NULL)
        return -1;
    return vfs_umount_at(node, user, flags);
}
EXPORT_SYMBOL(vfs_umount, 0);


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void vfs_scavenge()
{

}
