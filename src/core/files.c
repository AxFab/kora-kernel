#include <kernel/core.h>
#include <kernel/sys/inode.h>
#include <kernel/task.h>
#include <kora/rwlock.h>
#include <errno.h>

struct stream
{
    inode_t *ino;
    bbnode_t node; // TODO -- Is BBTree the best data structure !?
    rwlock_t lock;
};

struct filelist
{
    bbtree_t *tree; // TODO -- Is BBTree the best data structure !?
    rwlock_t lock;
};


filelist_t *file_list()
{
    filelist_t *flist = (filelist_t*)kalloc(sizeof(filelist_t));
    bbtree_init(&flist->tree);
    rwlock_init(&flist->lock);
    return flist;
}


stream_t *file_get(filelist_t *flist, int fd)
{
    rwlock_rdlock(&flist->lock);
    stream_t *stm = bbtree_search_eq(&flist->tree, (size_t)fd, stream_t, node);
    rwlock_rdunlock(&flist->lock);
    return stm;
}

int file_set(inode_t *ino)
{
    rwlock_wrlock(&flist->lock);
    stream_t *stm = (stream_t*)kalloc(sizeof(stream_t));
    stm->ino = ino;
    rwlock_init(&stm->lock);
    if (flist->tree.count == 0) {
        stm->node.value_ = 0;
    } else {
        stm->node.value_ = (int)(bbtree_last(&flist->tree, stream_t, node)->node.value_ + 1);
        if ((int)stm->node.value_ < 0) {
            kfree(stm);
            errno = ENOMEM; // TODO -- need a free list !
            return -1;
        }
    }
    rwlock_wrunlock(&flist->lock);
}

int file_close(int fd)
{

}

