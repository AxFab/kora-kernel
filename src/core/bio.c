/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/device.h>
#include <kora/mcrs.h>
#include <kora/hmap.h>

typedef struct bio_page bio_page_t;

struct bio_page {
    void *base;
    size_t lba;
    int rcu;
    llnode_t node;
};

struct bio {
    inode_t *ino;
    int map_flags;
    int block;
    int map_size;
    int modulus;
    int factor;
    int off_block;
    size_t offset;
    HMP_map table;
    int lru_size;
    llhead_t lru;
};


bio_t *bio_create(inode_t *ino, int flags, int block, size_t offset)
{
    bio_t *io = (bio_t *)kalloc(sizeof(bio_t));
    io->ino = ino;
    io->map_flags = flags;
    io->block = block;
    io->offset = offset;
    io->off_block = io->block;
    io->factor = 1;
    if (io->offset != 0) {
        io->off_block = 1;
        while ((io->offset & 1) == 0 && io->off_block < io->block) {
            io->off_block = io->off_block << 1;
            io->offset = io->offset >> 1;
        }
        io->factor = block / io->off_block;
        block = POW2_UP((io->offset % io->factor) * io->off_block + block);
    }
    io->map_size = MAX(block, PAGE_SIZE);
    io ->modulus = io->map_size / io->off_block;
    io->lru_size = 8;
    hmp_init(&io->table, 16);
    return io;
}

bio_t *bio_create2(inode_t *ino, int flags, int block, size_t offset, int extra)
{
    bio_t *io = bio_create(ino, flags, block, offset);
    io->map_size = ALIGN_UP(io->map_size + extra * io->block, PAGE_SIZE);
    return io;
}

void *bio_access(bio_t *io, size_t lba)
{
    lba = lba * io->factor + io->offset;
    size_t offset = io->modulus != 1 ? lba % io->modulus : 0;
    lba -= offset;
    bio_page_t *page = (bio_page_t *)hmp_get(&io->table, (char *)&lba, sizeof(lba));
    if (page == NULL) {
        page = (bio_page_t *)kalloc(sizeof(bio_page_t));
        page->lba = lba;
        page->base = kmap(io->map_size, io->ino, lba * (io->block / io->factor), io->map_flags);
        hmp_put(&io->table, (char *)&lba, sizeof(lba), page);
    } else if (page->rcu == 0)
        ll_remove(&io->lru, &page->node);
    ++page->rcu;
    return (uint8_t *)page->base + (offset * io->off_block);
}

void bio_clean(bio_t *io, size_t lba)
{
    lba = lba * io->factor + io->offset;
    size_t offset = io->modulus != 1 ? lba % io->modulus : 0;
    lba -= offset;
    bio_page_t *page = (bio_page_t *)hmp_get(&io->table, (char *)&lba, sizeof(lba));
    assert(page != NULL);
    if (--page->rcu == 0) {
        ll_push_back(&io->lru, &page->node);
        if (io->lru.count_ > io->lru_size) {
            page = itemof(ll_pop_front(&io->lru), bio_page_t, node);
            kunmap(page->base, io->map_size);
            hmp_remove(&io->table, (char *)&page->lba, sizeof(page->lba));
            kfree(page);
        }
    }
}

void bio_sync(bio_t *io)
{
    while (io->lru.count_ > 0) {
        bio_page_t *page = itemof(ll_pop_front(&io->lru), bio_page_t, node);
        kunmap(page->base, io->map_size);
        hmp_remove(&io->table, (char *)&page->lba, sizeof(page->lba));
        kfree(page);
    }
}

void bio_destroy(bio_t *io)
{
    bio_sync(io);
    assert(io->table.count_ == 0);
    hmp_destroy(&io->table, 0);
    kfree(io);
}


