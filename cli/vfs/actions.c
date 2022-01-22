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
#include <kernel/vfs.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#ifndef O_BINARY
# define O_BINARY 0
#endif
#ifndef O_NOFOLLOW
# define O_NOFOLLOW 0x100000
#endif


int imgdk_open(const char* path, const char* name);
void imgdk_create(const char* name, size_t size);
void vhd_create_dyn(const char* name, uint64_t size);


int do_dump(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    fsnode_t* node  = vfs->root;
    int indent = 0;
    char buf[256], tmp[16];
    memset(buf, ' ', indent * 2);
    buf[indent * 2] = '\0';
    strncat(buf, "- ", 256);
    strncat(buf, node->name, 256);
    snprintf(tmp, 16, ".%d", node->mode);
    strncat(buf, tmp, 256);
    printf("%s\n", buf);
    return 0;
}

int do_umask(vfs_t **ctx, size_t *param)
{
    int mask = param[0];
    if ((mask & 0777) != mask) {
        printf("Invalid value for umask: %o\n", mask);
        return -1;
    }
    vfs_t *vfs = *ctx;
    vfs->umask = mask;
    return 0;
}

int do_ls(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    char* path = (char*)param[0];
    fsnode_t* dir = vfs_search(vfs, path, NULL, true);
    fsnode_t* node;

    char tmp[16];
    if (dir == NULL) {
        printf("No such directory\n");
        return -1;
    }
    else if (dir->ino->type != FL_DIR) {
        vfs_close_fsnode(dir);
        printf("This file isn't a directory\n");
        return -1;
    }

    void* dctx = vfs_opendir(dir, NULL);
    if (!dctx) {
        vfs_close_fsnode(dir);
        printf("Error during directory reading\n");
        return -1;
    }

    while ((node = vfs_readdir(dir, dctx)) != NULL) {
        printf("    %s (%s)\n", node->name, vfs_inokey(node->ino, tmp));
        vfs_close_fsnode(node);
    }

    vfs_closedir(dir, dctx);
    vfs_close_fsnode(dir);
    return 0;
}

const char* vfs_timestr(xtime_t clock, char *tmp)
{
    time_t time = USEC_TO_SEC(clock);
    struct tm* tm = gmtime(&time);
    tm->tm_year += 1900;
    snprintf(tmp, 32, "%04d-%02d-%02d %02d:%02d:%02d.%03d", tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)((clock / 1000) % 1000));
    return tmp;
}

int do_stat(vfs_t **ctx, size_t* param)
{
    static const char *rights[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx" };

    vfs_t *vfs = *ctx;
    char* path = (char*)param[0];
    char* type = (char *)param[1];
    int mode = param[2];
    char *uid = (char *)param[3];
    char *gid = (char *)param[4];

    fsnode_t* node = vfs_search(vfs, path, NULL, true);

    if (node == NULL)
        return cli_error("No such entry\n");

    if (type != NULL) {
        if (strcmp(type, "DIR") == 0 && node->ino->type != FL_DIR) {
            vfs_close_fsnode(node);
            return cli_error("Entry is not a directory\n");
        } else if (strcmp(type, "REG") == 0 && node->ino->type != FL_REG) {
            vfs_close_fsnode(node);
            return cli_error("Entry is not a regular file\n");
        }
    }

    //if (mode != 0 && node->ino->mode != mode)
    //    return cli_error("Entry doesn't have the expected mode %o vs. %o\n", node->ino->mode, mode);
    


    char tmp[32];
    printf("Found file %s\n", vfs_inokey(node->ino, tmp));
    printf("    Size: %s\n", sztoa(node->ino->length));
    printf("    Mode: %s%s%s\n", rights[(node->ino->mode >> 6) & 7], rights[(node->ino->mode >> 3) & 7], rights[(node->ino->mode >> 0) & 7]);
    printf("  Access: %s\n", vfs_timestr(node->ino->atime, tmp));
    printf(" Created: %s\n", vfs_timestr(node->ino->ctime, tmp));
    printf("Modified: %s\n", vfs_timestr(node->ino->mtime, tmp));
    printf("   Birth: %s\n", vfs_timestr(node->ino->btime, tmp));

    if (mode != 0 && mode != node->ino->mode) {
        vfs_close_fsnode(node);
        return cli_error("Bad mode\n");
    }
    // TODO -- Check uid/gid 

    vfs_close_fsnode(node);
    return 0;
}

int do_links(vfs_t **ctx, size_t *param)
{
    vfs_t *vfs = *ctx;
    char *path = (char *)param[0];
    int count = param[1];

    fsnode_t *node = vfs_search(vfs, path, NULL, true);
    if (node == NULL)
        return cli_error("No such entry\n");
    else if (node->ino->type != FL_DIR) {
        vfs_close_fsnode(node);
        return cli_error("Is not a directory\n");
    }

    printf("Node have %d links\n", node->ino->links);
    if (count != 0 && node->ino->links != count) {
        vfs_close_fsnode(node);
        return cli_error("Invalid links count\n");
    }

    vfs_close_fsnode(node);
    return 0;
}

int do_times(vfs_t **ctx, size_t *param)
{
    vfs_t *vfs = *ctx;
    char *path = (char *)param[0];
    char *mode = (char *)param[1];
    char *cmp = (char *)param[2];
    for (int i = 0; mode && mode[i]; ++i)
        mode[i] = tolower(mode[i]);

    fsnode_t *node = vfs_search(vfs, path, NULL, true);
    if (node == NULL)
        return cli_error("No such entry\n");

    char tmp[32];
    printf("%s: %s\n", path, vfs_inokey(node->ino, tmp));
    if (mode == NULL || strchr(mode, 'a') != NULL)
        printf("  access: %s\n", vfs_timestr(node->ino->atime, tmp));
    if (mode == NULL || strchr(mode, 'c') != NULL)
        printf(" created: %s\n", vfs_timestr(node->ino->ctime, tmp));
    if (mode == NULL || strchr(mode, 'm') != NULL)
        printf("modified: %s\n", vfs_timestr(node->ino->mtime, tmp));
    if (mode == NULL || strchr(mode, 'b') != NULL)
        printf("   birth: %s\n", vfs_timestr(node->ino->btime, tmp));

    // TODO -- TESTS
    vfs_close_fsnode(node);
    return 0;
}

int do_cd(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    char* path = (char*)param[0];
    int ret = vfs_chdir(vfs, path, false);
    if (ret != 0)
        return cli_error("Error on changing working directory (%d: %s)\n", errno, strerror(errno));
    return 0;
}

int do_chroot(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    char* path = (char*)param[0];
    int ret = vfs_chdir(vfs, path, true);
    if (ret != 0)
        return cli_error("Error on changing root directory (%d: %s)\n", errno, strerror(errno));
    return 0;
}

int do_pwd(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    char buf[4096];
    int ret = vfs_readlink(vfs, vfs->pwd, buf, 4096, false);
    if (ret == 0)
        printf("Pwd: %s\n", buf);
    else
        return cli_error("Readlink failure (%d: %s)\n", errno, strerror(errno));
    return 0;
}

int do_open(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    char *path = (char *)param[0];
    char *flags = (char *)param[1];
    int mode = (char *)param[2];
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

    fsnode_t *node = vfs_open(vfs, path, NULL, opt, mode);
    if (node != NULL)
        vfs_close_fsnode(node);
    else 
        return cli_error("Unable to open file: %s : [%d - %s]\n", path, errno, strerror(errno));
    return 0;
}

int do_close(vfs_t **ctx, size_t* param)
{
    return cli_error("Not implemented: %s!\n", __func__);
}

int do_look(vfs_t **ctx, size_t* param)
{
    return cli_error("Not implemented: %s!\n", __func__);
}

int do_delay(vfs_t **ctx, size_t *param)
{
    xtime xt;
    xt.sec = 1;
    xt.nsec = 0;
    thrd_sleep(&xt, NULL);
    return 0;
}

int do_create(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    char *name = (char *)param[0];
    int mode = param[1];
    if (mode == 0)
        mode == 0644;
    fsnode_t* node = vfs_search(vfs, name, NULL, false);
    if (node == NULL)
        return cli_error("Unable to find path to directory %s: [%d - %s]\n", name, errno, strerror(errno));
    int ret = vfs_create(node, NULL, mode & (~vfs->umask & 0777));
    if (ret != 0)
        cli_error("Unable to create file %s: [%d - %s]\n", name, errno, strerror(errno));
    vfs_close_fsnode(node);
    return ret;
}

int do_unlink(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    const char *name = (char *)param[0];
    fsnode_t *node = vfs_search(vfs, name, NULL, false);
    if (node == NULL)
        return cli_error("Unable to find path to file %s: [%d - %s]\n", name, errno, strerror(errno));
    int ret = vfs_unlink(node);
    if (ret != 0)
        cli_error("Unable to remove file %s: [%d - %s]\n", name, errno, strerror(errno));
    vfs_close_fsnode(node);
    return ret;
}

int do_mkdir(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    char* name = (char*)param[0];
    int mask = param[1];
    if (mask == 0)
        mask = 0755;
    fsnode_t* node = vfs_search(vfs, name, NULL, false);
    if (node == NULL)
        return cli_error("Unable to find path to directory %s: [%d - %s]\n", name, errno, strerror(errno));
    int ret = vfs_mkdir(node, mask & (~vfs->umask & 0777), NULL);
    if (ret != 0)
        cli_error("Unable to create directory %s: [%d - %s]\n", name, errno, strerror(errno));
    vfs_close_fsnode(node);
    return ret;
}

int do_rmdir(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    const char *name = (char *)param[0];
    fsnode_t *node = vfs_search(vfs, name, NULL, false);
    if (node == NULL)
        return cli_error("Unable to find path to directory %s: [%d - %s]\n", name, errno, strerror(errno));
    int ret = vfs_rmdir(node);
    if (ret != 0)
        cli_error("Unable to remove directory %s: [%d - %s]\n", name, errno, strerror(errno));
    vfs_close_fsnode(node);
    return ret;
}

int do_dd(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    const char* dst = (char*)param[0];
    const char* src = (char*)param[1];
    size_t len = param[2];

    fsnode_t* srcNode = vfs_search(vfs, src, NULL, true);
    if (srcNode == NULL)
        return cli_error("Unable to find file %s!\n", src);
    fsnode_t* dstNode = vfs_search(vfs, dst, NULL, false);
    if (dstNode == NULL)
        return cli_error("Unable to find file %s!\n", dst);
    int ret = vfs_lookup(dstNode);
    if (ret != 0) {
        return cli_error("Unable to find file %s!\n", dst);
        // CREATE FILE !?
        // vfs_create(dstNode);
    }

    if (vfs_truncate(dstNode->ino, len) != 0) 
        return cli_error("Error truncate file at: %s!\n", __func__);

    xoff_t off = 0;
    char* buf = malloc(512);
    while (len > 0) {
        int cap = MIN(len, 512);
        if (vfs_read(srcNode->ino, buf, cap, off, 0) < 0) 
            return cli_error("Error reading file at: %s!\033[0m\n", __func__);

        if (vfs_write(dstNode->ino, buf, cap, off, 0) < 0) 
            return cli_error("Error writing file at: %s!\n", __func__);

        off += cap;
        len -= cap;
    }
    kfree(buf);
    vfs_close_fsnode(dstNode);
    return 0;
}

int do_clear_cache(vfs_t **ctx, size_t *param)
{
    vfs_t *vfs = *ctx;
    char *path = (char *)param[0];

    fsnode_t *node = vfs_search(vfs, path, NULL, true);
    if (node == NULL)
        return cli_error("No such entry\n");
    device_t *dev = node->ino->dev;
    int ret = node->rcu == 1 ? 0 : -1;
    vfs_close_fsnode(node);
    vfs_dev_scavenge(dev, -1);
    return ret;
}

int do_mount(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    char* dev = (char*)param[0];
    char* fstype = (char*)param[1];
    char* name = (char*)param[2];
    fsnode_t *node = vfs_mount(vfs, strcmp(dev, "-") == 0 ? NULL : dev, fstype, name, "");
    if (node == NULL)
        return cli_error("\033[31mUnable to mount disk '%s': [%d - %s]\033[0m\n", dev, errno, strerror(errno));
    else
        vfs_close_fsnode(node);
    return 0;
}

int do_umount(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    char *path = (char *)param[0];

    fsnode_t *node = vfs_search(vfs, path, NULL, true);
    if (node == NULL)
        return cli_error("No such entry\n");

    int ret = vfs_umount(node);
    if (ret != 0)
        cli_error("Error on umount '%s': [%d - %s]\033[0m\n", path, errno, strerror(errno));
    vfs_close_fsnode(node);
    return ret;
}

int do_extract(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    char* path = (char*)param[0];
    char* out = (char*)param[1];
    fsnode_t* node = vfs_search(vfs, path, NULL, true);

    if (node == NULL)
        return cli_error("No such entry\n");

    FILE* fp = fopen(out, "wb");

    xoff_t off = 0;
    char buffer[512];
    for (;;) {
        int ret = vfs_read(node->ino, buffer, 512, off, 0);
        if (ret < 0) {
            if (errno)
                printf("Reading error at %lld\n", off);
            else
                printf("End of file at %lld (expect %lld)\n", off, node->ino->length);
            break;
        }
        else if (ret == 0) {
            printf("Reading blocked at %lld\n", off);
            break;
        }

        fwrite(buffer, 1, ret, fp);
        off += ret;
    };
    fclose(fp);
    vfs_close_fsnode(node);
    return 0;
}




static uint32_t CRC32_T[] = { /* CRC polynomial 0xedb88320 */
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t crc32_r(uint32_t checksum, const void* buf, size_t len)
{
    const uint8_t* ptr = (const uint8_t*)buf;

    for (; len; --len, ++ptr)
        checksum = CRC32_T[(checksum ^ (*ptr)) & 0xFF] ^ (checksum >> 8);
    return ~checksum;
}


int do_checksum(vfs_t **ctx, size_t* param)
{
    vfs_t *vfs = *ctx;
    char* path = (char*)param[0];
    // char* out = (char*)param[1];
    fsnode_t* node = vfs_search(vfs, path, NULL, true);

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
                printf("Reading error at %lld\n", off);
            else
                printf("End of file at %lld (expect %lld)\n", off, node->ino->length);
            break;
        }
        else if (ret == 0) {
            printf("Reading blocked at %lld\n", off);
            break;
        }

        // Update checksum
        checksum = crc32_r(checksum, buffer, ret);
        off += ret;
    };

    // Complete checksum
    printf("Checksum %s   0x%08x\n", path, checksum); 
    vfs_close_fsnode(node);
    return 0;
}

int do_img_open(vfs_t **ctx, size_t* param)
{
    char* path = (char*)param[0];
    char* name = (char*)param[1];
    int ret = imgdk_open(path, name);
    if (ret != 0)
        return cli_error("Unable to open disk image '%s': [%d - %s]\n", path, errno, strerror(errno));
    return 0;
}

int do_img_create(vfs_t **ctx, size_t* param)
{
    char* path = (char*)param[0];
    char* size = (char*)param[1];
    char* type = (char*)param[2];
    char* ext = strrchr(path, '.');
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

int do_img_remove(vfs_t** ctx, size_t* param)
{
    char* path = (char*)param[0];
    errno = 0;
    remove(path);
    if (errno == ENOENT)
        printf("Unable to find disk image %s\n", path);
    else if  (errno != 0)
        printf("\033[31mUnable to remove disk image '%s': [%d - %s]\033[0m\n", path, errno, strerror(errno));
    else 
        printf("Sucessfuly removed disk image %s\n", path);
    return 0;
}

#pragma warning(disable : 4996)

int do_img_copy(vfs_t **ctx, size_t* param)
{
    char* src = (char*)param[0];
    char* dst = (char*)param[1];
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

int do_tar_mount(vfs_t **ctx, size_t* param)
{
    char* path = (char*)param[0];
    char* name = (char*)param[1];

    FILE* fp = fopen(path, "rb");
    if (fp == NULL) 
        return cli_error("\033[31mUnable to open tar file '%s': [%d - %s]\033[0m\n", path, errno, strerror(errno));

    size_t len = fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    void* buf = malloc(ALIGN_UP(len, PAGE_SIZE));
    fread(buf, len, 1, fp);
    tar_mount(buf, len, name);
    return 0;
}

int fatfs_format(inode_t* bdev, const char* options);
int ext2_format(inode_t* bdev, const char* options);

int do_format(vfs_t **ctx, size_t *param)
{
    vfs_t *vfs = *ctx;
    char* fs = (char*)param[0];
    char* path = (char*)param[1];
    char* options = (char*)param[2];
    fsnode_t* blk = vfs_search(vfs, path, NULL, true);
    if (blk == NULL) 
        return cli_error("Unable to find the device\n");

    if (blk->ino->type != FL_BLK) {
        printf("We can only format block devices\n");
        vfs_close_fsnode(blk);
        return -1;
    }
    // TODO -- Get method by FS name
    int ret = vfs_format(fs, blk->ino, options);
    if (ret == -2)
        printf("Unknown file system '%s'\n", fs);
    else if (ret != 0)
        printf("Error during the format operation\n");

    vfs_close_fsnode(blk);
    return 0;
}
