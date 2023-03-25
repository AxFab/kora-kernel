// Block IO
#include <kernel/vfs.h>
#include <kernel/memory.h>

typedef struct bio bio_t;
typedef struct bio_req bio_req_t;
typedef struct bio_algo bio_algo_t;
struct bio {
    inode_t *ino;
    bio_req_t *head;
    bio_req_t *tail;
};

struct bio_req {
    bio_req_t *next;
    xoff_t lba;
    size_t cnt;
    xtime_t exp;
    bool write;
};

struct bio_algo {
    void *item;
    void (*push)(void *item, bio_t* reqs);
    bio_req_t *(*pop)(void *item);
};


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Deadline

typedef struct bio_deadline bio_deadline_t;
struct bio_deadline {
    bio_req_t *head;
    bio_req_t *rlist;
    bio_req_t *wlist;
    mtx_t mtx;
    cnd_t cnd;
    size_t rio;
    size_t wio;
};

// void bio_deadline_push(bio_deadline_t* sched, bio_t * bio)
// {
//     mtx_lock(&sched->mtx);
//     bio_req_t* cursor = sched->head;
//     bio_req_t* req = bio->head;
//     xoff_t lba = -1;
//     while (req != NULL) {
//         if (cursor->next == NULL) {
//             // Add all to tail
//         } else if (bio_intersect(req, cursor->next)) {
//             bio_merge(req, cursor->next);
//             bio_req_t *del = req;
//             req = req->next;
//             bio_free(del);
//         } else if (req->lba < cursor->next->lba || cursor->next->lba < cursor->lba) {
//             // insertion of one...
//             req = req->next;
//         }

//         cursor = cursor->next;
//     }
//     mtx_unlock(&sched->mtx);
// }

bio_req_t *bio_deadline_pop(bio_deadline_t* sched)
{
    bio_req_t *res;
    might_sleep();
    mtx_lock(&sched->mtx);
    while (sched->head == NULL) {
        // Sleep until new request arrive, if only write keep 3sec delay
        cnd_wait(&sched->cnd, &sched->mtx);
        struct timespec xt;
        xt.tv_sec = 3;
        xt.tv_nsec = 0;
        while (sched->rio == 0 && xt.tv_sec > 0)
            cnd_timedwait(&sched->cnd, &sched->mtx, &xt);
        
    }

    xtime_t now = xtime_read(XTIME_CLOCK);
    if (sched->rlist && sched->rlist->exp <= now) { 
        res = sched->rlist;
        sched->rlist = sched->rlist->next;
        sched->rio--;
        mtx_unlock(&sched->mtx);
        return res;
    }

    if (sched->wlist && sched->wlist->exp <= now) {
        res = sched->wlist;
        sched->wlist = sched->wlist->next;
        sched->wio--;
        mtx_unlock(&sched->mtx);
        return res;
    }

    res = sched->head;
    sched->head = sched->head->next;
    if (res->write)
        sched->wio--;
    else
        sched->rio--;
    mtx_unlock(&sched->mtx);
    return res;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void bio_task(inode_t *ino, bio_req_t *req)
{
    int ret = 0;
    might_sleep();
    size_t bsize = ino->dev->block;
    uint8_t bpp = PAGE_SIZE / bsize;
    xoff_t moff = bpp - (req->lba & (bpp - 1));
    xoff_t mlba = req->lba - moff;
    size_t psize = ALIGN_UP(req->lba + (xoff_t)req->cnt, bpp) / bpp;
    int(*ops)(inode_t *, char *, size_t, xoff_t, int) = req->write ? ino->ops->write : ino->ops->read;
#if 0
    char *ptr = kmap(psize * PAGE_SIZE, NULL, 0, VM_RW | VMA_PHYS);
    for (int i = 0; i < psize; ++i) {
        size_t page = vfs_fetch_page(ino, (mlba + i * bpp) * req->bsize);
        mmu_resolve(ptr, page, VM_RW);
    }
    ret = ops(ino, ptr + (moff * req->bsize), req->lba * req->bsize, req->cnt * req->bsize, 0);
    kunmap(ptr, psize * PAGE_SIZE);
#else
    for (int i = 0; i < psize; ++i) {
        size_t page = vfs_fetch_page(ino, (mlba + i * bpp) * bsize, true);
        char *ptr = kmap(PAGE_SIZE, NULL, page, VM_RW | VMA_PHYS);
        char *buf = i != 0 ? ptr : ptr + moff * bsize;
        size_t pos = (req->lba + i * bpp) * bsize;
        size_t len = bpp * bsize;
        ret = ops(ino, buf, pos, len, 0);
        kunmap(ptr, PAGE_SIZE);
        if (ret != 0)
            break;
    }
#endif
    return ret;
}


void bio_deamon(inode_t *ino)
{
    bio_algo_t *algo = ino->drv_data; // / bio->ino;
    for (;;) {
        might_sleep();
        bio_req_t *req = algo->pop(algo->item);
        bio_task(ino, req);
    }
}


void bio_push(bio_t *bio)
{
    inode_t *ino = bio->ino;
    // Check we have a block device which use blockIO
    bio_algo_t *algo = ino->drv_data;// ->bio;
    algo->push(algo->item, bio);
}

void bio_sync(inode_t *ino)
{
    // Check we have a block device which use blockIO
    bio_algo_t *algo = ino->drv_data;//->bio;
    // cnd_wait(&algo->sync_cnd);
}

void bio_wait(bio_t *bio)
{

}

bio_t *bio_create(inode_t *ino)
{
}

typedef struct block_file block_file_t;
typedef struct block_page block_page_t;

void bio_request(bio_t *bio, block_page_t *page, size_t lba, size_t cnt, int flags)
{
}
