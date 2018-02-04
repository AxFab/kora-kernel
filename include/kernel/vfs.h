#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H 1

#include <kernel/types.h>

typedef struct dirent dirent_t;
typedef struct module module_t;
struct stat;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


// inode_t *vfs_initialize();
// void vfs_sweep(inode_t *root);

// inode_t *vfs_inode(int mode, size_t size);

// inode_t *vfs_lookup(inode_t *dir, const char *name);

// inode_t *vfs_mkdir(inode_t *dir, const char *name, int mode);
// inode_t *vfs_create(inode_t *dir, const char *name, int mode);
// inode_t *vfs_symlink(inode_t *dir, const char *name, const char *link);
// inode_t *vfs_mkfifo(inode_t *dir, const char *name, int mode);
// int vfs_mknod(inode_t *dir, const char *name, inode_t *ino);

// int vfs_chown(inode_t *ino, uid_t uid, gid_t gid);
// int vfs_chmod(inode_t *ino, int mode);

// int vfs_unlink(inode_t *dir, const char *name);
// int vfs_rmdir(inode_t *dir, const char *name);

// inode_t *vfs_open(inode_t* ino);
// void vfs_close(inode_t* ino);


// int vfs_read(inode_t *ino, void* data, size_t size, off_t offset);
// int vfs_write(inode_t *ino, const void* data, size_t size, off_t offset);


#if 0
/* Open an entry for a specific path */
dirent_t *vfs_open_path(dirent_t *root, dirent_t *pwd, const char *path);
/* Close a path entry previouly open */
void vfs_close_path(dirent_t *entry);
/* - */
inode_t *vfs_create(dirent_t *entry, int mode, uid_t uid, gid_t gid,
                    struct timespec times);
/* - */
int vfs_link(dirent_t *entry);
/* Remove a directory entry */
int vfs_unlink(dirent_t *entry);
/* - */
int vfs_symlink(dirent_t *entry);
/* - */
int vfs_rename(dirent_t *entry);
/* - */
int vfs_readlink(dirent_t *entry);

/* - */
inode_t *vfs_open(dirent_t *entry);
/* - */
int vfs_close(inode_t *inode);
/* Changes the file mode bits of each given file according to mode. */
int vfs_chmod(inode_t *inode, int mode);
/* - */
int vfs_chown(inode_t *inode, uid_t user, gid_t group);
/* - */
int vfs_utimes(inode_t *inode, struct timespec *time);
/* - */
int vfs_truncate(inode_t *inode, off_t length);
/* Checks whether the user can access the file pathname. */
int vfs_access(inode_t *inode, uid_t uid, int access);
/* - */
int vfs_stat(inode_t *inode, struct stat *stat);

/* - */
int vfs_mount(dirent_t *entry, module_t *fs, dev_t dev);
/* - */
int vfs_unmount(dirent_t *entry);

/* - */
int vfs_register_irq(int no, int entry());
/* - */
int vfs_unregister_irq(int no, int entry());
#endif

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// int vfs_mkdev(const char *name, inode_t *ino, const char* vendor, const char* class, const char* device, unsigned char id[16]);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// struct kVfs {
//   inode_t *root;
//   inode_t *devIno;
// };

// extern struct kVfs kVFS;

// Devices
// IRQ
// PCI
// FS
// BLK/CHR

#endif /* _KERNEL_VFS_H */
