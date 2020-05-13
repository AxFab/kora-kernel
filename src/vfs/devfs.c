#include <kernel/vfs.h>
#include <kernel/futex.h>
#include <kernel/device.h>
#include <errno.h>

typedef struct dfs_table dfs_table_t;
typedef struct dfs_info dfs_info_t;

void sleep_timer(long);

struct dfs_info {
    int ino;
    int flags;
    char uuid[16];
    char name[32];
    char label[32];
    int show;
    int filter;
    inode_t *dev;
};

struct dfs_table {
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
        return vfs_open(info->dev, X_OK);
    inode_t *ino = vfs_inode(info->ino, FL_DIR, kSYS.dev_ino->dev);
    ino->ops = &devfs_dir_ops;
    // TODO -- fill !
    return ino;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int null_read(inode_t *ino, char *buf, size_t len, int flags)
{
    int s = 0;
    if (flags & VFS_NOBLOCK) {
        errno = EWOULDBLOCK;
        return 0;
    }
    for (;;)
        sleep_timer(MIN_TO_USEC(15));
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

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


inode_t *devfs_open(inode_t *dir, const char *name, int mode, acl_t *acl, int flags)
{
    if ((flags & (VFS_OPEN | VFS_CREAT)) == VFS_CREAT) {
        errno = EROFS;
        return NULL;
    }
    dfs_info_t *info = devfs_fetch(dir->no);
    assert(info != NULL && info->flags == 2);

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
        if (k == 0)
            return devfs_inode(en);
    }
    return NULL;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


typedef struct devfs_dir_it {
    dfs_info_t *info;
    int idx;
} devfs_dir_it_t;


devfs_dir_it_t *devfs_opendir(inode_t *dir, acl_t *acl)
{
    dfs_info_t *info = devfs_fetch(dir->no);
    assert(info != NULL && info->flags == 2);

    devfs_dir_it_t *it = kalloc(sizeof(devfs_dir_it_t));
    it->info = info;
    it->idx = 0;
    return it;
}

inode_t *devfs_readdir(inode_t *dir, char *name, devfs_dir_it_t *ctx)
{
    (void)dir;
    dfs_table_t *table = kSYS.dev_table;
    int no = ctx->idx;
    while (no > table->length) {
        no -= table->length;
        if (table->next == NULL)
            return NULL;
        table = table->next;
    }

    for (;; ++no) {
        if (no > table->length) {
            table = table->next;
            if (table == NULL)
                return NULL;
            no = 0;
        }
        ctx->idx++;

        dfs_info_t *en = &table->entries[no];
        if (en->flags != 2)
            continue;
        if ((en->show & ctx->info->filter) == 0)
            continue;

        strcpy(name, en->name);
        return devfs_inode(en);
    }
}

void devfs_closedir(inode_t *dir, devfs_dir_it_t *ctx)
{
    (void)dir;
    kfree(ctx);
}

dev_ops_t dev_ops = {
    .ioctl = NULL,
};

fs_ops_t devfs_fs_ops = {
    .open = (void *)devfs_open,
};

ino_ops_t devfs_dir_ops = {
    .opendir = (void *)devfs_opendir,
    .readdir = (void *)devfs_readdir,
    .closedir = (void *)devfs_closedir,
};

ino_ops_t devfs_zero_ops = {
    .read = (void *)zero_read,
};

ino_ops_t devfs_null_ops = {
    .read = (void *)null_read,
    .write = (void *)null_write,
};

ino_ops_t devfs_rand_ops = {
    .read = (void *)rand_read,
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void devfs_register(inode_t *ino, inode_t *dir, const char *name)
{
    char tmp[12];
    dfs_info_t *info = devfs_fetch_new();
    switch (ino->type) {
    case FL_DIR:
    case FL_VOL:
        info->show = DF_MOUNT;
        if (ino->dev->id[0] != '0')
            info->show |= DF_MOUNT1;
        if (ino->dev->devname != NULL)
            info->show |= DF_MOUNT2;
        // kprintf(KLOG_MSG, "Mount %s as \033[35m%s\033[0m (%s)\n", devname, ino->dev->devname, ino->dev->devclass);
        kprintf(KLOG_MSG, "Mount drive as \033[35m%s\033[0m (%s)\n", ino->dev->devname, ino->dev->devclass);
        break;
    case FL_BLK:
        info->show = DF_DISK | DF_ROOT;
        if (ino->dev->id[0] != '0')
            info->show |= DF_DISK1;
        if (ino->dev->devname != NULL)
            info->show |= DF_DISK2;
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

    kprintf(-1, "Device %s\n", vfs_inokey(ino, tmp));
    info->dev = vfs_open(ino, X_OK);
    strncpy(info->name, name, 32);
    info->flags = 2;
}

void vfs_init()
{
    kSYS.dev_table = devfs_extends(1);
    dfs_info_t *info = devfs_fetch_new();
    assert(info->ino == 1);
    kSYS.dev_ino = vfs_inode(1, FL_DIR, NULL);
    kSYS.dev_ino->ops = &devfs_dir_ops;
    kSYS.dev_ino->dev->fsops = &devfs_fs_ops;
    info->dev = kSYS.dev_ino;
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

    // TODO -- FS drivers list
}


void vfs_sweep()
{
    int i;
    dfs_table_t *table = kSYS.dev_table;
    while (table) {
        for (i = 0; i < table->length; ++i) {
            dfs_info_t *info = &table->entries[i];
            if (info->flags == 0)
                continue;
            vfs_close(info->dev, X_OK);
        }

        dfs_table_t *p = table;
        table = table->next;
        kunmap(p, PAGE_SIZE);
    }
    kSYS.dev_table = NULL;
    // Clean inode cache
}

int vfs_mkdev(inode_t *ino, CSTR name)
{
    devfs_register(ino, NULL, name);
    return 0;
}

void vfs_rmdev(CSTR name)
{
}

unsigned entrop_k = 0;
unsigned entrop_arr[2];
unsigned entrop_T0 = 0; // time() % 4219;

void vfs_entropy(unsigned value)
{
    unsigned v = (entrop_k++) & 1;
    entrop_arr[v] = value;
    if (v) {
        unsigned T1 = entrop_arr[0] % 2521;
        unsigned T2 = entrop_arr[1] % 2339;

        unsigned S1 = (entrop_T0 * 3257 + T1 * 5023) % 2833;
        unsigned S2 = (entrop_T0 * 4051 + T2 * 4643) % 3923;

        entrop_T0 = (S1 * S2 * 27737 + 11831) % 4219;
        unsigned R0 = (S1 * S2 * 41263 + 11971) % 65729;
        ((void)R0);
        // pipe_write( , &R0, 2, IO_NO_BLOCK);
    }
}

