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
#include <kora/mcrs.h>
#include <kernel/vfs.h>

// https://wiki.osdev.org/Ext2#Directory_Entry_Type_Indicators
// http://web.mit.edu/tytso/www/linux/ext2intro.html
// https://www.nongnu.org/ext2-doc/ext2.html
// https://michelizza.developpez.com/realiser-son-propre-systeme/#LXIX-B


#define EXT2_SUPER_MAGIC 0xEF53
#define EXT2_DYNAMIC_REV 1
#define EXT2_MAX_BLOCK_SIZE 4096
#define EXT2_GOOD_OLD_FIRST_INO 11 // first non-reserved inode for legacy ext2
#define EXT2_NDIR_BLOCKS 12



typedef struct ext2_sb ext2_sb_t;
typedef struct ext2_grp ext2_grp_t;
typedef struct ext2_ino ext2_ino_t;

PACK(struct ext2_sb {
    uint32_t inodes_count;     /* Total number of inodes */
    uint32_t blocks_count;     /* Total number of blocks */
    uint32_t rsvd_blocks_count;   /* Total number of blocks reserved for the super user */
    uint32_t free_blocks_count;   /* Total number of free blocks */
    uint32_t free_inodes_count;   /* 10 - Total number of free inodes */
    uint32_t first_data_block; /* Id of the block containing the superblock structure */
    uint32_t log_block_size;   /* Used to compute block size = 1024 << log_block_size */
    uint32_t log_frag_size;    /* Used to compute fragment size */
    uint32_t blocks_per_group; /* 20 - Total number of blocks per group */
    uint32_t frags_per_group;  /* Total number of fragments per group */
    uint32_t inodes_per_group; /* Total number of inodes per group */
    uint32_t mtime;            /* Last time the file system was mounted */
    uint32_t wtime;            /* 30 - Last write access to the file system */
    uint16_t mnt_count;        /* How many `mount' since the last was full verification */
    uint16_t max_mnt_count;    /* Max count between mount */
    uint16_t magic;            /* = 0xEF53 */
    uint16_t state;            /* File system state */
    uint16_t errors;           /* Behaviour when detecting errors */
    uint16_t minor_rev_level;  /* Minor revision level */
    uint32_t lastcheck;        /* 40 - Last check */
    uint32_t checkinterval;    /* Max. time between checks */
    uint32_t creator_os;       /* = 5 */
    uint32_t rev_level;        /* = 1, Revision level */
    uint16_t def_resuid;       /* 50 - Default uid for reserved blocks */
    uint16_t def_resgid;       /* Default gid for reserved blocks */
    uint32_t first_ino;        /* First inode useable for standard files */
    uint16_t inode_size;       /* Inode size */
    uint16_t block_group_nr;   /* Block group hosting this superblock structure */
    uint32_t feature_compat;
    uint32_t feature_incompat; /* 60 - */
    uint32_t feature_ro_compat;
    uint8_t uuid[16];          /* Volume id */
    char volume_name[16];      /* Volume name */
    char last_mounted[64];     /* Path where the file system was last mounted */
    uint32_t algo_bitmap;      /* For compression */
    uint8_t prealloc_block_file;
    uint8_t prealloc_block_dir;
    uint16_t unused;
    uint8_t journal_id[16];
    uint32_t journal_inode;
    uint32_t journal_device;
    uint32_t head_orphan_inode;
    uint8_t padding[788];
});

PACK(struct ext2_disk {
    struct ext2_sb *sb;
    struct ext2_grp *gd;
    size_t sb_size;
    size_t gd_size;
    uint32_t blocksize;
    uint16_t groups;           /* Total number of groups */
    char volume_name[18];      /* Volume name */
});

PACK(struct ext2_grp {
    uint32_t block_bitmap;    /* Id of the first block of the "block bitmap" */
    uint32_t inode_bitmap;    /* Id of the first block of the "inode bitmap" */
    uint32_t inode_table;     /* Id of the first block of the "inode table" */
    uint16_t free_blocks_count;  /* Total number of free blocks */
    uint16_t free_inodes_count;  /* Total number of free inodes */
    uint16_t used_dirs_count; /* Number of inodes allocated to directories */
    uint16_t pad;             /* Padding the structure on a 32bit boundary */
    uint32_t reserved[3];     /* Future implementation */
});

PACK(struct ext2_ino {
    uint16_t mode;             /* File type + access rights */
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links;
    uint32_t blocks;           /* 512 bytes blocks ! */
    uint32_t flags;
    uint32_t osd1;

    /*
     * [0] -> [11] : block number (32 bits per block)
     * [12]        : indirect block number
     * [13]        : bi-indirect block number
     * [14]        : tri-indirect block number
     */
    uint32_t block[15];

    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t faddr;
    uint8_t osd2[12];
});

//PACK(struct directory_entry {
//    uint32_t inode;              /* inode number or 0 (unused) */
//    uint16_t rec_len;            /* offset to the next dir. entry */
//    uint8_t name_len;            /* name length */
//    uint8_t file_type;
//    char name;
//});

typedef struct ext2_volume ext2_volume_t;
typedef struct ext2_dir_iter ext2_dir_iter_t;
typedef struct ext2_dir_en ext2_dir_en_t;


struct ext2_volume {
    size_t blocksize;
    ext2_sb_t *sb;
    ext2_grp_t *grp;
    inode_t *blkdev;
    device_t *dev;
    unsigned groupCount;
    size_t groupSize;
    atomic_int rcu;
};

typedef struct
{
    xoff_t off;
    int lba;
    unsigned blocks;
    struct bkmap km;
    ext2_dir_en_t *previous;
    ext2_dir_en_t *entry;
    uint8_t *ptr;
    ext2_ino_t *dir;
    bool create;
    int flags;
} ext2_diterator_t;


struct ext2_dir_iter {
    ext2_diterator_t it;
    struct bkmap bk;
    ext2_ino_t *entry;

    //ext2_volume_t *vol;
    //// ext2_ino_t *entry;
    //// uint32_t blk;
    //int idx;
    //// int last;
    //// uint8_t *cur_block;

    ////void *emap;
    //struct bkmap bki;
    //ext2_ino_t *entry;

    //// size_t off;
    //size_t lba;
    //struct bkmap bkm;
    //void *cmap;
};


struct ext2_dir_en {
    uint32_t ino;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t type;
    char name[0];
};


typedef struct ext2_dir_hack ext2_dir_hack_t;

struct ext2_dir_hack {
    uint32_t ino1;
    uint16_t rec_len1;
    uint8_t name_len1;
    uint8_t file_type1;
    char name1[4];
    uint32_t ino2;
    uint16_t rec_len2;
    uint8_t name_len2;
    uint8_t file_type2;
    char name2[4];
    uint32_t ino3;
    uint16_t rec_len3;
    uint8_t name_len3;
    uint8_t file_type3;
    char name3[12];
};


#define EXT2_BAD_INO 1
#define EXT2_ROOT_INO 2
#define EXT2_BOOT_LOADER_INO 5
#define EXT2_UNDEL_DIR_INO 6


/* super_block: errors */
#define EXT2_ERRORS_CONTINUE    1
#define EXT2_ERRORS_RO          2
#define EXT2_ERRORS_PANIC       3
#define EXT2_ERRORS_DEFAULT     1

/* inode: mode */
#define EXT2_S_IFMT     0xF000  /* format mask  */
#define EXT2_S_IFSOCK   0xC000  /* socket */
#define EXT2_S_IFLNK    0xA000  /* symbolic link */
#define EXT2_S_IFREG    0x8000  /* regular file */
#define EXT2_S_IFBLK    0x6000  /* block device */
#define EXT2_S_IFDIR    0x4000  /* directory */
#define EXT2_S_IFCHR    0x2000  /* character device */
#define EXT2_S_IFIFO    0x1000  /* fifo */

#define EXT2_S_ISUID    0x0800  /* SUID */
#define EXT2_S_ISGID    0x0400  /* SGID */
#define EXT2_S_ISVTX    0x0200  /* sticky bit */
#define EXT2_S_IRWXU    0x01C0  /* user access rights mask */
#define EXT2_S_IRUSR    0x0100  /* read */
#define EXT2_S_IWUSR    0x0080  /* write */
#define EXT2_S_IXUSR    0x0040  /* execute */
#define EXT2_S_IRWXG    0x0038  /* group access rights mask */
#define EXT2_S_IRGRP    0x0020  /* read */
#define EXT2_S_IWGRP    0x0010  /* write */
#define EXT2_S_IXGRP    0x0008  /* execute */
#define EXT2_S_IRWXO    0x0007  /* others access rights mask */
#define EXT2_S_IROTH    0x0004  /* read */
#define EXT2_S_IWOTH    0x0002  /* write */
#define EXT2_S_IXOTH    0x0001  /* execute */


#define EXT2_FT_DIR  2   

int ext2_format(inode_t* dev, const char* options);


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// File operations
/* Copy into the buffer data from mapped area of the underlying block device */
int ext2_read(inode_t* ino, char* buffer, size_t length, xoff_t offset, int flags);
/* Copy the buffer data into mapped area of the underlying block device */
int ext2_write(inode_t* ino, const char* buffer, size_t length, xoff_t offset, int flags);
/* Change the size of a file inode and allocate or release blocks */
int ext2_truncate(inode_t* ino, xoff_t offset);


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Directory operations
/* Open a directory context for browsing files */
ext2_dir_iter_t *ext2_opendir(inode_t *dir);
/* Find the next inode on a directory context */
uint32_t ext2_nextdir(inode_t *dir, char *name, ext2_dir_iter_t *it);
/* Look for the next entry on a directoy context */
inode_t *ext2_readdir(inode_t *dir, char *name, ext2_dir_iter_t *it);
/* Close a directory context */
int ext2_closedir(inode_t *dir, ext2_dir_iter_t *it);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Symlink operations



// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


void ext2_iterator_open(ext2_volume_t *vol, ext2_ino_t *dir, ext2_diterator_t *iter, bool create, int flags);
void ext2_iterator_close(ext2_volume_t *vol, ext2_diterator_t *iter);
int ext2_iterator_next(ext2_volume_t *vol, ext2_diterator_t *iter);
ext2_dir_en_t *ext_iterator_find(ext2_volume_t *vol, ext2_diterator_t *iter, const char *name);

uint32_t ext2_alloc_block(ext2_volume_t *vol);
int ext2_release_block(ext2_volume_t *vol, uint32_t block);
uint32_t ext2_get_block(ext2_volume_t *vol, ext2_ino_t *dir, uint32_t blk, bool alloc);
void ext2_truncate_direct(ext2_volume_t *vol, uint32_t *table, int len, int bcount, int offset);
uint32_t ext2_truncate_indirect(ext2_volume_t *vol, uint32_t block, int bcount, int offset, int depth);

int ext2_free_blocks(ext2_volume_t *vol, uint32_t *blocks);


inode_t *ext2_inode(ext2_volume_t *vol, uint32_t no);


ext2_ino_t* ext2_entry(struct bkmap* bk, ext2_volume_t* vol, unsigned no, int flg);


int ext2_dir_is_empty(ext2_volume_t *vol, ext2_ino_t *dir);



int ext2_chmod(inode_t *ino, int mode);
