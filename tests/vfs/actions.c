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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "cli-vfs.h"

#ifndef O_BINARY
# define O_BINARY 0
#endif
#ifndef O_NOFOLLOW
# define O_NOFOLLOW 0x100000
#endif

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

size_t cli_read_size(char *str);

int do_dump(vfs_ctx_t *ctx, size_t *param)
{
    vfs_t *vfs = ctx->vfs;
    fnode_t *node = vfs->root;
    int indent = 0;
    char tmp[16];
    memset(tmp, ' ', indent * 2);
    tmp[indent * 2] = '\0';
    printf("%s- %s.%d\n", tmp, node->name, node->mode);
    return 0;
}

int do_umask(vfs_ctx_t *ctx, size_t *param)
{
    int mask = param[0];
    if ((mask & 0777) != mask) {
        printf("Invalid value for umask: %o\n", mask);
        return -1;
    }
    vfs_t *vfs = ctx->vfs;
    vfs->umask = mask;
    return 0;
}

int do_ls(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    inode_t *ino;

    char tmp[16];
    char buf[256];

    void *dctx = vfs_opendir(ctx->vfs, path, ctx->user);
    if (!dctx) {
        printf("Error during directory reading\n");
        return -1;
    }

    while ((ino = vfs_readdir(dctx, buf, 256)) != NULL) {
        printf("    %s (%s)\n", buf, vfs_inokey(ino, tmp));
        vfs_close_inode(ino);
    }

    vfs_closedir(dctx);
    return 0;
}

static const char *__timestr(xtime_t clock, char *tmp)
{
    time_t time = USEC_TO_SEC(clock);
    struct tm *tm = gmtime(&time);
    tm->tm_year += 1900;
    snprintf(tmp, 32, "%04d-%02d-%02d %02d:%02d:%02d.%03d", tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)((clock / 1000) % 1000));
    return tmp;
}

static int __do_stat(vfs_ctx_t *ctx, size_t *param, bool follow)
{
    static const char *rights[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx" };

    char *path = (char *)param[0];
    char *type = (char *)param[1];
    int mode = param[2];
    // char *uid = (char *)param[3];
    // char *gid = (char *)param[4];

    fnode_t *node = vfs_search(ctx->vfs, path, ctx->user, true, follow);
    if (node == NULL)
        return cli_error("No such entry\n");

    char tmp[32];
    printf("Found file %s\n", vfs_inokey(node->ino, tmp));
    printf("    Size: %s\n", sztoa(node->ino->length));
    printf("    Mode: %s%s%s\n", rights[(node->ino->mode >> 6) & 7], rights[(node->ino->mode >> 3) & 7], rights[(node->ino->mode >> 0) & 7]);
    printf("  Access: %s\n", __timestr(node->ino->atime, tmp));
    printf(" Created: %s\n", __timestr(node->ino->ctime, tmp));
    printf("Modified: %s\n", __timestr(node->ino->mtime, tmp));
    printf("   Birth: %s\n", __timestr(node->ino->btime, tmp));

    if (type != NULL) {
        if (strcmp(type, "DIR") == 0 && node->ino->type != FL_DIR) {
            vfs_close_fnode(node);
            return cli_error("Entry is not a directory\n");
        } else if (strcmp(type, "REG") == 0 && node->ino->type != FL_REG) {
            vfs_close_fnode(node);
            return cli_error("Entry is not a regular file\n");
        }
    }

    if (mode != 0 && mode != node->ino->mode) {
        vfs_close_fnode(node);
        return cli_error("Bad mode\n");
    }
    // TODO -- Check uid/gid 

    vfs_close_fnode(node);
    return 0;
}

int do_stat(vfs_ctx_t *ctx, size_t *param)
{
    return __do_stat(ctx, param, true);
}

int do_lstat(vfs_ctx_t *ctx, size_t *param)
{
    return __do_stat(ctx, param, false);
}

int do_link(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    char *path2 = (char *)param[1];
    int ret = vfs_link(ctx->vfs, path2, ctx->user, path);
    if (ret != 0)
        return cli_error("Unable to create link %s", path2);
    return 0;
}

int do_links(vfs_ctx_t *ctx, size_t * param)
{
    char *path = (char *)param[0];
    int count = param[1];

    fnode_t *node = vfs_search(ctx->vfs, path, ctx->user, true, true);
    if (node == NULL)
        return cli_error("No such entry");

    printf("Node have %d links\n", node->ino->links);
    if (count != 0 && node->ino->links != count) {
        vfs_close_fnode(node);
        return cli_error("Invalid links count");
    }

    vfs_close_fnode(node);
    return 0;
}

int do_times(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    char *mode = (char *)param[1];
    // char *cmp = (char *)param[2];
    for (int i = 0; mode && mode[i]; ++i)
        mode[i] = tolower(mode[i]);

    fnode_t *node = vfs_search(ctx->vfs, path, ctx->user, true, true);
    if (node == NULL)
        return cli_error("No such entry\n");

    char tmp[32];
    printf("%s: %s\n", path, vfs_inokey(node->ino, tmp));
    if (mode == NULL || strchr(mode, 'a') != NULL)
        printf("  access: %s\n", __timestr(node->ino->atime, tmp));
    if (mode == NULL || strchr(mode, 'c') != NULL)
        printf(" created: %s\n", __timestr(node->ino->ctime, tmp));
    if (mode == NULL || strchr(mode, 'm') != NULL)
        printf("modified: %s\n", __timestr(node->ino->mtime, tmp));
    if (mode == NULL || strchr(mode, 'b') != NULL)
        printf("   birth: %s\n", __timestr(node->ino->btime, tmp));

    // TODO -- TESTS
    vfs_close_fnode(node);
    return 0;
}

int do_cd(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    int ret = vfs_chdir(ctx->vfs, path, ctx->user, false);
    if (ret != 0)
        return cli_error("Error on changing working directory (%d: %s)\n", errno, strerror(errno));
    return 0;
}

int do_chroot(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    int ret = vfs_chdir(ctx->vfs, path, ctx->user, true);
    if (ret != 0)
        return cli_error("Error on changing root directory (%d: %s)\n", errno, strerror(errno));
    return 0;
}

int do_pwd(vfs_ctx_t *ctx, size_t *param)
{
    char buf[4096];
    int ret = vfs_readpath(ctx->vfs, ".", ctx->user, buf, 4096, false);
    if (ret == 0)
        printf("Pwd: %s\n", buf);
    else
        return cli_error("Readlink failure (%d: %s)\n", errno, strerror(errno));
    return 0;
}

int do_open(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    char *flags = (char *)param[1];
    int mode = (int)param[2];
    char *save = (char *)param[3];
    if (mode == 0)
        mode = 0644;

    int opt = 0;
    char *r;
    char *ptr = strtok_r(flags, ":", &r);
    for (; ptr; ptr = strtok_r(NULL, ":", &r)) {
        if (strcmp(ptr, "RDONLY") == 0)
            opt |= O_RDONLY;
        else if (strcmp(ptr, "WRONLY") == 0)
            opt |= O_WRONLY;
        else if (strcmp(ptr, "RDWR") == 0)
            opt |= O_RDWR;
        else if (strcmp(ptr, "CREAT") == 0)
            opt |= O_CREAT;
        else if (strcmp(ptr, "TRUNC") == 0)
            opt |= O_TRUNC;
        else if (strcmp(ptr, "EXCL") == 0)
            opt |= O_EXCL;
        else if (strcmp(ptr, "NOFOLLOW") == 0)
            opt |= O_NOFOLLOW;
        else
            printf("\033[m35Unknow OPEN mode '%s'\033[m0", ptr);
        // O_APPEND | O_NOINHERIT | O_RANDOM | O_RAW | O_SEQUENTIAL | O_TEMPORARY | O_TEXT | O_BINARY;
    }

    inode_t *ino = vfs_open(ctx->vfs, path, ctx->user, mode, opt);
    if (save != NULL && ino != NULL) {
        file_t *fp = malloc(sizeof(fp));
        fp->ino = ino;
        fp->rights = ((opt & (O_RDONLY | O_RDWR)) ? VM_RD : 0) | ((opt & (O_WRONLY | O_RDWR)) ? VM_WR : 0);
        if (fp->rights == 0)
            fp->rights = VM_RD;
        cli_store(save, fp, ST_INODE);
    }
    else if (ino != NULL)
        vfs_close_inode(ino);
    else
        return cli_error("Unable to open file: %s : [%d - %s]\n", path, errno, strerror(errno));
    return 0;
}

int do_close(vfs_ctx_t *ctx, size_t *param)
{
    char *save = (char *)param[0];
    file_t *fp = cli_fetch(save, ST_INODE);
    if (fp == NULL)
        return cli_error("Unable to find inode : %s", save);
    cli_remove(save, ST_INODE);
    vfs_usage(fp->ino, fp->rights, -1);
    vfs_close_inode(fp->ino);
    free(fp);
    return 0;
}

int do_read(vfs_ctx_t *ctx, size_t *param)
{
    char *buffer = (char *)param[0];
    char *path = (char *)param[1];
    size_t len = cli_read_size((char *)param[2]);
    xoff_t off = cli_read_size((char *)param[3]);

    inode_t *ino = NULL;
    if (*path == '@') {
        file_t *fp = cli_fetch(path, ST_INODE);
        if (fp && fp->rights & VM_RD)
            ino = fp->ino;
    }
    else
        ino = vfs_search_ino(ctx->vfs, path, ctx->user, true);
    if (ino == NULL)
        return cli_error("Unable to find inode: %s", path);

    buf_t *buf = cli_fetch(buffer, ST_BUFFER);
    if (buf && buf->len < len) {
        free(buf);
        buf = NULL;
    }
    if (buf == NULL) {
        buf = malloc(sizeof(buf_t) + len);
        buf->len = len;
        cli_store(buffer, buf, ST_BUFFER);
    }

    int ret = vfs_read(ino, buf->ptr, len, off, 0) > 0 ? 0 : -1;

    if (*path != '@')
        vfs_close_inode(ino);
    return ret;
}

int do_write(vfs_ctx_t *ctx, size_t *param)
{
    char *buffer = (char *)param[0];
    char *path = (char *)param[1];
    size_t len = cli_read_size((char *)param[2]);
    xoff_t off = cli_read_size((char *)param[3]);

    inode_t *ino = NULL;
    if (*path == '@') {
        file_t *fp = cli_fetch(path, ST_INODE);
        if (fp && fp->rights & VM_WR)
            ino = fp->ino;
    }
    else
        ino = vfs_search_ino(ctx->vfs, path, ctx->user, true);
    if (ino == NULL)
        return cli_error("Unable to find inode: %s", path);

    int ret = -1;
    buf_t *buf = cli_fetch(buffer, ST_BUFFER);
    if (buf != NULL) {
        ret = vfs_write(ino, buf->ptr, MAX(buf->len, len), off, 0) > 0 ? 0 : -1;
    } else {
        cli_error("Unable to find buffer: %s", buffer);
    }

    if (*path != '@')
        vfs_close_inode(ino);
    return ret;
}

int do_rmbuf(vfs_ctx_t *ctx, size_t *param)
{
    char *buffer = (char *)param[0];
    buf_t *buf = cli_fetch(buffer, ST_BUFFER);
    cli_remove(buffer, ST_BUFFER);
    free(buf);
    return 0;
}

int do_delay(vfs_ctx_t * ctx, size_t * param)
{
    struct timespec xt;
    xt.tv_sec = 1;
    xt.tv_nsec = 0;
    thrd_sleep(&xt, NULL);
    return 0;
}

int do_create(vfs_ctx_t *ctx, size_t *param)
{
    char *name = (char *)param[0];
    int mode = param[1];
    if (mode == 0)
        mode = 0644;

    inode_t *ino = vfs_open(ctx->vfs, name, ctx->user, mode, O_CREAT | O_EXCL);
    if (ino == NULL)
        return cli_error("Unable to create file %s: [%d - %s]\n", name, errno, strerror(errno));

    vfs_close_inode(ino);
    return 0;
}

int do_unlink(vfs_ctx_t *ctx, size_t *param)
{
    const char *name = (char *)param[0];

    int ret = vfs_unlink(ctx->vfs, name, ctx->user);
    if (ret != 0)
        cli_error("Unable to remove file %s: [%d - %s]\n", name, errno, strerror(errno));
    cli_info("Removed file entry: %s\n", name);
    return ret;
}

int do_symlink(vfs_ctx_t *ctx, size_t *param)
{
    const char *name = (char *)param[0];
    const char *name2 = (char *)param[1];

    int ret = vfs_symlink(ctx->vfs, name2, ctx->user, name);
    if (ret != 0)
        return cli_error("Unable to create symlink %s: [%d - %s]\n", name, errno, strerror(errno));
    return 0;
}

int do_mkdir(vfs_ctx_t *ctx, size_t *param)
{
    char *name = (char *)param[0];
    int mask = param[1];
    if (mask == 0)
        mask = 0755;

    int ret = vfs_mkdir(ctx->vfs, name, ctx->user, mask);
    if (ret != 0)
        return cli_error("Unable to create directory %s: [%d - %s]\n", name, errno, strerror(errno));
    return 0;
}

int do_rmdir(vfs_ctx_t *ctx, size_t *param)
{
    const char *name = (char *)param[0];

    int ret = vfs_rmdir(ctx->vfs, name, ctx->user);
    if (ret != 0)
        return cli_error("Unable to remove directory %s: [%d - %s]\n", name, errno, strerror(errno));
    cli_info("Remove directory: %s\n", name);
    return ret;
}

int do_dd(vfs_ctx_t *ctx, size_t *param)
{
    const char *src = (char *)param[0];
    const char *dst = (char *)param[1];
    size_t bsz = cli_read_size((char *)param[2]);
    size_t len = cli_read_size((char *)param[3]);

    if (bsz == 0)
        bsz = 512;

    inode_t *src_ino = vfs_search_ino(ctx->vfs, src, ctx->user, true);
    if (src_ino == NULL)
        return cli_error("Unable to find file %s!\n", src);
    inode_t *dst_ino = vfs_open(ctx->vfs, dst, ctx->user, 0644, O_CREAT | O_WRONLY);
    if (dst_ino == NULL) {
        vfs_close_inode(src_ino);
        return cli_error("Unable to find file %s!\n", dst);
    }

    if (len == 0)
        len = src_ino->length;

    if (vfs_truncate(dst_ino, len) != 0)
        return cli_error("Error truncate file at: %s!\n", __func__);

    xoff_t off = 0;
    xoff_t soff = 0;
    char *buf = malloc(bsz);
    while (len > 0) {
        int cap = MIN(len, bsz);
        if (src_ino->length > 0 && (xoff_t)cap > src_ino->length - soff)
            cap = (int)(src_ino->length - soff);

        if (vfs_read(src_ino, buf, cap, soff, 0) < 0)
            return cli_error("Error reading file at: %s!\033[0m\n", __func__);

        if (vfs_write(dst_ino, buf, cap, off, 0) < 0)
            return cli_error("Error writing file at: %s!\n", __func__);

        off += cap;
        soff += cap;
        len -= cap;
        if (soff >= src_ino->length)
            soff = 0;
    }
    free(buf);
    vfs_close_inode(src_ino);
    vfs_close_inode(dst_ino);
    return 0;
}

int do_truncate(vfs_ctx_t *ctx, size_t *param)
{
    const char *path = (char *)param[0];
    size_t len = (size_t)param[1];

    inode_t *ino = vfs_search_ino(ctx->vfs, path, ctx->user, true);
    if (ino == 0)
        return cli_error("Unable to find file %s\n", path);
    int ret = vfs_truncate(ino, len);
    vfs_close_inode(ino);
    return ret;
}

int do_size(vfs_ctx_t *ctx, size_t *param)
{
    const char *path = (char *)param[0];
    size_t len = (size_t)param[1];

    inode_t *ino = vfs_search_ino(ctx->vfs, path, ctx->user, true);
    if (ino == 0)
        return cli_error("Unable to find file %s\n", path);
    printf("File size is (%s)\n", sztoa(ino->length));
    int ret = 0;
    if (len != 0 && (xoff_t)len != ino->length)
        printf("Real size if "XOFF_F"\n", ino->length);
    vfs_close_inode(ino);
    return ret;
}

int do_clear_cache(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];

    fnode_t *node = vfs_search(ctx->vfs, path, ctx->user, true, true);
    if (node == NULL)
        return cli_error("No such entry\n");
    device_t *dev = node->ino->dev;
    int ret = node->rcu == 1 ? 0 : -1;
    vfs_close_fnode(node);
    vfs_dev_scavenge(dev, -1);
    return ret;
}

int do_mount(vfs_ctx_t *ctx, size_t *param)
{
    char *dev = (char *)param[0];
    char *fstype = (char *)param[1];
    char *name = (char *)param[2];
    int ret = vfs_mount(ctx->vfs, strcmp(dev, "-") == 0 ? NULL : dev, fstype, name, ctx->user, "");
    if (ret != 0)
        return cli_error("\033[31mUnable to mount disk '%s': [%d - %s]\033[0m\n", dev, errno, strerror(errno));
    return 0;
}

int do_umount(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];

    int ret = vfs_umount(ctx->vfs, path, ctx->user, 0);
    if (ret != 0)
        cli_error("Error on umount '%s': [%d - %s]\033[0m\n", path, errno, strerror(errno));
    return ret;
}

int do_extract(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    char *out = (char *)param[1];
    fnode_t *node = vfs_search(ctx->vfs, path, ctx->user, true, true);

    if (node == NULL)
        return cli_error("No such entry\n");

    FILE *fp = fopen(out, "wb");

    xoff_t off = 0;
    char buffer[512];
    for (;;) {
        int ret = vfs_read(node->ino, buffer, 512, off, 0);
        if (ret < 0) {
            if (errno)
                printf("Reading error at "XOFF_F"\n", off);
            else
                printf("End of file at "XOFF_F" (expect "XOFF_F")\n", off, node->ino->length);
            break;
        } else if (ret == 0) {
            printf("Reading blocked at "XOFF_F"\n", off);
            break;
        }

        fwrite(buffer, 1, ret, fp);
        off += ret;
    };
    fclose(fp);
    vfs_close_fnode(node);
    return 0;
}

int do_checksum(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    char* out = (char*)param[1];
    uint32_t chk = out == NULL ? 0 : strtoul(out, NULL, 16);

    fnode_t *node = vfs_search(ctx->vfs, path, ctx->user, true, true);

    if (node == NULL)
        return cli_error("No such entry\n");

    // Init checksum
    uint32_t checksum = 0xFFFFFFFF;

    xoff_t off = 0;
    char buffer[512];
    for (;;) {
        int ret = vfs_read(node->ino, buffer, 512, off, 0);
        if (ret < 0) {
            if (errno)
                printf("Reading error at "XOFF_F"\n", off);
            else
                printf("End of file at "XOFF_F" (expect "XOFF_F")\n", off, node->ino->length);
            break;
        } else if (ret == 0) {
            printf("Reading blocked at "XOFF_F"\n", off);
            break;
        }

        // Update checksum
        checksum = ~crc32_r(checksum, buffer, ret);
        off += ret;
    };

    // Complete checksum
    checksum = ~checksum;
    printf("Checksum %s   0x%08x\n", path, checksum);
    vfs_close_fnode(node);
    return (chk == 0 || chk == checksum) ? 0 : -1;
}

int do_img_open(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    char *name = (char *)param[1];
    int ret = imgdk_open(path, name);
    if (ret != 0)
        return cli_error("Unable to open disk image '%s': [%d - %s]\n", path, errno, strerror(errno));
    return 0;
}

int do_img_create(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    char *size = (char *)param[1];
    // char *type = (char *)param[2];
    char *ext = strrchr(path, '.');
    uint64_t hdSize = strtol(size, &size, 10);
    char sfx = (*size | 0x20);
    if (sfx == 'k')
        hdSize = hdSize << 10;
    else if (sfx == 'm')
        hdSize = hdSize << 20;
    else if (sfx == 'g')
        hdSize = hdSize << 30;
    else if (sfx == 't')
        hdSize = hdSize << 40;

    if (hdSize > (2ULL << 40))
        return cli_error("\033[31mImage disk is too big, max is 2Tb at %s!\033[0m\n", __func__);

    if (strcmp(ext, ".img") == 0)
        imgdk_create(path, hdSize);
    else if (strcmp(ext, ".vhd") == 0)
        vhd_create_dyn(path, hdSize);
    else
        return cli_error("\033[31mUnrecognized disk image format '%s' at %s!\033[0m\n", ext, __func__);
    return 0;
}

int do_img_remove(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    errno = 0;
    remove(path);
    if (errno == ENOENT)
        printf("Unable to find disk image %s\n", path);
    else if (errno != 0)
        printf("\033[31mUnable to remove disk image '%s': [%d - %s]\033[0m\n", path, errno, strerror(errno));
    else
        printf("Sucessfuly removed disk image %s\n", path);
    return 0;
}

int do_img_copy(vfs_ctx_t *ctx, size_t *param)
{
    char *src = (char *)param[0];
    char *dst = (char *)param[1];
    int size = (int)param[2];
    if (size <= 0)
        size = 512;

    int fsrc = open(src, O_RDONLY | O_BINARY);
    if (fsrc == -1)
        return cli_error("\033[31mUnable to open file %s!\033[0m\n", src);

    int fdst = open(dst, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY);
    if (fdst == -1) {
        printf("\033[31mUnable to open file %s!\033[0m\n", dst);
        close(fsrc);
        return -1;
    }

    int lg;
    char *buf = malloc(size);
    for (;;) {
        lg = read(fsrc, buf, 512);
        if (lg == 0)
            break;
        if (lg < 0) {
            printf("\033[31mError while reading file %s!\033[0m\n", src);
            free(buf);
            return -1;
        }
        lg = write(fdst, buf, lg);
        if (lg <= 0) {
            printf("\033[31mError while writing file %s!\033[0m\n", dst);
            free(buf);
            return -1;
        }
    }
    free(buf);
    close(fsrc);
    close(fdst);
    return 0;
}

int do_tar_mount(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    char *name = (char *)param[1];

    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
        return cli_error("\033[31mUnable to open tar file '%s': [%d - %s]\033[0m\n", path, errno, strerror(errno));

    size_t len = fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    void *buf = kalloc(ALIGN_UP(len, PAGE_SIZE));
    fread(buf, len, 1, fp);
    fclose(fp);
    inode_t *ino = tar_mount(buf, len, name);
    int ret = vfs_early_mount(ino, name);
    vfs_close_inode(ino);
    return ret;
}

int do_format(vfs_ctx_t *ctx, size_t *param)
{
    char *fs = (char *)param[0];
    char *path = (char *)param[1];
    char *options = (char *)param[2];
    fnode_t *blk = vfs_search(ctx->vfs, path, ctx->user, true, true);
    if (blk == NULL)
        return cli_error("Unable to find the device\n");

    if (blk->ino->type != FL_BLK) {
        printf("We can only format block devices\n");
        vfs_close_fnode(blk);
        return -1;
    }
    // TODO -- Get method by FS name
    int ret = vfs_format(fs, blk->ino, options);
    if (ret == -2)
        printf("Unknown file system '%s'\n", fs);
    else if (ret != 0)
        printf("Error during the format operation\n");

    vfs_close_fnode(blk);
    return 0;
}

int do_chmod(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    char *mode_str = (char *)param[1];

    int mode = 0;
    if (mode_str[0] == '0')
        mode = strtol(mode_str, NULL, 8);
    else {
        return cli_error("Not supported mode %s", mode_str);
    }

    int ret = vfs_chmod(ctx->vfs, path, ctx->user, mode);
    if (ret != 0)
        return cli_error("Error during chmod of %s (%d)", path, errno);
    return 0;
}

int do_chown(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    // char *uid = (char *)param[1];
    // char *gid = (char *)param[2];

    user_t *nacl = NULL;

    int ret = vfs_chown(ctx->vfs, path, ctx->user, nacl);
    if (ret != 0)
        return cli_error("Error during chown of %s (%d)", path, errno);
    return 0;
}

int do_utimes(vfs_ctx_t *ctx, size_t *param)
{
    char *path = (char *)param[0];
    char *utime = (char *)param[1];
    char *flags = (char *)param[2];

    xtime_t xtime = 0;
    if (strcmp(utime, "now") == 0) {
        xtime = xtime_read(XTIME_CLOCK);
    } else {
        xtime = strtoll(utime, NULL, 10) * _PwMicro_;
    }
    int tflags = 0;
    if (strchr(flags, 'C') != NULL)
        tflags |= FT_CREATED;
    if (strchr(flags, 'M') != NULL)
        tflags |= FT_MODIFIED;
    if (strchr(flags, 'A') != NULL)
        tflags |= FT_ACCESSED;
    if (strchr(flags, 'B') != NULL)
        tflags |= FT_BIRTH;

    int ret = vfs_utimes(ctx->vfs, path, ctx->user, xtime, tflags);
    if (ret != 0)
        return cli_error("Error during utimes of %s (%d)", path, errno);
    return 0;
}

int do_ino(vfs_ctx_t* ctx, size_t* param)
{
    char* path = (char*)param[0];
    // char* check = (char*)param[1];
    // char* save = (char*)param[2];

    fnode_t* node = vfs_search(ctx->vfs, path, ctx->user, true, true);
    if (node == NULL)
        return cli_error("Unable to find %s", path);

    inode_t* ino = vfs_inodeof(node);
    vfs_close_fnode(node);
    char tmp[16];
    printf("Inode at '%s' is '%s'\n", path, vfs_inokey(ino, tmp));
    vfs_close_inode(ino);
    return 0;
}

int do_rename(vfs_ctx_t* ctx, size_t* param)
{
    char* path_src = (char*)param[0];
    char* path_dst = (char*)param[1];
    int ret = vfs_rename(ctx->vfs, path_src, path_dst, ctx->user);
    if (ret != 0)
        return cli_error("Error during rename of %s to %s (%d)", path_src, path_dst, errno);
    return ret;
}

int do_pipe(vfs_ctx_t *ctx, size_t *param)
{
    char *store_rd = (char *)param[0];
    char *store_wr = (char *)param[1];
    inode_t *ino = vfs_pipe(ctx->vfs);
    if (ino == NULL)
        return cli_error("Unable to open a pipe");

    file_t *fp_rd = malloc(sizeof(file_t));
    fp_rd->ino = ino;
    fp_rd->rights = VM_RD;
    cli_store(store_rd, fp_rd, ST_INODE);

    file_t *fp_wr = malloc(sizeof(file_t));
    fp_wr->ino = vfs_open_inode(ino);
    fp_wr->rights = VM_WR;
    cli_store(store_wr, fp_wr, ST_INODE);
    return 0;
}
