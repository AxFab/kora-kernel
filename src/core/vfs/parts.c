#include <kernel/mods/fs.h>
#include <string.h>
#include <errno.h>

extern device_t *vfs_lookup_device_(CSTR name);

PACK(struct MBR_parts {
    char media;
    char start_chs[3];
    char fs;
    char end_chs[3];
    unsigned int start;
    unsigned int length;
});

PACK(struct MBR_sector {
    int jmp;
    char code[438];
    unsigned int uid;
    struct MBR_parts parts[4];
    unsigned short sign;
});

typedef struct MBR_inode {
    inode_t ino;
    device_t *underlying;
} MBR_inode_t;

int vfs_mbr_read(MBR_inode_t *ino, char *buf, size_t len, off_t off)
{
    if (ino->ino.length < off + (signed)len) {
        errno = EINVAL;
        return -1;
    }
    return vfs_read(ino->underlying->ino, buf, len, off + ino->ino.lba * 512);
}

int vfs_mbr_write(MBR_inode_t *ino, const char *buf, size_t len, off_t off)
{
    if (ino->ino.length < off + (signed)len) {
        errno = EINVAL;
        return -1;
    } else if (ino->ino.dev->read_only) {
        errno = EROFS;
        return -1;
    }
    return vfs_write(ino->underlying->ino, buf, len, off + ino->ino.lba * 512);
}

int vfs_fdisk(CSTR dname, long parts, long *sz)
{
    int i;
    device_t *dev = vfs_lookup_device_(dname);
    if (dev == NULL) {
        errno = ENODEV;
        return -1;
    }

    struct MBR_sector *sector = (struct MBR_sector *)kmap(PAGE_SIZE, dev->ino, 0, VMA_FILE_RW);
    sector->jmp = 0x109064ef;
    sector->sign = 0xAA55;
    sector->uid = 0x12345678;
    memset(sector->parts, 0, 16 * 4);
    for (i = 0; i < parts; ++i) {
        sector->parts[i].media = 0;
        sector->parts[i].fs = sz[i * 3 + 2];
        sector->parts[i].start = sz[i * 3];
        if (sz[i * 3 + 1] == -1)
            sector->parts[i].length = (dev->ino->length / 512) - sz[i * 3];
        else
            sector->parts[i].length = sz[i * 3 + 1] - sz[i * 3];

    }

    vfs_write(dev->ino, sector, PAGE_SIZE, 0);
    kunmap(sector, PAGE_SIZE);
    errno = 0;
    return 0;
}

int vfs_parts(CSTR dname)
{
    int i;
    char buf[20];
    device_t *dev = vfs_lookup_device_(dname);
    if (dev == NULL) {
        errno = ENODEV;
        return -1;
    }

    struct MBR_sector *sector = (struct MBR_sector *)kmap(PAGE_SIZE, dev->ino, 0, VMA_FILE_RO);
    if ((sector->jmp & 0xFFFFFF) != 0x009064ef || sector->sign != 0xAA55) {
        errno = EINVAL;
        return -1;
    }

    for (i = 0; i < 4; ++i) {
        if (sector->parts[i].start != 0 && sector->parts[i].length != 0) {
            sprintf(buf, "%s%d", dname, i);
            MBR_inode_t *ino = (MBR_inode_t*)vfs_inode(i, S_IFBLK | 0640, NULL, sizeof(MBR_inode_t));
            ino->ino.lba = sector->parts[i].start;
            ino->ino.length = sector->parts[i].length * 512;
            ino->underlying = dev;
            device_t *sdev = (device_t*)kalloc(sizeof(device_t));
            sdev->read_only = dev->read_only;
            sdev->block = dev->block;
            sdev->vendor = "MBR";
            sdev->class = "Logical partition";
            sdev->read = (fs_read)vfs_mbr_read;
            sdev->write = (fs_write)vfs_mbr_write;
            vfs_mkdev(buf, sdev, &ino->ino);
        }
    }

    kunmap(sector, PAGE_SIZE);
    errno = 0;
    return 0;
}


void FATFS_setup() {}
void FATFS_teardown() {}


