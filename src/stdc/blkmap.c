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
    map->ino = (void*)fopen(path, "r");
    map->block = blocksize;
    map->msize = blocksize; // ALIGN_UP(blocksize, PAGE_SIZE);
    map->map = blk_host_map_;
    map->clone = blk_host_clone_;
    map->close = blk_host_close_;
    return map;
}

#endif
