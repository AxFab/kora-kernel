#include <kernel/core.h>
#include <kernel/sys/inode.h>
#include <kernel/sys/vma.h>
#include <kernel/vfs.h>
#include <stdio.h>
#include <errno.h>

PACK(struct MBR_partition {
    uint8_t media;
    uint8_t start_chs[3];
    uint8_t type;
    uint8_t end_chs[3];
    uint32_t start_lba;
    uint32_t end_lba;
});

PACK(struct MBR_record {
    uint8_t init[440];
    uint32_t sign;
    uint16_t unused;
    struct MBR_partition parts[4];
    uint16_t eor;
});

typedef struct MBR_inode MBR_inode_t;

struct MBR_inode {
    inode_t ino;
    inode_t *dev;
    uint64_t lba;
    uint64_t sects;
};

extern vfs_io_ops_t MBR_io_ops;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void MBR_format(inode_t *ino)
{
    struct MBR_record *rcd = (struct MBR_record *)kalloc(512);
    rcd->sign = 0x61726F4B;
    rcd->eor = 0xAA55;

    rcd->parts[0].type = 6;
    rcd->parts[0].start_lba = 16;
    rcd->parts[0].end_lba = ino->length / 512;

    vfs_write(ino, rcd, 512, 0);
}



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int MBR_read(MBR_inode_t *ino, void *data, size_t size, off_t offset)
{
    if ((off_t)size + offset > ino->ino.length) {
        errno = EINVAL;
        return -1;
    }
    return vfs_read(ino->dev, data, ino->ino.lba + size, offset);
}

int MBR_write(MBR_inode_t *ino, const void *data, size_t size, off_t offset)
{
    if ((off_t)size + offset > ino->ino.length) {
        errno = EINVAL;
        return -1;
    }
    return vfs_write(ino->dev, data, ino->ino.lba + size, offset);
}

int MBR_mount(inode_t *dev, const char *devname)
{
    int i;
    char name[8];
    if (dev == NULL) {
        errno = ENODEV;
        return -1;
    }

    struct MBR_record *rcd = (struct MBR_record *)kmap(PAGE_SIZE, dev, 0,
                             VMA_FG_RO_FILE);
    if (rcd-> eor != 0xAA55) {
        kunmap(rcd, PAGE_SIZE);
        errno = EBADF;
        return -1;
    }

    for (i = 0; i < 4; ++i) {
        if (rcd->parts[i].start_lba == 0 ||
                rcd->parts[i].end_lba <= rcd->parts[i].start_lba) {
            continue;
        }

        uint32_t sz = rcd->parts[i].end_lba - rcd->parts[i].start_lba;
        snprintf(name, 8, "%s%d", devname, i + 1);
        MBR_inode_t *ino = (MBR_inode_t *)vfs_inode(i + 1, S_IFBLK,
                           sizeof(MBR_inode_t));
        ino->ino.length = sz * dev->block;
        ino->ino.lba = rcd->parts[i].start_lba * dev->block;
        ino->ino.block = dev->block;
        ino->ino.io_ops = &MBR_io_ops;
        ino->dev = dev;

        vfs_mkdev(name, (inode_t *)ino, NULL, "MBR Partition", NULL, NULL);
        vfs_close((inode_t *)ino);
    }

    kunmap(rcd, PAGE_SIZE);
    return 0;
}

int MBR_unmount(inode_t *ino)
{
    errno = ENOSYS;
    return -1;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int MBR_setup()
{
    errno = 0;
    return 0;
}

int MBR_teardown()
{
    errno = 0;
    return 0;
}


vfs_io_ops_t MBR_io_ops = {
    .read = (fs_read)MBR_read,
    .write = (fs_write)MBR_write,
};

