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


PACK(struct ext2_super_block {
    u32 s_inodes_count;     /* Total number of inodes */
    u32 s_blocks_count;     /* Total number of blocks */
    u32 s_r_blocks_count;   /* Total number of blocks reserved for the super user */
    u32 s_free_blocks_count;        /* Total number of free blocks */
    u32 s_free_inodes_count;        /* Total number of free inodes */
    u32 s_first_data_block; /* Id of the block containing the superblock structure */
    u32 s_log_block_size;   /* Used to compute block size = 1024 << s_log_block_size */
    u32 s_log_frag_size;    /* Used to compute fragment size */
    u32 s_blocks_per_group; /* Total number of blocks per group */
    u32 s_frags_per_group;  /* Total number of fragments per group */
    u32 s_inodes_per_group; /* Total number of inodes per group */
    u32 s_mtime;            /* Last time the file system was mounted */
    u32 s_wtime;            /* Last write access to the file system */
    u16 s_mnt_count;        /* How many `mount' since the last was full verification */
    u16 s_max_mnt_count;    /* Max count between mount */
    u16 s_magic;            /* = 0xEF53 */
    u16 s_state;            /* File system state */
    u16 s_errors;           /* Behaviour when detecting errors */
    u16 s_minor_rev_level;  /* Minor revision level */
    u32 s_lastcheck;        /* Last check */
    u32 s_checkinterval;    /* Max. time between checks */
    u32 s_creator_os;       /* = 5 */
    u32 s_rev_level;        /* = 1, Revision level */
    u16 s_def_resuid;       /* Default uid for reserved blocks */
    u16 s_def_resgid;       /* Default gid for reserved blocks */
    u32 s_first_ino;        /* First inode useable for standard files */
    u16 s_inode_size;       /* Inode size */
    u16 s_block_group_nr;   /* Block group hosting this superblock structure */
    u32 s_feature_compat;
    u32 s_feature_incompat;
    u32 s_feature_ro_compat;
    u8 s_uuid[16];          /* Volume id */
    char s_volume_name[16]; /* Volume name */
    char s_last_mounted[64];        /* Path where the file system was last mounted */
    u32 s_algo_bitmap;      /* For compression */
    u8 s_padding[820];
};

PACK(struct EXT_disk {
    struct ext2_super_block *sb;
    struct ext2_group_desc *gd;
    size_t sb_size;
    size_t gd_size
    u32 blocksize;
    u16 groups;             /* Total number of groups */
    char s_volume_name[18]; /* Volume name */
});

PACK(struct ext2_group_desc {
    u32 bg_block_bitmap;    /* Id of the first block of the "block bitmap" */
    u32 bg_inode_bitmap;    /* Id of the first block of the "inode bitmap" */
    u32 bg_inode_table;     /* Id of the first block of the "inode table" */
    u16 bg_free_blocks_count;       /* Total number of free blocks */
    u16 bg_free_inodes_count;       /* Total number of free inodes */
    u16 bg_used_dirs_count; /* Number of inodes allocated to directories */
    u16 bg_pad;             /* Padding the structure on a 32bit boundary */
    u32 bg_reserved[3];     /* Future implementation */
});

PACK(struct ext2_inode {
    u16 i_mode;             /* File type + access rights */
    u16 i_uid;
    u32 i_size;
    u32 i_atime;
    u32 i_ctime;
    u32 i_mtime;
    u32 i_dtime;
    u16 i_gid;
    u16 i_links_count;
    u32 i_blocks;           /* 512 bytes blocks ! */
    u32 i_flags;
    u32 i_osd1;

    /*
     * [0] -> [11] : block number (32 bits per block)
     * [12]        : indirect block number
     * [13]        : bi-indirect block number
     * [14]        : tri-indirect block number
     */
    u32 i_block[15];

    u32 i_generation;
    u32 i_file_acl;
    u32 i_dir_acl;
    u32 i_faddr;
    u8 i_osd2[12];
});

PACK(struct directory_entry {
    u32 inode;              /* inode number or 0 (unused) */
    u16 rec_len;            /* offset to the next dir. entry */
    u8 name_len;            /* name length */
    u8 file_type;
    char name;
});


/* super_block: s_errors */
#define EXT2_ERRORS_CONTINUE    1
#define EXT2_ERRORS_RO          2
#define EXT2_ERRORS_PANIC       3
#define EXT2_ERRORS_DEFAULT     1

/* inode: i_mode */
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

