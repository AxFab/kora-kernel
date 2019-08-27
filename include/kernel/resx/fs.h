#ifndef __KERNEL_RESX_FS_H
#define __KERNEL_RESX_FS_H 1

#include <stddef.h>


rxfs_t *rxfs_clone(rxfs_t *);
rxfs_t *rxfs_create(inode_t *ino);
rxfs_t *rxfs_open(rxfs_t *);
void rxfs_chdir(rxfs_t *fs, inode_t *ino);
void rxfs_chroot(rxfs_t *fs, inode_t *ino);

inode_t *resx_fs_pwd(resx_fs_t *resx);
inode_t *resx_fs_root(resx_fs_t *resx);
resx_fs_t *resx_fs_create();
resx_fs_t *resx_fs_open(resx_fs_t *resx);
void resx_fs_chpwd(resx_fs_t *resx, inode_t *ino);
void resx_fs_chroot(resx_fs_t *resx, inode_t *ino);
void resx_fs_close(resx_fs_t *resx);


#endif  /* __KERNEL_RESX_FS_H */
