#include <kernel/vfs.h>
#include <kernel/futex.h>
#include <kernel/device.h>
#include <errno.h>

typedef struct dfs_table dfs_table_t;
typedef struct dfs_info dfs_info_t;

struct dfs_info
{
    int ino;
    int flags;
    char uuid[16];
    char name[32];
    char label[32];
    int show;
    int filter;
    inode_t *dev;
};

struct dfs_table
{
    dfs_table_t *next;
    int length;
    int free;
    dfs_info_t entries[0];
};

enum {
    DF_ROOT = (1 << 0),
    DF_BUS = (1 << 1),
    DF_DISK = (1 << 2),
    DF_INPUT = (1 << 3),
    DF_MOUNT = (1 << 4),
    DF_NET = (1 << 5),
    DF_SHM = (1 << 6),
    DF_DISK1 = (1 << 7),
    DF_DISK2 = (1 << 8),
    DF_MOUNT1 = (1 << 9),
    DF_MOUNT2 = (1 << 10),
    DF_CSTM1 = (1 << 11),
    DF_CSTM2 = (1 << 12),
    DF_CSTM3 = (1 << 13),
    DF_CSTM4 = (1 << 14),
    DF_CSTM5 = (1 << 15),
    DF_CSTM6 = (1 << 16),
    DF_CSTM7 = (1 << 17),
    DF_CSTM8 = (1 << 18),
};


static dfs_table_t *devfs_extends(int first)
{
    int i;
    dfs_table_t *table;
    table = kmap(PAGE_SIZE, NULL, 0, VMA_ANON_RW);
    memset(table, 0, PAGE_SIZE);
    int sz = (PAGE_SIZE - sizeof(dfs_table_t)) / sizeof(dfs_info_t);
    table->length = sz;
    table->free = sz;
    for (i = 0; i < table->length; ++i)
        table->entries[i].ino = first + i;
    return table;
}


static dfs_info_t *devfs_fetch_new()
{
    int idx;
    dfs_table_t *table = kSYS.dev_table;
    for (idx = 0; ; ++idx) {
    	if (table->free == 0 || idx >= table->length) {
    	    if (table->next == NULL)
    		table->next = devfs_extends(table->entries[0].ino + table->length);
    	    table = table->next;
    	    idx = 0;
    	}
    	if (table->entries[idx].flags == 0) {
    	    table->entries[idx].flags = 1;
    	    return &table->entries[idx];
    	}
    }
}

static dfs_info_t *devfs_fetch(int no)
{
    dfs_table_t *table = kSYS.dev_table;
    while (no > table->length) {
        no -= table->length;
        table = table->next;
    }
    return &table->entries[no - 1];
}

static int devfs_dir(const char *name, int prt, int filter, int show)
{
    dfs_info_t *info = devfs_fetch_new();
    // info->prt = prt;
    (void)prt;
    info->show = show;
    info->filter = filter;
    strncpy(info->name, name, 32);
    info->flags = 2;
    return info->ino;
}

static inode_t *devfs_dev(const char *name, int type, int show, ino_ops_t *ops)
{
    dfs_info_t *info = devfs_fetch_new();
    inode_t *ino = vfs_inode(info->ino, type, NULL);
    ino->ops = ops;
    // info->dev->devname = strdup(name);
    ino->dev->devclass = strdup("Bytes device");
    info->show = show;
    info->dev = ino;
    strncpy(info->name, name, 32);
    info->flags = 2;
    kprintf(KLOG_MSG, "%s %s <\033[33m%s\033[0m>\n", ino->dev->devclass,
        ino->dev->model ? ino->dev->model : "", name);

    return ino;
}

extern ino_ops_t devfs_dir_ops;

static inode_t *devfs_inode(dfs_info_t *info)
{
    if (info->dev)
        return vfs_open(info->dev);
    inode_t *ino = vfs_inode(info->ino, FL_DIR, kSYS.dev_ino->dev);
    ino->ops = &devfs_dir_ops;
    // TODO -- fill !
    return ino;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int null_read(inode_t *ino, char *buf, size_t len, int flags)
{
    int s = 0;
    while (flags & VFS_BLOCK)
        futex_wait(&s, 0, -1, 0);
    errno = EWOULDBLOCK;
    return 0;
}

int null_write(inode_t *ino, const char *buf, size_t len, int flags)
{
    errno = 0;
    return len;
}

int zero_read(inode_t *ino, char *buf, size_t len, int flags)
{
    memset(buf, 0, len);
    errno = 0;
    return len;
}

int rand_read(inode_t *ino, char *buf, size_t len, int flags)
{
    size_t i;
    for (i = 0; i < len; ++i)
        buf[i] = rand8();
    errno = 0;
    return len;
}


inode_t *devfs_open(inode_t *dir, const char *name, int mode, acl_t *acl, int flags)
{
    // TODO - If create exclusive, return EROFS
    dfs_info_t *info = devfs_fetch(dir->no);
    if (info == NULL || info->flags != 2) {
        errno = ENOENT;
        return NULL;
    }

    // Look on name / label or uuid ?
    int idx;
    dfs_table_t *table = kSYS.dev_table;
    for (idx = 0; ; ++idx) {
        if (idx >= table->length) {
            idx = 0;
            table = table->next;
            if (table == NULL) {
                errno = ENOENT;
                return NULL;
            }
        }

        dfs_info_t *en = &table->entries[idx];
        if ((en->show & info->filter) == 0)
            continue;

        int k = strcmp(en->name, name);
        if (k == 0) {
            return devfs_inode(en);
        }
    }
    return NULL;
}

dev_ops_t dev_ops = {
    .ioctl = NULL,
};

fs_ops_t devfs_fs_ops = {
    .open = devfs_open,
};

ino_ops_t devfs_dir_ops = {

};

ino_ops_t devfs_zero_ops = {
    .read = zero_read,
};

ino_ops_t devfs_null_ops = {
    .read = null_read,
    .write = null_write,
};

ino_ops_t devfs_rand_ops = {
    .read = rand_read,
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void devfs_register(inode_t *ino, inode_t *dir, const char *name)
{
    dfs_info_t *info = devfs_fetch_new();
    switch (ino->type) {
    case FL_DIR:
        info->show = DF_MOUNT;
        if (ino->dev->id[0] != '0')
            info->show |= DF_MOUNT1;
        if (ino->dev->devname[0] != '0')
            info->show |= DF_MOUNT2;
        // kprintf(KLOG_MSG, "Mount %s as \033[35m%s\033[0m (%s)\n", devname, ino->dev->devname, ino->dev->devclass);
        kprintf(KLOG_MSG, "Mount drive as \033[35m%s\033[0m (%s)\n", ino->dev->devname, ino->dev->devclass);
        break;
    case FL_BLK:
        info->show = DF_DISK;
        if (ino->dev->id[0] != '0')
            info->show = DF_DISK1;
        if (ino->dev->devname[0] != '0')
            info->show = DF_DISK2;
        if (ino->length)
            kprintf(KLOG_MSG, "%s %s %s <\033[33m%s\033[0m>\n", ino->dev->devclass,
                    ino->dev->model ? ino->dev->model : "", sztoa(ino->length), name);
        else
        kprintf(KLOG_MSG, "%s %s <\033[33m%s\033[0m>\n", ino->dev->devclass,
                ino->dev->model ? ino->dev->model : "", name);
        break;
    default:
        info->show = DF_ROOT;
        kprintf(KLOG_MSG, "%s %s <\033[33m%s\033[0m>\n", ino->dev->devclass,
                ino->dev->model ? ino->dev->model : "", name);
        break;
    }

    info->dev = vfs_open(ino);
    strncpy(info->name, name, 32);
    info->flags = 2;
}

void devfs_mount()
{
    kSYS.dev_table = devfs_extends(1);
    dfs_info_t *info = devfs_fetch_new();
    assert(info->ino == 1);
    kSYS.dev_ino = vfs_inode(1, FL_DIR, NULL);
    kSYS.dev_ino->ops = &devfs_dir_ops;
    kSYS.dev_ino->dev->fsops = &devfs_fs_ops;
    info->ino = kSYS.dev_ino;
    info->filter = DF_ROOT;
    info->flags = 2;

    int bus = devfs_dir("bus", 1, DF_BUS, DF_ROOT);
    int dsk = devfs_dir("disk", 1, DF_DISK, DF_ROOT);
    devfs_dir("input", 1, DF_INPUT, DF_ROOT);
    int mnt = devfs_dir("mnt", 1, DF_MOUNT, DF_ROOT);
    devfs_dir("net", 1, DF_NET, DF_ROOT);
    devfs_dir("shm", 1, DF_SHM, DF_ROOT);

    devfs_dir("by-uuid", dsk, DF_DISK1, DF_DISK);
    devfs_dir("by-label", dsk, DF_DISK2, DF_DISK);

    devfs_dir("by-uuid", mnt, DF_MOUNT1, DF_MOUNT);
    devfs_dir("by-label", mnt, DF_MOUNT2, DF_MOUNT);

    devfs_dir("pci", bus, DF_CSTM1, DF_BUS);
    devfs_dir("usb", bus, DF_CSTM2, DF_BUS);

    devfs_dev("zero", FL_CHR, DF_ROOT, &devfs_zero_ops);
    devfs_dev("null", FL_CHR, DF_ROOT, &devfs_null_ops);
    devfs_dev("rand", FL_CHR, DF_ROOT, &devfs_rand_ops);
}


void devfs_sweep()
{
    int i;
    dfs_table_t *table = kSYS.dev_table;
    while (table) {
        for (i = 0; i < table->length; ++i) {
            dfs_info_t *info = &table->entries[i];
            if (info->flags == 0)
                continue;
            vfs_close(info->dev);
            kfree(info->name);
        }

        dfs_table_t *p = table;
        table = table->next;
        kunmap(p, PAGE_SIZE);
    }
}

