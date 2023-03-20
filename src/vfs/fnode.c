#include <kernel/vfs.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

fnode_t *vfs_fsnode_from(fnode_t *parent, const char *name)
{
    char tmp[16];
    assert(parent && parent->mode == FN_OK && parent->ino);
    //device_t *device = parent->ino->dev;
    // mtx_lock(&parent->mtx);
    int len = strnlen(name, 256);
    if (len >= 256) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    splock_lock(&__vfs_share->fnode_lock);
    fnode_t *node = hmp_get(&parent->map, name, len);
    if (node == NULL) {
        node = kalloc(sizeof(fnode_t));
        kprintf(KL_FSA, "Alloc new fsnode `%s/%s`\n", vfs_inokey(parent->ino, tmp), name);
        mtx_init(&node->mtx, mtx_plain);
        node->parent = vfs_open_fnode(parent);
        ll_append(&parent->clist, &node->cnode);
        hmp_init(&node->map, 8);
        hmp_put(&parent->map, name, len, node);
        strcpy(node->name, name);
    } else if (ll_contains(&__vfs_share->fnode_llru, &node->nlru)) {
        ll_remove(&__vfs_share->fnode_llru, &node->nlru);
        kprintf(KL_FSA, "Remove fsnode from LRU `%s/%s`\n", vfs_inokey(parent->ino, tmp), name);
    }

    atomic_inc(&node->rcu);
    splock_unlock(&__vfs_share->fnode_lock);
    // mtx_unlock(&parent->mtx);
    // kprintf(KL_FSA, "Open fsnode `%s/%s` (%d)\n", vfs_inokey(parent->ino, tmp), name, node->rcu);
    return node;
}

static void vfs_close_fnode_unlocked(fnode_t *node)
{
    char tmp[16];
    assert(node);
    // kprintf(KL_FSA, "Close fsnode `%s/%s` (%d)\n", node->parent ? vfs_inokey(node->parent->ino, tmp) : "", node->name, node->rcu);
    if (atomic_xadd(&node->rcu, -1) != 1)
        return;

    // TODO -- Do we destroy on some case !!?
    kprintf(KL_FSA, "Add fsnode to LRU `%s/%s`\n", node->parent ? vfs_inokey(node->parent->ino, tmp) : "", node->name);
    ll_enqueue(&__vfs_share->fnode_llru, &node->nlru);
}

// /!\ We must have spinlock on __vfs_share
static void vfs_destroy_fsnode(fnode_t *node)
{
    char tmp[16];

    fnode_t *parent = node->parent;
    int len = strnlen(node->name, 256);
    assert(len < 256);
    hmp_remove(&parent->map, node->name, len);

    // mtx_lock(&node->parent->mtx);
    if (node->is_mount)
        ll_remove(&node->parent->mnt, &node->nmt);

    ll_remove(&node->parent->clist, &node->cnode); // TODO -- IS clist usefull !!?
    kprintf(KL_FSA, "Release fsnode `%s/%s`\n", vfs_inokey(parent->ino, tmp), node->name);
    mtx_destroy(&node->mtx);
    vfs_close_fnode_unlocked(node->parent);
    vfs_close_inode(node->ino);
    kfree(node);
}

void vfs_close_fnode(fnode_t *node)
{
    assert(node);
    // kprintf(KL_FSA, "Close fsnode `%s/%s` (%d)\n", node->parent ? vfs_inokey(node->parent->ino, tmp) : "", node->name, node->rcu);
    if (atomic_xadd(&node->rcu, -1) != 1)
        return;
    splock_lock(&__vfs_share->fnode_lock);
    atomic_inc(&node->rcu);
    vfs_close_fnode_unlocked(node);
    splock_unlock(&__vfs_share->fnode_lock);
}

void vfs_scavenge(int max)
{
    if (max <= 0)
        max = __vfs_share->fnode_llru.count_;
    splock_lock(&__vfs_share->fnode_lock);
    while (max-- > 0) {
        fnode_t *node = ll_dequeue(&__vfs_share->fnode_llru, fnode_t, nlru);
        if (node == NULL)
            break;
        vfs_destroy_fsnode(node);
    }
    splock_unlock(&__vfs_share->fnode_lock);
}

fnode_t *vfs_open_fnode(fnode_t *node)
{
    assert(node);
    atomic_inc(&node->rcu);
    // kprintf(KL_FSA, "Open fsnode `%s/%s` (%d)\n", node->parent ? vfs_inokey(node->parent->ino, tmp) : "", node->name, node->rcu);
    return node;
}

static int vfs_clear_fsnode(fnode_t *node, int mode)
{
    char tmp[16];
    assert(mode != FN_OK);
    assert(node->parent);

    // Assertion mutex is locked !!?
    // mtx_lock(&node->mtx); // TODO -- Keep lock, destroy child later (move list)...
    kprintf(KL_FSA, "Unlink fsnode `%s/%s`\n", vfs_inokey(node->parent->ino, tmp), node->name);
    vfs_close_inode(node->ino);
    node->ino = NULL;
    node->mode = mode;
    // mtx_unlock(&node->mtx);
    return 0;
}

int vfs_lookup(fnode_t *node)
{
    if (node->mode == FN_EMPTY) {
        mtx_lock(&node->mtx);
        if (node->mode != FN_EMPTY)
            mtx_unlock(&node->mtx);
        else {
            inode_t *dir = node->parent->ino;
            assert(dir->ops->lookup != NULL);
            inode_t *ino = dir->ops->lookup(dir, node->name, NULL);
            // TODO - Handle error
            if (ino != NULL) {
                vfs_resolve(node, ino);
                vfs_close_inode(ino);
            } else
                node->mode = FN_NOENTRY;

            mtx_unlock(&node->mtx);
        }
    }
    if (node->mode == FN_NOENTRY) {
        errno = ENOENT;
        // vfs_close_fnode(node);
        return -1;
    }
    return 0;
}

inode_t *vfs_inodeof(fnode_t *node)
{
    assert(node && node->mode == FN_OK && node->ino);
    return vfs_open_inode(node->ino); // TODO -- Should we use RCU !?
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

int vfs_mkdir(fs_anchor_t *fsanchor, const char *name, user_t *user, int mode)
{
    int ret = -1;
    errno = 0;
    // TODO -- Check last entry can't be . or .. -> EINVAL or 'root'
    fnode_t *node = vfs_search(fsanchor, name, user, false, false);
    if (node == NULL || node->parent == NULL)
        goto err1;

    mtx_lock(&node->mtx);
    if (node->mode != FN_EMPTY && node->mode != FN_NOENTRY) {
        errno = EEXIST;
        goto err2;
    }

    inode_t *dir = vfs_inodeof(node->parent);
    assert(dir != NULL);
    if (dir->ops->mkdir == NULL) {
        errno = EPERM;
        goto err3;
    }

    assert(irq_ready());
    inode_t *ino = dir->ops->mkdir(dir, node->name, mode & (~fsanchor->umask & 07777), user);
    if (ino != NULL) {
        ret = 0;
        vfs_resolve(node, ino);
        vfs_close_inode(ino);
    }
err3:
    vfs_close_inode(dir);
err2:
    mtx_unlock(&node->mtx);
    vfs_close_fnode(node);

err1:
    assert((ret == 0) != (errno != 0));
    return ret;
}
EXPORT_SYMBOL(vfs_mkdir, 0);

int vfs_rmdir(fs_anchor_t *fsanchor, const char *name, user_t *user)
{
    errno = 0;
    fnode_t *node = vfs_search(fsanchor, name, user, true, false); // TODO -- can't be '.', '..' or '/'
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

    inode_t *dir = vfs_inodeof(node->parent);
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

static int vfs_create(fnode_t *node, void *user, int mode)
{
    assert(node != NULL);
    mtx_lock(&node->mtx);
    if (node->mode != FN_EMPTY && node->mode != FN_NOENTRY) {
        mtx_unlock(&node->mtx);
        errno = EEXIST;
        return -1;
    }

    assert(node->parent);
    inode_t *dir = vfs_inodeof(node->parent);
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

inode_t *vfs_open(fs_anchor_t *fsanchor, const char *name, user_t *user, int mode, int flags)
{
    int access = vfs_open_access(flags);
    if (access <= 0)
        return NULL;

    errno = 0;
    fnode_t *node = vfs_search(fsanchor, name, user, false, false);
    if (node == NULL)
        return NULL;

    bool created = false;
    if (vfs_lookup(node) != 0) {
        if (flags & O_CREAT) {
            created = true;
            int ret = vfs_create(node, NULL, mode & (~fsanchor->umask & 0777));
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

int vfs_unlink(fs_anchor_t *fsanchor, const char *name, user_t *user)
{
    int ret = -1;
    errno = 0;
    fnode_t *node = vfs_search(fsanchor, name, user, true, false);
    if (node == NULL)
        goto err1;

    mtx_lock(&node->mtx);
    if (node->mode == FN_NOENTRY) {
        errno = ENOENT;
        goto err2;
    }

    inode_t *dir = vfs_inodeof(node->parent);
    assert(dir != NULL);
    if (dir->ops->unlink == NULL) {
        errno = ENOSYS;
        goto err3;
    }
    ret = dir->ops->unlink(dir, node->name);
    if (ret == 0)
        vfs_clear_fsnode(node, FN_NOENTRY);
err3:
    vfs_close_inode(dir);
err2:
    mtx_unlock(&node->mtx);
    vfs_close_fnode(node);
err1:
    assert((ret == 0) != (errno != 0));
    return ret;
}
EXPORT_SYMBOL(vfs_unlink, 0);

int vfs_link(fs_anchor_t *fsanchor, const char *name, user_t *user, const char *path)
{
    int ret = -1;
    fnode_t *target = vfs_search(fsanchor, path, user, true, true);
    if (target == NULL)
        goto err1;

    errno = 0;
    // TODO -- Check last entry can't be . or .. -> EINVAL
    fnode_t *node = vfs_search(fsanchor, name, user, false, false);
    if (node == NULL)
        goto err2;

    inode_t *ino = vfs_inodeof(target);
    if (ino->type == FL_DIR) {
        errno = EPERM;
        goto err3;
    }

    mtx_lock(&node->mtx);
    if (node->mode != FN_EMPTY && node->mode != FN_NOENTRY) {
        mtx_unlock(&node->mtx);
        errno = EEXIST;
        goto err3;
    }

    inode_t *dir = vfs_inodeof(node->parent);
    assert(dir != NULL);
    if (dir->ops->link == NULL) {
        errno = EPERM;
        goto err4;
    }

    if (dir->dev != ino->dev) {
        errno = EXDEV;
        goto err4;
    }

    assert(irq_ready());
    ret = dir->ops->link(dir, node->name, ino);
    assert(irq_ready());
    if (ret == 0)
        vfs_resolve(node, ino);
err4:
    mtx_unlock(&node->mtx);
    vfs_close_inode(dir);
err3:
    vfs_close_inode(ino);
    vfs_close_fnode(node);
err2:
    vfs_close_fnode(target);
err1:
    assert((ret == 0) != (errno != 0));
    return ret;
}
EXPORT_SYMBOL(vfs_link, 0);

int vfs_symlink(fs_anchor_t *fsanchor, const char *name, user_t *user, const char *path)
{
    int ret = -1;
    errno = 0;
    fnode_t *node = vfs_search(fsanchor, name, user, false, false);
    if (node == NULL)
        goto err1;

    mtx_lock(&node->mtx);
    if (node->mode != FN_EMPTY && node->mode != FN_NOENTRY) {
        errno = EEXIST;
        goto err2;
    }

    assert(node->parent && node->parent->ino);
    inode_t *dir = vfs_inodeof(node->parent);
    if (dir->dev->flags & FD_RDONLY) {
        errno = EROFS;
        goto err3;
    }
    if (dir->ops->symlink == NULL) {
        errno = ENOSYS;
        goto err3;
    }

    assert(irq_ready());
    inode_t *ino = dir->ops->symlink(dir, node->name, user, path);
    if (ino != NULL) {
        ret = 0;
        vfs_resolve(node, ino);
        vfs_close_inode(ino);
    }
err3:
    vfs_close_inode(dir);
err2:
    mtx_unlock(&node->mtx);
    vfs_close_fnode(node);
err1:
    assert((ret == 0) != (errno != 0));
    return ret;
}
EXPORT_SYMBOL(vfs_symlink, 0);

int vfs_rename(fs_anchor_t *fsanchor, const char *src, const char *dest, user_t *user)
{
    int ret = -1;
    errno = 0;
    fnode_t *fsrc = vfs_search(fsanchor, src, user, true, true);
    if (fsrc == NULL)
        goto err1;
    fnode_t *fdst = vfs_search(fsanchor, dest, user, false, false);
    if (fdst == NULL)
        goto err2;

    inode_t *dir_s = vfs_inodeof(fsrc->parent);
    if (dir_s == NULL) {
        errno = ENOENT;
        goto err3;
    }

    inode_t *dir_d = vfs_inodeof(fdst->parent);
    if (dir_d == NULL) {
        errno = ENOENT;
        goto err4;
    }

    if (dir_s->dev != dir_d->dev) {
        errno = EXDEV;
        goto err5;
    } else if (dir_d->ops->rename == NULL) {
        errno = EPERM;
        goto err5;
    }

    inode_t *ino = vfs_inodeof(fsrc);
    if (ino == NULL) {
        errno = ENOENT;
        goto err6;
    }

    mtx_lock(&dir_s->dev->dual_lock);
    mtx_lock(&fdst->mtx);
    mtx_lock(&fsrc->mtx);
    assert(irq_ready());
    ret = dir_d->ops->rename(dir_s, fsrc->name, dir_d, fdst->name);
    assert(irq_ready());
    if (ret == 0) {
        vfs_resolve(fdst, ino);
        vfs_clear_fsnode(fsrc, FN_NOENTRY);
    }
    mtx_unlock(&fdst->mtx);
    mtx_unlock(&fsrc->mtx);
    mtx_unlock(&dir_s->dev->dual_lock);

err6:
    vfs_close_inode(ino);
err5:
    vfs_close_inode(dir_d);
err4:
    vfs_close_inode(dir_s);
err3:
    vfs_close_fnode(fdst);
err2:
    vfs_close_fnode(fsrc);
err1:
    assert((ret == 0) != (errno != 0));
    return ret;
}
EXPORT_SYMBOL(vfs_rename, 0);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int vfs_mount_at(fnode_t *node, inode_t *ino)
{
    vfs_lookup(node);
    mtx_lock(&node->mtx);
    if (node->mode != FN_NOENTRY) {
        mtx_unlock(&node->mtx);
        errno = EEXIST;
        return -1;
    }

    if (ino->type != FL_DIR) {
        mtx_unlock(&node->mtx);
        errno = ENOTDIR;
        return -1;
    }

    fnode_t *dir = node->parent;
    splock_lock(&dir->lock);
    ll_append(&dir->mnt, &node->nmt);
    splock_unlock(&dir->lock);

    vfs_resolve(node, ino);
    node->is_mount = true;
    errno = 0;
    splock_lock(&__vfs_share->lock);
    ll_append(&__vfs_share->mnt_list, &node->nlru);
    splock_unlock(&__vfs_share->lock);
    mtx_unlock(&node->mtx);
    kprintf(KL_MSG, "Mount drive as \033[35m%s\033[0m (%s)\n", ino->dev->devname, ino->dev->devclass);
    return 0;
}

int vfs_early_mount(inode_t *ino, const char *name)
{
    fnode_t *mnt = vfs_fsnode_from(__vfs_share->fsanchor->root, "mnt");
    if (vfs_lookup(mnt) != 0)
        return -1;
    fnode_t *node = vfs_fsnode_from(mnt, name);
    vfs_close_fnode(mnt);
    int ret = vfs_mount_at(node, ino);
    return ret;
}

int vfs_mount(fs_anchor_t *fsanchor, const char *dev, const char *fstype, const char *path, user_t *user, const char *options)
{
    assert(fstype != NULL && path != NULL);
    fnode_t *node = vfs_search(fsanchor, path, user, false, false);
    if (node == NULL) {
        assert(errno != 0);
        return -1;
    }

    splock_lock(&__vfs_share->lock);
    fsreg_t *fs = hmp_get(&__vfs_share->fs_hmap, fstype, strlen(fstype));
    splock_unlock(&__vfs_share->lock);
    if (fs == NULL || fs->mount == NULL) {
        errno = ENOSYS;
        vfs_close_fnode(node);
        return -1;
    }

    inode_t *dev_ino = NULL;
    if (dev != NULL) {
        dev_ino = vfs_search_ino(fsanchor, dev, user, true);
        if (dev_ino == NULL) {
            errno = ENODEV;
            vfs_close_fnode(node);
            return -1;
        }
    }
    inode_t *ino = fs->mount(dev_ino, options);
    if (dev_ino)
        vfs_close_inode(dev_ino);
    if (ino == NULL) {
        assert(errno != 0);
        return -1;
    }

    int ret = vfs_mount_at(node, ino);
    if (ret != 0) {
        vfs_close_fnode(node);
        return -1;
    }

    vfs_close_inode(ino);
    return 0;
}
EXPORT_SYMBOL(vfs_mount, 0);

int vfs_umount_at(fnode_t *node, user_t *user, int flags)
{
    inode_t *ino = vfs_inodeof(node);
    splock_lock(&__vfs_share->lock);
    bool is_mounted = ll_contains(&__vfs_share->mnt_list, &node->nlru);
    splock_unlock(&__vfs_share->lock);
    if (!is_mounted /* || acl_cap(CAP_MOUNT) != 0 */) {
        vfs_close_inode(ino);
        errno = EPERM;
        return -1;
    }

    inode_t *dir = vfs_inodeof(node->parent);
    if (vfs_access(dir, NULL, VM_EX | VM_WR) != 0) {
        errno = EACCES;
        vfs_close_inode(dir);
        vfs_close_inode(ino);
        return -1;
    }

    vfs_close_inode(dir);
    splock_lock(&__vfs_share->lock);
    ll_remove(&__vfs_share->mnt_list, &node->nlru);
    splock_unlock(&__vfs_share->lock);
    vfs_close_fnode(node);
    vfs_close_inode(ino);
    return 0;
}

int vfs_umount(fs_anchor_t *fsanchor, const char *path, user_t *user, int flags)
{
    fnode_t *node = vfs_search(fsanchor, path, user, true, true);
    if (node == NULL)
        return -1;
    if (!node->is_mount) {
        vfs_close_fnode(node);
        return -1;
    }
    int ret = vfs_umount_at(node, user, flags);
    vfs_close_fnode(node);
    return ret;
}
EXPORT_SYMBOL(vfs_umount, 0);

