#include <kernel/vfs.h>

typedef struct dfs_table dfs_table_t;
typedef struct dfs_info dfs_info_t;

struct dfs_table
{
    dfs_table_t *next;
    int length;
    int free;
};

struct dfs_info
{
    int ino;
    char uuid[16];
    char name[32];
    char label[32];
    int show;
    int filter;
    inode_t *dev;
};

dfs_table_t *table0;

dfs_table_t *devfs_extends()
{
    dfs_table_t *table;
    table = kmap(PAGE_SIZE, NULL, 0, VMA_ANON_RW);
    table->length = (PAGE_SIZE - sizeof(dfs_table_t)) / sizeof(dfs_info_t);
    return table;
}


static dfs_info_t *devfs_fetch_new()
{
    int idx;
    dfs_table_t *table = table0;
    for (idx = 0; ; ++idx) {
	if (table->free == 0 || idx >= table->length) {
	    if (table->next == NULL)
		table->next = devfs_extends();
	    table = table->next;
	    idx = 0;
	}
	if (table->entry[idx].flags == 0) {
	    table->entry[idx].flags = 1;
	    return &table->entry[idx];
	}
    }
}

static dfs_info_t *devfs_fetch(int no)
{
    dfs_table_t *table = table0;
    while (no > table->length) {
        no -= table->length;
        table = table->next;
    }
    return &table->entries[no - 1];
}

static int devfs_dir(const char *name, int prt, int show, int flag)
{
    dfs_info_t *info = devfs_fetch_new();
    info->prt = prt;
    info->show = show;
    info->filter = filter;
    strncpy(info->name, name, 32);
    info->flags = 2;
    return info->ino;
}

void devfs_mount()
{
    int root = devfs_dir("", 0, DF_ROOT, 0);
    int bus = devfs_dir("bus", root, DF_BUS, DF_ROOT);
    int dsk = devfs_dir("disk", root, DF_DISK, DF_ROOT);
    int inp = devfs_dir("input", root, DF_INPUT, DF_ROOT);
    int mnt = devfs_dir("mnt", root, DF_MOUNT, DF_ROOT);
    int net = devfs_dir("net", root, DF_NET, DF_ROOT);
    int shm = devfs_dir("shm", root, DF_SHM, DF_ROOT);

    int di = devfs_dir("by-uuid", dsk, DF_DISK1, DF_DISK);
    int dl = devfs_dir("by-label", dsk, DF_DISK2, DF_DISK);

    int mi = devfs_dir("by-uuid", mnt, DF_MOUNT1, DF_MOUNT);
    int ml = devfs_dir("by-label", mnt, DF_MOUNT2, DF_MOUNT);

    int pci = devfs_dir("pci", bus, DF_CST1, DF_BUS);
    int usb = devfs_dir("usb", bus, DF_CST2, DF_BUS);

    dfs_info_t *dve = devfs_fetch_new();
    inode_t *dvi = vfs_inode(dve->ino, FL_DIR, NULL);
    
    dfs_info_t *zre = devfs_fetch_new();
    inode_t *zri = vfs_inode(zre->ino, FL_CHR, dev);
}


