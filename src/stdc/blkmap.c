#include <stdio.h>
#include <kernel/blkmap.h>

#ifdef KORA_KRN

static void *blk_map_(blkmap_t *map, size_t no, int rights)
{
    xoff_t roff = no * map->block;
    xoff_t moff = ALIGN_DW(roff, map->msize);
    if (map->ptr == NULL || map->off != moff || map->rights != rights) {
        if (map->ptr != NULL)
            kunmap(map->ptr, map->msize);
        map->rights = rights;
        map->off = moff;
        map->ptr = kmap(map->msize, map->ino, map->off, rights);
    }
    return ADDR_OFF(map->ptr, roff - moff);
}


static blkmap_t *blk_clone_(blkmap_t *map)
{
    blkmap_t *map2 = kalloc(sizeof(blkmap_t));
    map2->ino = vfs_open_inode(map->ino);
    map2->block = map->block;
    map2->msize = map->msize;
    map2->map = map->map;
    map2->clone = map->clone;
    map2->close = map->close;
    return map2;
}

static void blk_close_(blkmap_t *map)
{
    vfs_close_inode(map->ino);
    if (map->ptr != NULL)
        kunmap(map->ptr, map->msize);
    kfree(map);
}

blkmap_t *blk_open(inode_t *ino, size_t blocksize)
{
    if (!POW2(blocksize))
        return NULL;
    blkmap_t *map = kalloc(sizeof(blkmap_t));
    map->ino = vfs_open_inode(ino);
    map->block = blocksize;
    map->msize = ALIGN_UP(blocksize, PAGE_SIZE);
    map->map = blk_map_;
    map->clone = blk_clone_;
    map->close = blk_close_;
    return map;
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#else 

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
        fseek((FILE *)map->ino, map->off, SEEK_SET);
        fread(map->ptr, map->msize, 1, (FILE *)map->ino);
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

blkmap_t *blk_host_open(const char *path, size_t blocksize)
{
    if (!POW2(blocksize))
        return NULL;
    blkmap_t *map = kalloc(sizeof(blkmap_t));
    map->ino = fopen(path, "r");
    map->block = blocksize;
    map->msize = blocksize; // ALIGN_UP(blocksize, PAGE_SIZE);
    map->map = blk_host_map_;
    map->clone = blk_host_clone_;
    map->close = blk_host_close_;
    return map;
}

#endif
