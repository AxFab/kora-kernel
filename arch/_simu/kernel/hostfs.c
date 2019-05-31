#include <kernel/vfs.h>
#include <kernel/device.h>
#include <sys/stat.h>

void *hostfs_opendir(inode_t *dir)
{
    return NULL;
}

ino_ops_t hostfs_dir_ops = {
    .opendir = hostfs_opendir,
};

inode_t *hostfs_mount(const char *path, const char *name, int flags)
{
    struct stat st;
    lstat(path, &st);

    inode_t *ino = vfs_inode(st.st_ino, FL_DIR, NULL);
    ino->ops = &hostfs_dir_ops;

    vfs_mount2(ino, NULL, kSYS.dev_ino, name, "hostfs", flags);
    return ino;
}

int hostfs_setup()
{
    inode_t *ino = hostfs_mount(".", "boot", 0);
    vfs_close(ino);
}

