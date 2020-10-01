#ifndef __KERNEL_RESX_STREAM_H
#define __KERNEL_RESX_STREAM_H 1

#include <stddef.h>

#define rcu_open(n)  rcu_open_(n, &(n)->rcu)
#define rcu_close(n,dtor) do { if (rcu_close_(n, &(n)->rcu)) dtor(n); } while(0)

static inline void *rcu_open_(void *ptr, atomic_int *rcu)
{
    atomic_fetch_add(rcu, 1);
    return ptr;
}

static inline bool rcu_close_(void *ptr, atomic_int *rcu)
{
    int val = atomic_fetch_sub(rcu, 1);
    return val == 1;
}

struct stream_pool {
    bbtree_t tree;
    rwlock_t lock;
    atomic_int rcu;
};

// struct stream_t {
//     inode_t *ino;
//     unsigned flags;
//     size_t pos;
// };

int resx_rm(resx_t *resx, int fd);
resx_t *resx_create();
resx_t *resx_open(resx_t *resx);
stream_t *resx_get(resx_t *resx, int fd);
stream_t *resx_set(resx_t *resx, inode_t *ino, int access);
void resx_close(resx_t *resx);


#endif  /* __KERNEL_RESX_STREAM_H */
