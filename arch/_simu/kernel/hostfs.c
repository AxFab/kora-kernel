#include <kernel/vfs.h>
#include <kernel/device.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

void *hostfs_opendir(inode_t *dir)
{
    return NULL;
}

ino_ops_t hostfs_dir_ops = {
    .opendir = hostfs_opendir,
};

inode_t *hostfs_open(inode_t *dir, const char *name, int mode, acl_t *acl, int flags)
{
    char path[256];
    snprintf(path, 256, "%s/%s", dir->info, name);
    struct stat st;
    if (lstat(path, &st) != 0) {
        errno = ENOENT;
        return NULL;
    }
    int type = FL_REG;
    inode_t *ino = vfs_inode(st.st_ino, type, dir->dev);
    if (ino->ops == NULL) {
        ino->ops = &hostfs_opendir;
        ino->btime = TMSPEC_TO_USEC(st.st_ctim);
        ino->ctime = TMSPEC_TO_USEC(st.st_ctim);
        ino->mtime = TMSPEC_TO_USEC(st.st_mtim);
        ino->atime = TMSPEC_TO_USEC(st.st_atim);
    }

    errno = 0;
    return ino;
}


fs_ops_t hostfs_fs_ops = {
    .open = hostfs_open,
};


inode_t *hostfs_mount(const char *path, int flags)
{
    struct stat st;
    lstat(path, &st);
    if (!S_ISDIR(st.st_mode)) {
        return NULL;
    }

    inode_t *ino = vfs_inode(st.st_ino, FL_DIR, NULL);
    ino->ops = &hostfs_dir_ops;
    ino->dev->fsops = &hostfs_fs_ops;
    ino->info = strdup(path);

    ino->dev->devname = strdup("miniboot");
    ino->dev->devclass = strdup("hostfs");
    return ino;
}

int hostfs_setup()
{
    inode_t *ino = hostfs_mount(".", 0);

    // vfs_mount2(ino, NULL, kSYS.dev_ino, name, "hostfs", flags);

    devfs_register(ino, NULL, "boot");

    vfs_close(ino);
}

