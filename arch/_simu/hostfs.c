#include <kernel/vfs.h>
#include <kernel/device.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
// #include <unistd.h>
#include <time.h>

#ifdef _WIN32
#define lstat stat
#define S_ISDIR(m)  ((m & _S_IFMT) == S_IFDIR)
#define S_ISREG(m)  ((m & _S_IFMT) == S_IFREG)
#endif


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
        if (!(flags & VFS_CREAT)) {
            errno = ENOENT;
            return NULL;
        }
        if (mode == FL_REG) {
            int fd = open(path, O_CREAT | O_EXCL, 0644);
            if (fd == -1 || lstat(path, &st) != 0) {
                close(fd);
                errno = EIO;
                return NULL;
            }
            close(fd);
        } else if (mode == FL_DIR) {
            mkdir(path, 0755);
            if (lstat(path, &st) != 0) {
                errno = EIO;
                return NULL;
            }
        }
    } else if (!(flags & VFS_OPEN)) {
        errno = EEXIST;
        return NULL;
    }

    int type = 0;
    if (S_ISDIR(st.st_mode))
        type = FL_DIR;
    if (S_ISREG(st.st_mode))
        type = FL_REG;

    inode_t *ino = vfs_inode(st.st_ino, type, dir->dev);
    if (ino->ops == NULL) {
        ino->ops = &hostfs_dir_ops;
#ifndef _WIN32
        ino->btime = TMSPEC_TO_USEC(st.st_ctim);
        ino->ctime = TMSPEC_TO_USEC(st.st_ctim);
        ino->mtime = TMSPEC_TO_USEC(st.st_mtim);
        ino->atime = TMSPEC_TO_USEC(st.st_atim);
#else
        ino->btime = SEC_TO_USEC(st.st_ctime);
        ino->ctime = SEC_TO_USEC(st.st_ctime);
        ino->mtime = SEC_TO_USEC(st.st_mtime);
        ino->atime = SEC_TO_USEC(st.st_atime);
#endif
        ino->info = strdup(path);
    }

    errno = 0;
    return ino;
}

int hostfs_unlink(inode_t *dir, CSTR name)
{
    char path[256];
    snprintf(path, 256, "%s/%s", dir->info, name);
    struct stat st;
    if (lstat(path, &st) != 0) {
        errno = ENOENT;
        return -1;
    }

    int ret;
    if (S_ISDIR(st.st_mode))
        ret = rmdir(path);
    else if (S_ISREG(st.st_mode))
        ret = unlink(path);
    else {
        errno = ENOSYS;
        return -1;
    }
    if (ret != 0) {
        errno = EIO;
        return -1;
    }
    // TODO -- Set errno!
    return ret;
}


fs_ops_t hostfs_fs_ops = {
    .open = (void *)hostfs_open,
    .unlink = (void *)hostfs_unlink,
};


inode_t *hostfs_mount(const char *path, int flags)
{
    struct stat st;
    lstat(path, &st);
    if (!S_ISDIR(st.st_mode))
        return NULL;

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

    vfs_mkdev(ino, "boot");

    vfs_close(ino);
    return 0;
}

