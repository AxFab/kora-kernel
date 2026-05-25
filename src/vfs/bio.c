// Block IO
#include <kernel/vfs.h>
#include <kernel/memory.h>
#include <kernel/threads.h>

struct bio_queue {
    mtx_t lock;
    cnd_t wake;
    bbtree_t pending;
    llhead_t inflight;
    bio_submit_fn submit;
    inode_t *dev;
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void bio_queue_alloc(inode_t *dev)
{
    bio_queue_t *bioq = kalloc(sizeof(bio_queue_t));
    cnd_init(&bioq->wake);
    mtx_init(&bioq->lock, mtx_plain);
    // INIT pending / inflight
    // submit ?
    // TODO -- Set on dev-> ?
    bioq->dev = dev;
}

void bio_queue_free(inode_t *dev)
{
    bio_queue_t *bioq = NULL; // TODO -- Get from dev-> ?
    kfree(bioq);
}

void bio_deamon(inode_t *dev)
{
    might_sleep();
    // struct timespec ts { tv_sec = 180, tv_nsec = 0 };
    bio_queue_t *bioq = NULL; // TODO -- Get from dev-> ?
    mtx_lock(&bioq->lock);
    for (;;) {
        if (cnd_wait(&bioq->wake, &bioq->lock) != 0) {
            // LOG + DELAY
            continue;
        }
        bio_t *bio = bbtree_first(&bioq->pending, bio_t, bnode);
        if (bio == NULL)
            continue; // Odd ?
        // TODO Put it on the inflight list ?
        mtx_unlock(&bioq->lock);
        dev->ops->submit(dev, bio);
        bio_wait(bio);
    }
}

bio_t *bio_alloc(inode_t *dev, int op, page_t phys, xoff_t lba, size_t nsectors)
{
    assert(nsectors * dev->dev->block == PAGE_SIZE); // Single phys-page supported for now
    bio_t *bio = kalloc(sizeof(bio_t));
    cnd_init(&bio->done);
    mtx_init(&bio->lock, mtx_plain);
    // INIT BBTREE NODE AND LIST NODE !?
    bio->dev = dev;
    bio->op = op;
    bio->phys = phys;
    bio->lba = lba;
    bio->nsectors = nsectors;
    return bio;
}

void bio_free(bio_t *bio)
{
    // TODO -- Assert mutex is available and nobody is waiting?
    kfree(bio);
}

void bio_submit(bio_t *bio)
{
    bio_queue_t *bioq = NULL; // TODO -- Get from bio->dev-> ?
    mtx_lock(&bioq->lock);
    bio->bnode.value = bio->lba;
    bbtree_insert(&bioq->pending, &bio->bnode);
    cnd_signal(&bioq->wake);
    mtx_unlock(&bioq->lock);
}

int bio_wait(bio_t *bio)
{
    cnd_wait(&bio->done, &bio->lock);
    return bio->err;
}

void bio_complete(bio_t *bio, int err)
{
    // bio_queue_t *bioq = NULL; // TODO -- Get from bio->dev-> ?
    bio->err = err;
    cnd_broadcast(&bio->done);
}
