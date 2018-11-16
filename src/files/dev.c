#include <kernel/vfs.h>
#include <kernel/device.h>
#include <kernel/core.h>
#include <errno.h>

int null_read(inode_t* ino, char *buf, size_t len, int flags) {
	while (flags & VFS_BLOCK)
	    async_wait(NULL, NULL, -1);
	errno = EWOULDBLOCK;
    return 0;
}

int null_write(inode_t* ino, char *buf, size_t len, int flags) {
	errno = 0;
	return len;
}

int zero_read(inode_t* ino, char *buf, size_t len, int flags) {
	memset(buf, 0, len);
	errno = 0;
	return len;
}

int rand_read(inode_t* ino, char *buf, size_t len, int flags) {
	int i;
	for (i = 0; i < len; ++i) 
	    buf[i] = rand8();
	errno = 0;
	return len;
}

dev_ops_t dev_ops = {
};

ino_ops_t null_ino_ops = {
    .read = null_read, 
    .write = null_write, 
};

ino_ops_t zero_ino_ops = {
    .read = zero_read,
};

ino_ops_t rand_ino_ops = {
    .read = rand_read,
};

void dev_setup() 
{
	inode_t *null_ino = vfs_inode(1, FL_CHR, NULL);
    null_ino->und.dev->block = 1;
    null_ino->und.dev->ops = &dev_ops;
    null_ino->ops = &null_ino_ops;
    null_ino->und.dev->devclass = (char*)"Bytes devices";
    vfs_mkdev(null_ino, "null");
    vfs_close(null_ino);
    
	inode_t *zero_ino = vfs_inode(2, FL_CHR, NULL);
    zero_ino->und.dev->block = 1;
    zero_ino->und.dev->flags = VFS_RDONLY;
    zero_ino->und.dev->ops = &dev_ops;
    zero_ino->ops = &zero_ino_ops;
    zero_ino->und.dev->devclass = (char*)"Bytes devices";
    vfs_mkdev(zero_ino, "zero");
    vfs_close(zero_ino);
        
	inode_t *rand_ino = vfs_inode(3, FL_CHR, NULL);
    rand_ino->und.dev->block = 1;
    rand_ino->und.dev->flags = VFS_RDONLY;
    rand_ino->und.dev->ops = &dev_ops;
    rand_ino->ops = &rand_ino_ops;
    rand_ino->und.dev->devclass = (char*)"Bytes devices";
    vfs_mkdev(rand_ino, "rand");
    vfs_close(rand_ino);
}

void dev_teardown() {} 

MODULE(dev, dev_setup, dev_teardown);
