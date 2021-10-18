/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#include <kernel/vfs.h>
#include <string.h>
#include <errno.h>
#include <kora/llist.h>

atomic_int dev_no = 1;
llhead_t dev_head = INIT_LLHEAD;
splock_t dev_lock = INIT_SPLOCK;

void vfs_createfile(inode_t *ino);
void vfs_cleanfile(inode_t *ino);

/* An inode must be created by the driver using a call to `vfs_inode()' */
inode_t *vfs_inode(unsigned no, ftype_t type, device_t *device, const ino_ops_t *ops)
{
    if (device == NULL) {
        device = kalloc(sizeof(device_t));
        // TODO -- Give UniqueID / Register on
        device->no = atomic_xadd(&dev_no, 1);
        splock_lock(&dev_lock);
        ll_append(&dev_head, &device->node);
        splock_unlock(&dev_lock);
        // kprintf(KL_INO, "Alloc new device `%d`\n", device->no);
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

    inode = (inode_t *)kalloc(sizeof(inode_t)); // TODO -- Alloc while holding a spinlock !?
    inode->no = no;
    inode->type = type;
    inode->dev = device;
    inode->ops = ops;
    inode->rcu = 1;
    // kprintf(KL_INO, "Alloc new inode `%s`\n", vfs_inokey(inode, tmp));
    vfs_createfile(inode);
    atomic_inc(&device->rcu);

    inode->bnode.value_ = no;
    bbtree_insert(&device->btree, &inode->bnode);
    splock_unlock(&device->lock);
    return inode;
}

inode_t *vfs_open_inode(inode_t *ino)
{
    if (ino)
        atomic_inc(&ino->rcu);
    return ino;
}

void vfs_close_inode(inode_t *ino)
{
    if (ino == NULL)
        return;
    device_t *device = ino->dev;
    splock_lock(&device->lock);
    if (atomic_xadd(&ino->rcu, -1) != 1) {
        splock_unlock(&device->lock);
        return;
    }

    // kprintf(KL_INO, "Release inode `%s`\n", vfs_inokey(ino, tmp));
    bbtree_remove(&device->btree, ino->bnode.value_);
    splock_unlock(&device->lock);
    vfs_cleanfile(ino);
    if (ino->ops->close)
        ino->ops->close(NULL, ino); // TODO -- Can we find the parent?
    kfree(ino);

    if (atomic_xadd(&device->rcu, -1) != 1)
        return;

    assert(device->map.count == 0);
    hmp_destroy(&device->map);
    splock_lock(&dev_lock);
    ll_remove(&dev_head, &device->node);
    splock_unlock(&dev_lock);
    // kprintf(KL_INO, "Release device `%d`\n", device->no);
    kfree(device);
}

device_t *vfs_device_by_id(int no)
{
    device_t *dev = NULL;
    splock_lock(&dev_lock);
    for ll_each(&dev_head, dev, device_t, node) {
        if (dev->no == no)
            break;
    }
    splock_unlock(&dev_lock);
    return dev; // This pointer can be invalid already !?
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

fsnode_t *vfs_fsnode_from(fsnode_t *parent, const char *name)
{
    device_t *device = parent->ino->dev;
    splock_lock(&device->lock);
    int len = 4 + strnlen(name, 256);
    if (len >= 260) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    char *key = kalloc(len + 1); // TODO -- Annoying alloc
    *((uint32_t *)key) = parent->ino->no;
    strcpy(&key[4], name);
    fsnode_t *node = hmp_get(&device->map, key, len);
    if (node == NULL) {
        node = kalloc(sizeof(fsnode_t));
        // kprintf(KL_INO, "Alloc new fsnode `%s/%s`\n", vfs_inokey(parent->ino, tmp), name);
        mtx_init(&node->mtx, mtx_plain);
        node->parent = vfs_open_fsnode(parent);
        hmp_put(&device->map, key, len, node);
        strcpy(node->name, name);
    } else if (ll_contains(&device->llru, &node->nlru))
        ll_remove(&device->llru, &node->nlru);

    kfree(key);
    atomic_inc(&node->rcu);
    splock_unlock(&device->lock);
    return node;
}

void vfs_dev_scavenge(device_t *dev, int max)
{
    char *key = kalloc(256 + 4);
    while (max-- > 0) {
        splock_lock(&dev->lock);

        fsnode_t *node = ll_dequeue(&dev->llru, fsnode_t, nlru);
        if (node == NULL) {
            splock_unlock(&dev->lock);
            break;
        }

        int len = 4 + strlen(node->name);
        *((uint32_t *)key) = node->parent->ino->no;
        strcpy(&key[4], node->name);
        hmp_remove(&dev->map, key, len);

        splock_unlock(&dev->lock);

        vfs_close_fsnode(node->parent);
        // kprintf(KL_INO, "Release fsnode `%s/%s`\n", vfs_inokey(node->parent->ino, tmp), node->name);
        mtx_destroy(&node->mtx);
        vfs_close_inode(node->ino);
        kfree(node);
    }
    kfree(key);
}

fsnode_t *vfs_open_fsnode(fsnode_t *node)
{
    if (node)
        atomic_inc(&node->rcu);
    return node;
}

void vfs_close_fsnode(fsnode_t *node)
{
    if (node == NULL)
        return;
    if (atomic_xadd(&node->rcu, -1) == 1) {
        device_t *device = node->parent->ino->dev;
        splock_lock(&device->lock);
        if (node->rcu == 0)
            ll_enqueue(&device->llru, &node->nlru);
        splock_unlock(&device->lock);
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

typedef struct pelmt pelmt_t;

struct path {
    llhead_t list;
    fsnode_t *node;
    bool absolute;
    bool directory;
};

struct pelmt {
    llnode_t node;
    char name[1];
};


static fsnode_t *vfs_release_path(path_t *path, bool keep_node)
{
    fsnode_t *node = path->node;
    pelmt_t *el;
    for (;;) {
        el = itemof(ll_pop_front(&path->list), pelmt_t, node);
        if (el == NULL)
            break;
        kfree(el);
    }
    if (path->node && !keep_node) {
        vfs_close_fsnode(path->node);
        node = NULL;
    }
    kfree(path);
    return node;
}

static path_t *vfs_breakup_path(vfs_t *vfs, const char *path)
{
    pelmt_t *el;
    path_t *pl = kalloc(sizeof(path_t));
    pl->node = vfs_open_fsnode(*path == '/' ? vfs->root : vfs->pwd);

    while (*path) {
        while (*path == '/')
            path++;
        if (*path == '\0') {
            pl->directory = true;
            break;
        }

        int s = 0;
        while (path[s] != '/' && path[s] != '\0')
            s++;

        if (s >= 256) {
            errno = ENAMETOOLONG;
            vfs_release_path(pl, false);
            return NULL;
        }

        if (s == 1 && path[0] == '.') {
            path += s;
            continue;
        } else if (s == 2 && path[0] == '.' && path[1] == '.') {
            el = itemof(ll_pop_back(&pl->list), pelmt_t, node);
            if (el == NULL) {
                errno = EPERM;
                vfs_release_path(pl, false);
                return NULL;
            }
            kfree(el);
            path += s;
            continue;
        }

        el = kalloc(sizeof(pelmt_t) + s + 1);
        memcpy(el->name, path, s);
        el->name[s] = '\0';
        ll_push_back(&pl->list, &el->node);
        path += s;
    }

    return pl;
}

int vfs_lookup(fsnode_t *node)
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
                node->ino = ino;
                node->mode = FN_OK;
            } else
                node->mode = FN_NOENTRY;

            mtx_unlock(&node->mtx);
        }
    }
    if (node->mode == FN_NOENTRY) {
        errno = ENOENT;
        // vfs_close_fsnode(node);
        return -1;
    }
    return 0;
}

fsnode_t *vfs_search(vfs_t *vfs, const char *pathname, void *user, bool resolve)
{
    path_t *path = vfs_breakup_path(vfs, pathname);
    if (path == NULL)
        return NULL;
    else if (path->list.count_ == 0) {
        assert(path->node->mode == FN_OK);
        return vfs_release_path(path, true);
    }

    int links = 0;
    for (;;) {
        if (path->node->ino->type == FL_LNK) {
            if (links++ > 25) {
                errno = ELOOP;
                return vfs_release_path(path, false);
            }
            // Add symbolique link in front of the current path
            errno = ENOSYS;
            return vfs_release_path(path, false);
        } else if (path->node->ino->type != FL_DIR) {
            errno = ENOTDIR;
            return vfs_release_path(path, false);
        }
        // else if (vfs_access(ino, X_OK, acl) != 0) {
        //errno = EACCES;
        //kfree(el);
        //return NULL;
        // }

        pelmt_t *el = itemof(ll_pop_front(&path->list), pelmt_t, node);
        assert(el != NULL);

        fsnode_t *node = vfs_fsnode_from(path->node, el->name);
        vfs_close_fsnode(path->node);
        path->node = node;
        kfree(el);
        if (path->list.count_ == 0) {
            if (resolve) {
                if (vfs_lookup(node) != 0)
                    return vfs_release_path(path, false);
                assert(node->mode == FN_OK);
            }
            return vfs_release_path(path, true);
        }

        if (vfs_lookup(node) != 0)
            return vfs_release_path(path, false);

        assert(node->mode == FN_OK);
    }
}

int vfs_create(fsnode_t *node, void *user, int flags, int mode)
{
    assert(node != NULL);
    mtx_lock(&node->mtx);
    if (node->mode != FN_EMPTY && node->mode != FN_NOENTRY) {
        mtx_unlock(&node->mtx);
        errno = EEXIST;
        return -1;
    }

    assert(node->parent);
    inode_t *dir = node->parent->ino;
    if (dir->dev->flags & FD_RDONLY) {
        mtx_unlock(&node->mtx);
        errno = EROFS;
        return -1;
    }
    if (dir->ops->create == NULL) {
        mtx_unlock(&node->mtx);
        errno = ENOSYS;
        return -1;
    }

    inode_t *ino = dir->ops->create(dir, node->name, user, mode);
    if (ino != NULL) {
        node->ino = ino;
        node->mode = FN_OK;
    }
    mtx_unlock(&node->mtx);
    return ino ? 0 : -1;
}


const char *ftype_char = "?rbpcnslfd";

char *vfs_inokey(inode_t *ino, char *buf)
{
    snprintf(buf, 12, "%02d-%04d-%c", ino->dev->no, ino->no, ftype_char[ino->type]);
    return buf;
}


void vfs_usage(fsnode_t *node, int flags, int use)
{
    if (node->ino->fops && node->ino->fops->usage != NULL)
        node->ino->fops->usage(node->ino, flags, use);
}


int vfs_access(fsnode_t *node, int access)
{
    return 0;
}
