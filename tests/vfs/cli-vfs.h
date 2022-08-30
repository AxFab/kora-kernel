#ifndef __CLI_VFS_H
#define __CLI_VFS_H 1

#include "../cli.h"
#include <kora/mcrs.h>
#include <kernel/stdc.h>
#include <kernel/vfs.h>


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#define ST_INODE 1
#define ST_BUFFER 2


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
typedef struct vfs_ctx
{
    fs_anchor_t *fsa;
    user_t *user;
    bool auto_scavenge;
} vfs_ctx_t;

typedef struct buf
{
    size_t len;
    char ptr[0];
} buf_t;

typedef struct file
{
    inode_t *ino;
    int rights;
} file_t;


int do_dump(vfs_ctx_t *ctx, size_t *param);
int do_umask(vfs_ctx_t *ctx, size_t *param);
int do_ls(vfs_ctx_t *ctx, size_t *param);
int do_stat(vfs_ctx_t *ctx, size_t *param);
int do_lstat(vfs_ctx_t *ctx, size_t *param);
int do_link(vfs_ctx_t *ctx, size_t *param);
int do_links(vfs_ctx_t *ctx, size_t *param);
int do_times(vfs_ctx_t *ctx, size_t *param);
int do_cd(vfs_ctx_t *ctx, size_t *param);
int do_chroot(vfs_ctx_t *ctx, size_t *param);
int do_pwd(vfs_ctx_t *ctx, size_t *param);
int do_open(vfs_ctx_t *ctx, size_t *param);
int do_close(vfs_ctx_t *ctx, size_t *param);
int do_read(vfs_ctx_t *ctx, size_t *param);
int do_write(vfs_ctx_t *ctx, size_t *param);
int do_rmbuf(vfs_ctx_t *ctx, size_t *param);
int do_delay(vfs_ctx_t *ctx, size_t *param);
int do_create(vfs_ctx_t *ctx, size_t *param);
int do_unlink(vfs_ctx_t *ctx, size_t *param);
int do_symlink(vfs_ctx_t *ctx, size_t *param);
int do_mkdir(vfs_ctx_t *ctx, size_t *param);
int do_rmdir(vfs_ctx_t *ctx, size_t *param);
int do_dd(vfs_ctx_t *ctx, size_t *param);
int do_truncate(vfs_ctx_t *ctx, size_t *param);
int do_size(vfs_ctx_t *ctx, size_t *param);
int do_clear_cache(vfs_ctx_t *ctx, size_t *param);
int do_mount(vfs_ctx_t *ctx, size_t *param);
int do_umount(vfs_ctx_t *ctx, size_t *param);
int do_extract(vfs_ctx_t *ctx, size_t *param);
int do_checksum(vfs_ctx_t *ctx, size_t *param);
int do_img_open(vfs_ctx_t *ctx, size_t *param);
int do_img_create(vfs_ctx_t *ctx, size_t *param);
int do_img_copy(vfs_ctx_t *ctx, size_t *param);
int do_img_remove(vfs_ctx_t *ctx, size_t *param);
int do_tar_mount(vfs_ctx_t *ctx, size_t *param);
int do_format(vfs_ctx_t *ctx, size_t *param);
int do_chmod(vfs_ctx_t *ctx, size_t *param);
int do_chown(vfs_ctx_t *ctx, size_t *param);
int do_utimes(vfs_ctx_t *ctx, size_t *param);
int do_ino(vfs_ctx_t *ctx, size_t *param);
int do_rename(vfs_ctx_t *ctx, size_t *param);
int do_pipe(vfs_ctx_t *ctx, size_t *param);


int alloc_check();

#ifdef _EMBEDED_FS
void fat_setup();
void fat_teardown();
void isofs_setup();
void isofs_teardown();
void ext2_setup();
void ext2_teardown();
#endif

int imgdk_open(const char *path, const char *name);
void imgdk_create(const char *name, size_t size);
void vhd_create_dyn(const char *name, uint64_t size);
uint32_t crc32_r(uint32_t checksum, const void *buf, size_t len);



#endif /* __CLI_VFS_H */
