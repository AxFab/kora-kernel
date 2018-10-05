
#if 0
struct inode {
    int fd;
    int rcu;
};

inode_t *vfs_inode()
{
    inode_t *ino = (inode_t *)kalloc(sizeof(inode_t));
    ino->rcu = 1;
    return ino;
}
inode_t *vfs_open(inode_t *ino)
{
    if (ino != NULL)
        ino->rcu++;
    return ino;
}

void vfs_close(inode_t *ino)
{
    if (--ino->rcu == 0)
        kfree(ino);
}

/* Find the page mapping the content of a block inode */
page_t ioblk_page(inode_t *ino, off_t off)
{
    (void)ino;
    assert(IS_ALIGNED(off, PAGE_SIZE));
    return 0x4000 + off;
}

int vfs_readlink(inode_t *ino, char *buf, int len, int flags)
{
    buf[0] = 'a';
    buf[1] = '\0';
    return 0;
}

void vfs_read(inode_t *ino, char *buf, size_t len, off_t off)
{

}
#endif

