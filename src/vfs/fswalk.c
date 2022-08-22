#include <kernel/vfs.h>
#include <errno.h>

typedef struct pelmt pelmt_t;

struct path
{
    llhead_t list;
    fnode_t *node;
    bool absolute;
    bool directory;
};

struct pelmt
{
    llnode_t node;
    char name[1];
};


static fnode_t *vfs_release_path(path_t *path, bool keep_node)
{
    fnode_t *node = path->node;
    pelmt_t *el;
    for (;;) {
        el = itemof(ll_pop_front(&path->list), pelmt_t, node);
        if (el == NULL)
            break;
        kfree(el);
    }
    if (path->node && !keep_node) {
        vfs_close_fnode(path->node);
        node = NULL;
    }
    kfree(path);
    return node;
}

static path_t *vfs_breakup_path(vfs_t *vfs, const char *path)
{
    pelmt_t *el;
    path_t *pl = kalloc(sizeof(path_t));
    pl->node = vfs_open_fnode(*path == '/' ? vfs->root : vfs->pwd);

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
            if (el != NULL) {
                kfree(el);
                path += s;
                continue;
                //errno = EPERM;
                //vfs_release_path(pl, false);
                //return NULL;
            }
        }

        el = kalloc(sizeof(pelmt_t) + s + 1);
        memcpy(el->name, path, s);
        el->name[s] = '\0';
        ll_push_back(&pl->list, &el->node);
        path += s;
    }

    return pl;
}

static int vfs_concatpath(const char *lnk, path_t *path)
{
    if (*lnk == '/') {
        // TODO -- Reset node!!!
        errno = EIO;
        return -1;
    }

    llhead_t plist = INIT_LLHEAD;
    while (*lnk) {
        int s = 0;
        while (lnk[s] != '/' && lnk[s] != '\0')
            s++;

        if (s >= 256) {
            errno = ENAMETOOLONG;
            return -1;
        }

        pelmt_t *el = kalloc(sizeof(pelmt_t) + s + 1);
        memcpy(el->name, lnk, s);
        el->name[s] = '\0';
        ll_push_back(&plist, &el->node);
        lnk += s;
    }

    while (plist.count_ > 0) {
        pelmt_t *el = ll_dequeue(&plist, pelmt_t, node);
        ll_enqueue(&path->list, &el->node);
    }
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

const char *ftype_char = "?rbpcnslfd";

char *vfs_inokey(inode_t *ino, char *buf)
{
    snprintf(buf, 16, "%02d-%04d-%c", ino->dev->no, ino->no, ftype_char[ino->type]);
    return buf;
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

fnode_t *vfs_search(vfs_t *vfs, const char *pathname, acl_t *acl, bool resolve, bool follow)
{
    char *lnk_buf = NULL;
    path_t *path = vfs_breakup_path(vfs, pathname);
    if (path == NULL)
        return NULL;
    else if (path->list.count_ == 0) {
        if (path->node->mode != FN_OK) {
            if (vfs_lookup(path->node) != 0)
                return vfs_release_path(path, false);
        }
        assert(path->node->mode == FN_OK);
        return vfs_release_path(path, true);
    }

    int links = 0;
    for (;;) {
        if (vfs_lookup(path->node) != 0) {
            if (lnk_buf != NULL)
                kfree(lnk_buf);
            return vfs_release_path(path, false);
        }

        inode_t *ino = vfs_inodeof(path->node);
        if (ino->type == FL_LNK) {
            if (links++ > 25) {
                errno = ELOOP;
                if (lnk_buf != NULL)
                    kfree(lnk_buf);
                vfs_close_inode(ino);
                return vfs_release_path(path, false);
            }

            if (lnk_buf == NULL)
                lnk_buf = kalloc(sizeof(PAGE_SIZE));
            vfs_readlink(ino, lnk_buf, PAGE_SIZE);
            if (vfs_concatpath(lnk_buf, path) != 0) {
                if (lnk_buf != NULL)
                    kfree(lnk_buf);
                vfs_close_inode(ino);
                return vfs_release_path(path, false);
            }
            fnode_t *node = vfs_open_fnode(path->node->parent);
            vfs_close_fnode(path->node);
            path->node = node;
        } else if (ino->type != FL_DIR) {
            errno = ENOTDIR;
            if (lnk_buf != NULL)
                kfree(lnk_buf);
            vfs_close_inode(ino);
            return vfs_release_path(path, false);
        } else if (vfs_access(ino, acl, VM_EX) != 0) {
            errno = EACCES;
            if (lnk_buf != NULL)
                kfree(lnk_buf);
            vfs_close_inode(ino);
            return vfs_release_path(path, false);
        }
        vfs_close_inode(ino);

        pelmt_t *el = itemof(ll_pop_front(&path->list), pelmt_t, node);
        assert(el != NULL);

        if (strcmp(el->name, "..") == 0) {
            if (path->node == vfs->root) {
                errno = EPERM;
                if (lnk_buf != NULL)
                    kfree(lnk_buf);
                return vfs_release_path(path, false);
            }
            fnode_t *node = vfs_open_fnode(path->node->parent);
            vfs_close_fnode(path->node);
            path->node = node;
        } else {
            fnode_t *node = vfs_fsnode_from(path->node, el->name);
            vfs_close_fnode(path->node);
            path->node = node;
        }

        kfree(el);
        if (path->list.count_ == 0) {
            if (resolve) {
                if (vfs_lookup(path->node) != 0) {
                    if (lnk_buf != NULL)
                        kfree(lnk_buf);
                    return vfs_release_path(path, false);
                }
                assert(path->node->mode == FN_OK);
            }
            if (resolve && follow) {
                inode_t *ino = path->node->ino;
                if (ino->type == FL_LNK) {
                    if (links++ > 25) {
                        errno = ELOOP;
                        if (lnk_buf != NULL)
                            kfree(lnk_buf);
                        return vfs_release_path(path, false);
                    }

                    if (lnk_buf == NULL)
                        lnk_buf = kalloc(sizeof(PAGE_SIZE));
                    vfs_readlink(ino, lnk_buf, PAGE_SIZE);
                    fnode_t *node = vfs_open_fnode(path->node->parent);
                    vfs_close_fnode(path->node);
                    path->node = node;
                    if (vfs_concatpath(lnk_buf, path) != 0) {
                        if (lnk_buf != NULL)
                            kfree(lnk_buf);
                        return vfs_release_path(path, false);
                    }
                    continue;
                }
            }
            if (lnk_buf != NULL)
                kfree(lnk_buf);
            return vfs_release_path(path, true);
        }

    }
}

inode_t *vfs_search_ino(vfs_t *vfs, const char *pathname, acl_t *acl, bool follow)
{
    fnode_t *node = vfs_search(vfs, pathname, acl, true, follow);
    if (node == NULL)
        return NULL;
    inode_t *ino = vfs_inodeof(node);
    vfs_close_fnode(node);
    return ino;
}
