#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <kernel/vfs.h>

#ifndef O_BINARY
# define O_BINARY 0
#endif
#ifndef O_NOFOLLOW
# define O_NOFOLLOW 0x100000
#endif

//hmap_t __inode_map;
//bool __inode_init = false;

typedef struct file
{
    inode_t *ino;
    int fd;
    char *name;
    // Block cache
    size_t pages;
    size_t alloc;
    size_t *pcache;
    int *rcus;
    int prcu;
} file_t;

//static void __vfs_init()
//{
//    if (__inode_init)
//        return;
//    hmp_init(&__inode_map, 8);
//    __inode_init = true;
//}

char *vfs_inokey(inode_t *ino, char *buf)
{
    file_t *file = ino->drv_data;
    strncpy(buf, file->name, 16);
    return buf;
}

inode_t *vfs_open_inode(inode_t *ino)
{
    atomic_inc(&ino->rcu);
    return ino;
}

void vfs_close_inode(inode_t *ino)
{
    if (atomic_xadd(&ino->rcu, -1) != 1)
        return;
    file_t *file = ino->drv_data;
    close(file->fd);
    free(file->name);
    assert(file->prcu == 0);
    if (file->pages > 0) {
        free(file->pcache);
        free((int *)file->rcus);
    }
    kfree(ino);
    free(file);
}

size_t page_new();
void page_release(size_t);

size_t vfs_fetch_page(inode_t *ino, xoff_t off)
{
    file_t *file = ino->drv_data;
    size_t idx = (size_t)(off / PAGE_SIZE);
    if (file->pages <= idx) {
        size_t len = ALIGN_UP(idx + 1, 8);
        file->pcache = realloc(file->pcache, len * sizeof(size_t));
        file->rcus = realloc(file->rcus, len * sizeof(int));
        memset(&file->pcache[file->pages], 0, (len - file->pages) * sizeof(size_t));
        memset(&file->rcus[file->pages], 0, (len - file->pages) * sizeof(int));
        file->pages = len;
    }
    
    if (file->pcache[idx] == 0) {
        file->pcache[idx] = page_new();
        file->alloc++;
    }
    size_t pg = file->pcache[idx];
    file->prcu++;
    file->rcus[idx]++;
    return pg;
}

int vfs_release_page(inode_t *ino, xoff_t off, size_t pg, bool dirty)
{
    file_t *file = ino->drv_data;
    size_t idx = (size_t)(off / PAGE_SIZE);
    if (file->pages <= idx || file->pcache[idx] == 0)
        return -1;
    file->prcu--;
    if (--file->rcus[idx] == 0) {
        // TODO -- Simul write-back !
        page_release(file->pcache[idx]);
        file->pcache[idx] = 0;
    }
    return 0;
}

inode_t *vfs_search_ino(fs_anchor_t *fsanchor, const char *name, user_t *user, bool follow)
{
    // __vfs_init();

    file_t *file = NULL; // hmp_get(&__inode_map, name, strlen(name));
    // if (file != NULL)
    //     return vfs_open_inode(file->ino);

    while (*name == '/')
        name++;

    int fd = open(name, O_RDWR | O_BINARY);
    if (fd == -1)
        return NULL;

    file = calloc(1, sizeof(file_t));
    file->ino = kalloc(sizeof(inode_t));
    file->fd = fd;
    char *ptr = strrchr(name, '/');
    file->name = strdup(ptr == NULL ? name : ptr);
    xtime_t now = xtime_read(XTIME_CLOCK);
    file->ino->ctime = now;
    file->ino->mtime = now;
    file->ino->atime = now;
    file->ino->btime = now;
    file->ino->drv_data = file;
    file->ino->length = lseek(fd, 0, SEEK_END);
    file->ino->links = 1;
    file->ino->mode = 0644;
    file->ino->rcu = 1;
    file->ino->type = FL_REG;
    file->ino->no = fd;
    return file->ino;
}

int vfs_read(inode_t *ino, char *buf, size_t length, xoff_t off, int flags)
{
    file_t *file = ino->drv_data;
    lseek(file->fd, off, SEEK_SET);
    return read(file->fd, buf, length);
    // return length;
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <kernel/blkmap.h>
#include <stdlib.h>

#if !defined(_WIN32)
# define _valloc(s)  valloc(s)
# define _vfree(p)  free(p)
#else
# define _valloc(s)  _aligned_malloc(s, PAGE_SIZE)
# define _vfree(p)  _aligned_free(p)
#endif

static void *blk_host_map_(blkmap_t *map, size_t no, int rights)
{
    xoff_t roff = no * map->block;
    xoff_t moff = ALIGN_DW(roff, map->msize);
    if (map->ptr == NULL || map->off != moff || map->rights != rights) {
        if (map->ptr != NULL)
            _vfree(map->ptr);
        map->rights = rights;
        map->off = moff;
        map->ptr = _valloc(map->msize);
        file_t *file = (file_t *)map->ino->drv_data;
        lseek(file->fd, map->off, SEEK_SET);
        read(file->fd, map->ptr, map->msize);
    }
    return ADDR_OFF(map->ptr, roff - moff);
}

static blkmap_t *blk_host_clone_(blkmap_t *map)
{
    blkmap_t *map2 = kalloc(sizeof(blkmap_t));
    map2->ino = map->ino;
    map2->block = map->block;
    map2->msize = map->msize;
    map2->map = map->map;
    map2->clone = map->clone;
    map2->close = map->close;
    return map2;
}

static void blk_host_close_(blkmap_t *map)
{
    if (map->ptr != NULL)
        _vfree(map->ptr);
    kfree(map);
}

blkmap_t *blk_open(inode_t *ino, size_t blocksize)
{
    blkmap_t *map = kalloc(sizeof(blkmap_t));
    map->ino = ino;
    map->block = PAGE_SIZE;
    map->msize = PAGE_SIZE;
    map->map = blk_host_map_;
    map->clone = blk_host_clone_;
    map->close = blk_host_close_;
    return map;
}