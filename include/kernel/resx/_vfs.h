#ifndef __KERNEL_RESX__VFS_H
#define __KERNEL_RESX__VFS_H 1

#include <stddef.h>

char *vfs_inokey(inode_t *ino, char *buf);
inode_t *vfs_create(inode_t *dir, const char *name, ftype_t type, acl_t *acl, int flags);
inode_t *vfs_inode(unsigned no, ftype_t type, device_t *volume);
inode_t *vfs_lookup(inode_t *dir, const char *name);
inode_t *vfs_mount(const char *dev, const char *fs, const char *name);
inode_t *vfs_open(inode_t *ino, int access);
inode_t *vfs_readdir(inode_t *dir, char *name, void *ctx);
inode_t *vfs_search(inode_t *root, inode_t *pwd, const char *path, acl_t *acl);
inode_t *vfs_search_device(const char *name);
inode_t *vfs_symlink(inode_t *dir, const char *name, const char *path);
int vfs_access(inode_t *ino, int access, acl_t *acl);
int vfs_chmod(inode_t *ino, int mode);
int vfs_chown(inode_t *ino, acl_t *acl);
int vfs_chtimes(inode_t *ino, struct timespec *ts, int flags);
int vfs_closedir(inode_t *dir, void *ctx);
int vfs_fdisk(const char *dname, long parts, long *sz);
int vfs_link(inode_t *dir, const char *name, inode_t *ino);
int vfs_mkdev(inode_t *ino, const char *name);
int vfs_read(inode_t *ino, char *buf, size_t size, off_t offset, int flags);
int vfs_readlink(inode_t *ino, char *buf, int len, int flags);
int vfs_rename(inode_t *dir, const char *name, inode_t *ino);
int vfs_truncate(inode_t *ino, off_t lengtg);
int vfs_umount(inode_t *ino);
int vfs_unlink(inode_t *dir, const char *name);
int vfs_write(inode_t *ino, const char *buf, size_t size, off_t offset, int flags);
void *vfs_opendir(inode_t *dir, acl_t *acl);
void vfs_close(inode_t *ino, int access);
void vfs_fini();
void vfs_init();
void vfs_rmdev(const char *name);
void vfs_show_devices();

void register_fs(CSTR, fs_mount mount);
void unregister_fs(const char *name);



#endif  /* __KERNEL_RESX__VFS_H */
