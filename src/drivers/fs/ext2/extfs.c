/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#include <kernel/core.h>
#include <kernel/drivers.h>
#include <errno.h>

#ifdef K_MODULE
#  define EXT_setup setup
#  define EXT_teardown teardown
#endif

extern vfs_fs_ops_t EXT_fs_ops;


static int EXT_inode_by_no(struct EXT_disk *disk, int no,
                           struct ext2_inode *ino)
{
    /* groupe qui contient l'inode */
    int gr_num = (i_num - 1) / disk->sb->s_inodes_per_group;

    /* index de l'inode dans le groupe */
    int index = (i_num - 1) % disk->sb->s_inodes_per_group;

    /* offset de l'inode sur le disk */
    off_t offset = disk->gd[gr_num].bg_inode_table * disk->blocksize + index *
                   disk->sb->s_inode_size;


    // !?
    uint8_t *map = kmap(ALIGN_UP(disk->sb->s_inode_size, PAGE_SIZE), disk->dev,
                        ALIGN_DOWN(offset, PAGE_SIZE), VMA_FG_RO_FILE);
    memcpy(ino, &map[offset & (PAGE_SIZE - 1)], disk->sb->s_inode_size);

    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


/* Retourne la structure d'inode à partir de son numéro */
EXT_inode *EXT_lookup(EXT_inode_t *dir, const char *name)
{
    // !?
}

char *EXT_read(EXT_inode_t *ino, char *buffer, size_t size, off_t offset)
{
    // !?
    // !?
    // !?


    char *mmap_base, *mmap_head, *buf;

    int *p, *pp, *ppp;
    int i, j, k;
    int n, size;

    buf = (char *) kmalloc(hd->blocksize);
    p = (int *) kmalloc(hd->blocksize);
    pp = (int *) kmalloc(hd->blocksize);
    ppp = (int *) kmalloc(hd->blocksize);

    /* taille totale du fichier */
    size = inode->i_size;
    mmap_head = mmap_base = kmalloc(size);

    /* direct block number */
    for (i = 0; i < 12 && inode->i_block[i]; i++) {
        disk_read(hd->device, inode->i_block[i] * hd->blocksize, buf, hd->blocksize);

        n = ((size > hd->blocksize) ? hd->blocksize : size);
        memcpy(mmap_head, buf, n);
        mmap_head += n;
        size -= n;
    }

    /* indirect block number */
    if (inode->i_block[12]) {
        disk_read(hd->device, inode->i_block[12] * hd->blocksize, (char *) p,
                  hd->blocksize);

        for (i = 0; i < hd->blocksize / 4 && p[i]; i++) {
            disk_read(hd->device, p[i] * hd->blocksize, buf, hd->blocksize);

            n = ((size > hd->blocksize) ? hd->blocksize : size);
            memcpy(mmap_head, buf, n);
            mmap_head += n;
            size -= n;
        }
    }

    /* bi-indirect block number */
    if (inode->i_block[13]) {
        disk_read(hd->device, inode->i_block[13] * hd->blocksize, (char *) p,
                  hd->blocksize);

        for (i = 0; i < hd->blocksize / 4 && p[i]; i++) {
            disk_read(hd->device, p[i] * hd->blocksize, (char *) pp, hd->blocksize);

            for (j = 0; j < hd->blocksize / 4 && pp[j]; j++) {
                disk_read(hd->device, pp[j] * hd->blocksize, buf, hd->blocksize);

                n = ((size > hd->blocksize) ? hd-> blocksize : size);
                memcpy(mmap_head, buf, n);
                mmap_head += n;
                size -= n;
            }
        }
    }

    /* tri-indirect block number */
    if (inode->i_block[14]) {
        disk_read(hd->device, inode->i_block[14] * hd->blocksize, (char *) p,
                  hd->blocksize);

        for (i = 0; i < hd->blocksize / 4 && p[i]; i++) {
            disk_read(hd->device, p[i] * hd->blocksize, (char *) pp, hd->blocksize);

            for (j = 0; j < hd->blocksize / 4 && pp[j]; j++) {
                disk_read(hd->device, pp[j] * hd->blocksize, (char *) ppp, hd->blocksize);

                for (k = 0; k < hd->blocksize / 4 && ppp[k]; k++) {
                    disk_read(hd->device, ppp[k] * hd->blocksize, buf, hd->blocksize);

                    n = ((size > hd->blocksize) ? hd->blocksize : size);
                    memcpy(mmap_head, buf, n);
                    mmap_head += n;
                    size -= n;
                }
            }
        }
    }

    kfree(buf);
    kfree(p);
    kfree(pp);
    kfree(ppp);

    return mmap_base;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

inode_t *EXT_mount(inode_t *dev)
{
    if (dev == NULL) {
        errno = ENODEV;
        return NULL;
    }

    struct EXT_disk *disk = (struct EXT_disk *)kalloc(sizeof(struct EXT_disk));

    /* Read the superblock */
    disk->sb_size = 1024;
    disk->sb = (struct ext2_super_block *)kmap(PAGE_SIZE, dev, sb_size,
               VMA_FG_RW_FILE);
    if (disk->sb == NULL) {
        kfree(disk);
        errno = ENOMEM;
        return NULL;
    }
    if (disk->sb->s_magic != 0xEF53) {
        kunmap(disk->sb, sb_size);
        kfree(disk);
        errno = EBADF;
        return NULL;
    }

    disk->dev = vfs_open(dev);

    disk->blocksize = 1024 << disk->sb->s_log_block_size;
    int i = (disk->sb->s_blocks_count / disk->sb->s_blocks_per_group) +
            ((disk->sb->s_blocks_count % disk->sb->s_blocks_per_group) ? 1 : 0);
    int j = (disk->sb->s_inodes_count / disk->sb->s_inodes_per_group) +
            ((disk->sb->s_inodes_count % disk->sb->s_inodes_per_group) ? 1 : 0);
    disk->groups = (i > j) ? i : j;

    /* Read the group descriptors */
    off_t gd_offset = disk->blocksize == 1024 ? 2048 : disk->blocksize;
    disk->gd_size = hd->groups * sizeof(struct ext2_group_desc);

    disk->gd = kmap(ALIGN_UP(gd_size, PAGE_SIZE), dev, gd_offset, VMA_FG_RW_FILE);
    if (disk->gd == NULL) {
        kunmap(disk->sb, sb_size);
        kfree(disk);
        vfs_close(dev);
        errno = ENOMEM;
        return NULL;
    }

    // TODO check
    if (false) {
        kunmap(disk->sb, sb_size);
        kunmap(disk->gd, gd_size);
        kfree(disk);
        vfs_close(dev);
        errno = EBADF;
        return NULL;
    }

    disk->name[16] = '\0';
    strncpy(disk->name, disk->sb->s_volume_name, 16);

    EXT_inode_t *ino = vfs_mountpt(0, disk->name, "Ext2", sizeof(EXT_inode_t));
    // ino-> - TODO - Read root !

    errno = 0;
    return ino;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int EXT_setup()
{
    vfs_fs_register("ext-fs", EXT_fs_ops);
    errno = 0;
    return 0;
}

int EXT_teardown()
{
    vfs_fs_unregister("ext-fs");
    errno = 0;
    return 0;
}

vfs_fs_ops_t EXT_fs_ops = {
    .mount = (fs_mount)EXT_mount,
};

