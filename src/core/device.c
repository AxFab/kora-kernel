#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/sys/inode.h>
#include <kernel/sys/device.h>
#include <kora/llist.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


llhead_t dev_list = INIT_LLHEAD;
llhead_t mnt_list = INIT_LLHEAD;

int vfs_mkdev(const char *name, inode_t *ino, const char *vendor,
              const char *class, const char *device, unsigned char id[16])
{
    int i;
    if (name == NULL || ino == NULL) {
        errno = EINVAL;
        return -1;
    }

    device_t *dev = (device_t *)kalloc(sizeof(device_t));
    dev->name = strdup(name);
    dev->ino = ino;
    if (vendor) {
        dev->vendor = strdup(vendor);
    }
    if (class) {
        dev->class = strdup(class);
    }
    if (device) {
        dev->device = strdup(device);
    }

    if (id != NULL) { // Check ID is not zero
        memcpy(dev->id, id, 16);
    } else {
        for (i = 0; i < 16; ++i) {
            dev->id[i] = rand() & 0xFF;
        }
    }

    kprintf(-1, "[VFS ] New device '%s' (%s, %s, %s, %s)\n", name, vendor, class,
            device, sztoa(ino->length));
    vfs_open(ino);
    ll_append(&dev_list, &dev->node);

    // TODO -- Use id to check if we know the device.
    errno = 0;
    return 0;
}

inode_t *vfs_mountpt(int no, const char *name, const char *fs, size_t size)
{
    inode_t *ino = vfs_inode(no, S_IFDIR, size);
    ino->mnt = (mountpt_t *)kalloc(sizeof(mountpt_t));
    ino->mnt->name = name != NULL ? strdup(name) : NULL;
    ino->mnt->fs = fs != NULL ? strdup(fs) : NULL;
    hmp_init(&ino->mnt->hmap, 16);
    ll_append(&mnt_list, &ino->mnt->node);
    kprintf(-1, "[VFS ] New mount point '%s' (%s)\n", name, fs);
    return ino;
}

inode_t *vfs_lookup_device(const char *name)
{
    device_t *dev;
    for ll_each(&dev_list, dev, device_t, node) {
        if (!strcmp(name, dev->name)) {
            return dev->ino;
        }
    }
    return NULL;
}

inode_t *vfs_lookup_mountpt(const char *name)
{
    mountpt_t *mnt;
    for ll_each(&mnt_list, mnt, mountpt_t, node) {
        if (!strcmp(name, mnt->name)) {
            return mnt->ino;
        }
    }
    return NULL;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

llhead_t fs_list = INIT_LLHEAD;

typedef struct fs_mod {
    char *name;
    vfs_fs_ops_t *ops;
    llnode_t node;
} fs_mod_t;

void register_filesystem(const char *name, vfs_fs_ops_t *ops)
{
    fs_mod_t *fs = (fs_mod_t *)kalloc(sizeof(fs_mod_t));
    fs->ops = ops;
    fs->name = strdup(name);
    ll_append(&fs_list, &fs->node);
}

void unregister_filesystem(const char *name)
{
    fs_mod_t *fs;
    for ll_each(&fs_list, fs, fs_mod_t, node) {
        if (!strcmp(name, fs->name)) {
            ll_remove(&fs_list, &fs->node);
            kfree(fs->name);
            kfree(fs);
            return;
        }
    }
}

vfs_fs_ops_t *vfs_fs(const char *name)
{
    fs_mod_t *fs;
    for ll_each(&fs_list, fs, fs_mod_t, node) {
        if (!strcmp(name, fs->name)) {
            return fs->ops;
        }
    }
    return NULL;
}
