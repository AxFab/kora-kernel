#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/task.h>
#include <kora/bbtree.h>
#include <kora/llist.h>
#include <kora/rwlock.h>
#include <kora/mcrs.h>
#include <errno.h>

#define IO_TIMEOUT 10000 // 10ms

typedef struct blkpage blkpage_t;
typedef struct blkcache blkcache_t;

struct blkpage {
	bbnode_t bnode;
	atomic_uint usage;
	page_t phys;
	inode_t *ino;
	bool dirty;
};

struct blkcache {
	bbtree_t tree;
	rwlock_t lock;
	llhead_t wlist; /* Waiting list for readers */
};

/* Request sync to driver */
static int ioblk_sync_(blkpage_t *page)
{
	void *map = kmap(PAGE_SIZE, NULL, page->phys, VMA_PHYSIQ);
	int ret = vfs_write(page->ino, map, PAGE_SIZE, (off_t)page->bnode.value_);
	assert((ret == 0) != (errno != 0));
	kunmap(map, PAGE_SIZE);
	if (ret == 0)
	    page->dirty = false;
	return ret;
}


/* Request fetch to driver */
static int ioblk_fetch_(blkpage_t *page)
{
	void *map = kmap(PAGE_SIZE, NULL, 0, VMA_PHYSIQ);
	int ret = vfs_read(page->ino, map, PAGE_SIZE, (off_t)page->bnode.value_);
	assert((ret == 0) != (errno != 0));
	kunmap(map, PAGE_SIZE);
	if (ret == 0)
	    page->phys = mmu_read(NULL, (size_t)map);
	return ret;
}

static void ioblk_close(blkcache_t *cache, blkpage_t *page, bool write_lock)
{
	/* Decrement RCU */
	if (atomic_fetch_add(&page->usage, -1) != 1)
	    return;

	if (!write_lock) {
		/* Grab lock and recheck */
		rwlock_upgrade(&cache->lock);
		if (page->usage != 0) {
			rwlock_wrunlock(&cache->lock);
			rwlock_rdlock(&cache->lock);
			return;
		}
	}

    /* Synchronize if needed */
    if (page->dirty) {
    	rwlock_wrunlock(&cache->lock);
        ioblk_sync_(page);
        rwlock_wrlock(&cache->lock);
        // TODO - Still ok for trashing!?
    }

    bbtree_remove(&cache->tree, page->bnode.value_);
    rwlock_wrunlock(&cache->lock);
    page_release(page->phys);
    kfree(page);
}

static blkpage_t *ioblk_search(inode_t *ino, off_t off)
{
	/* Do some checks */
	assert(off >= 0 && IS_ALIGN(off, PAGE_SIZE));
	assert(S_ISREG(ino->mode) || S_ISBLK(ino->mode));
	if (off >= ino->length) {
		errno = EINVAL;
		return NULL;
	}

	/* Look for a known page */
	blkcache_t *cache = (blkcache_t*)ino->object;
	assert(true/* cache is locked in rd or wr */);
	blkpage_t *page = bbtree_search_eq(&cache->tree, (size_t)off, blkpage_t, bnode);
	if (page == NULL)
	    return NULL;
	// TODO REMOVE FROM LRU
	atomic_inc(&page->usage);
	errno = 0;
	return page;
}


void ioblk_init(inode_t *ino)
{
	blkcache_t *cache = (blkcache_t*)kalloc(sizeof(blkcache_t));
	rwlock_init(&cache->lock);
	bbtree_init(&cache->tree);
	ino->object = cache;
}

void ioblk_sweep(inode_t *ino)
{
	blkcache_t *cache = (blkcache_t*)ino->object;
	rwlock_wrlock(&cache->lock);
	// REMOVE ALL PAGES
	assert(cache->tree.count_ == 0);
	rwlock_wrunlock(&cache->lock);
	kfree(cache);
}

void ioblk_release(inode_t *ino, off_t off)
{
	blkcache_t *cache = (blkcache_t*)ino->object;
	rwlock_rdlock(&cache->lock);
	blkpage_t *page = ioblk_search(ino, off);
	assert(page != NULL);
	ioblk_close(cache, page, false);
	ioblk_close(cache, page, false);
	rwlock_rdunlock(&cache->lock);
}

/* Find the page mapping the content of a block inode */
page_t ioblk_page(inode_t *ino, off_t off)
{
	blkcache_t *cache = (blkcache_t*)ino->object;
	rwlock_rdlock(&cache->lock);
	blkpage_t *page = ioblk_search(ino, off);

	/* If the page is referenced return aonce ready */
	if (page != NULL) {
		if (page->phys || advent_wait_rd(&cache->lock, &cache->wlist, IO_TIMEOUT) == 0) {
			rwlock_rdunlock(&cache->lock);
			errno = 0;
			return page->phys;
		}
		assert(errno != 0);
		int err = errno;
		ioblk_close(cache, page, false);
		rwlock_rdunlock(&cache->lock);
		errno = err;
		return 0;
	}

	/* Reference the page */
    page = (blkpage_t*)kalloc(sizeof(blkpage_t));
    page->ino = ino;
    page->bnode.value_ = (size_t)off;
    ++page->usage;
    rwlock_upgrade(&cache->lock);
    bbtree_insert(&cache->tree, &page->bnode);
    rwlock_wrunlock(&cache->lock);

    /* Request fetch to driver */
    int ret = ioblk_fetch_(page);
    rwlock_wrlock(&cache->lock);
    advent_awake(&cache->wlist, errno);
    if (ret != 0)
        ioblk_close(cache, page, true);
    rwlock_wrunlock(&cache->lock);
    return page->phys;
}

/* Synchronize a page mapping the content of a block inode */
int ioblk_sync(inode_t *ino, off_t off)
{
	blkcache_t *cache = (blkcache_t*)ino->object;
	rwlock_rdlock(&cache->lock);
	blkpage_t *page = ioblk_search(ino, off);
	assert(page != NULL);
	/* Request sync to driver */
	rwlock_rdunlock(&cache->lock);
	int ret = ioblk_sync_(page);
	rwlock_rdlock(&cache->lock);
	ioblk_close(cache, page, false);
	rwlock_rdunlock(&cache->lock);
	return ret;
}

void ioblk_dirty(inode_t *ino, off_t off)
{
	blkcache_t *cache = (blkcache_t*)ino->object;
	rwlock_rdlock(&cache->lock);
	blkpage_t *page = ioblk_search(ino, off);
	assert(page != NULL);
	page->dirty = true;
    // PUSH ON DIRTY LIST
	ioblk_close(cache, page, false);
	rwlock_rdunlock(&cache->lock);
}
