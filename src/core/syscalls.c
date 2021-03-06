#include <kernel/tasks.h>
#include <kernel/memory.h>
#include <kernel/vfs.h>
#include <errno.h>
#include <kernel/syscalls.h>

#include <bits/mman.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void sys_exit(int code)
{
    task_stop(__current, code);
    for (;;)
        scheduler_switch(TS_ZOMBIE);
}

// long sys_wait(int pid, int *status, int option)
// {
//     for (;;) {
//         // TODO - Does a child did terminate ?
//         scheduler_switch(TS_BLOCKED);
//     }
// }


long sys_sleep(xtime_t *timeout, xtime_t *remain)
{
    // TODO - Check mspace_check(__current->vm, remain, sizeof(long));
    if (*timeout <= 0)
        scheduler_switch(TS_READY);
    else {
        xtime_t rest = sleep_timer(*timeout);
        if (*remain)
            *remain = rest;
    }
    errno = 0;
    return 0;
}


long sys_futex_wait(int *addr, int val, xtime_t *timeout, int flags)
{
    // TODO - Check mspace_check(__current->vm, addr, sizeof(int));
    return futex_wait(addr, val, *timeout, flags);
}

long sys_futex_requeue(int *addr, int val, int val2, int *addr2, int flags)
{
    // TODO - Check mspace_check(__current->vm, addr, sizeof(int));
    return futex_requeue(addr, val, val2, addr2, flags);
}

long sys_futex_wake(int *addr, int val)
{
    // TODO - Check mspace_check(__current->vm, addr, sizeof(int));
    return futex_wake(addr, val);
}


long sys_spawn(const char *program, const char **args, const char **envs, int *streams, int flags)
{
    if (mspace_check_str(__current->vm, program, 4096) != 0)
        return -1;
    // TODO -- Check arrays and content of args / envs

    fstream_t *strm;
    fsnode_t *nodes[4];

    strm = stream_get(__current->fset, streams[0]);
    if (strm != NULL)
        nodes[0] = strm->file;

    strm = stream_get(__current->fset, streams[1]);
    if (strm != NULL)
        nodes[1] = strm->file;

    strm = stream_get(__current->fset, streams[2]);
    if (strm != NULL)
        nodes[2] = strm->file;

    return task_spawn(program, args, nodes);
}

long sys_thread(const char *name, void *entry, void *params, size_t len, int flags)
{
    if (name != NULL && mspace_check_str(__current->vm, name, 4096) != 0)
        return -1;
    if (mspace_check(__current->vm, params, len, VM_RD) != 0)
        return -1;

    return task_thread(name, entry, params, len, KEEP_FS | KEEP_FSET | KEEP_VM);
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void *sys_mmap(void *addr, size_t length, unsigned flags, int fd, size_t off)
{
    inode_t *ino = NULL;
    if (fd >= 0) {
        fstream_t *strm = stream_get(__current->fset, fd);
        if (strm == NULL) {
            errno = EBADF;
            return (void *) -1;
        }
        ino = strm->file->ino;
    }

    unsigned vma = flags & VM_RWX;
    if (ino != NULL)
        vma |= VMA_FILE;
    else if (flags & MAP_HEAP)
        vma |= VMA_HEAP;
    else if (flags & MAP_STACK)
        vma |= VMA_STACK;
    else
        vma |= VMA_ANON;
    if (flags & MAP_POPULATE && !(flags & MAP_SHARED))
        vma |= VM_RESOLVE;

    // TODO - Transform flags !
    void *ptr = mspace_map(__current->vm, (size_t)addr, length, ino, off, vma);
    return ptr != NULL ? ptr : (void *) -1;
}

long sys_munmap(void *addr, size_t length)
{
    return mspace_unmap(__current->vm, (size_t)addr, length);
}


long sys_mprotect(void *addr, size_t length, unsigned flags)
{
    unsigned vma = flags & VM_RWX;
    // TODO - Transform flags !
    return mspace_protect(__current->vm, (size_t)addr, length, vma);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static int copystr(bool access, const char *value, char *buf, size_t len)
{
    if (!access || len < strlen(value) + 1)
        return -1;
    strncpy(buf, value, len);
    return 0;
}

long sys_ginfo(unsigned info, char *buf, size_t len)
{
    if (mspace_check(__current->vm, buf, len, VM_WR) != 0)
        return -1;

    switch (info) {
    case SNFO_ARCH:
        return copystr(true, __ARCH, buf, len);
    // case SNFO_GITH:
    //    return copystr(true, _GITH_, buf, len);
    case SNFO_SNAME:
        return copystr(true, "Kora", buf, len);
    // case SNFO_VERSION:
    //     return copystr(true, _MP_ " Kora core " _VTAG_ "-" _GITH_ " ("_DATE_")", buf, len);
    // case SNFO_RELEASE:
    //     return copystr(true, _VTAG_"-"__ARCH, buf, len);
    case SNFO_OSNAME:
        return copystr(true, "KoraOS", buf, len);
    // case SNFO_USER:
    case SNFO_PWD:
        return vfs_readlink(__current->vfs, __current->vfs->pwd, buf, len, false);
    // case SNFO_USERNAME:
    // case SNFO_USERMAIL:
    // case SNFO_HOSTNAME:
    // case SNFO_DOMAIN:
    default:
        errno = EINVAL;
        return -1;
    }
}

long sys_sinfo(unsigned info, const char *buf, size_t len)
{
    if (mspace_check(__current->vm, buf, len, VM_RD) != 0)
        return -1;

    switch (info) {
    // case SNFO_HOSTNAME:
    // case SNFO_DOMAIN:
    default:
        errno = EINVAL;
        return -1;
    }
}

long sys_syslog(const char *log)
{
    kprintf(KL_USR, "Task.%d] %s\n", __current->pid, log);
    return 0;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

long sys_open(int dirfd, const char *path, int flags, int mode)
{
    if (mspace_check_str(__current->vm, path, 4096) != 0)
        return -1;

    if (dirfd >= 0) {
        fstream_t *strm = stream_get(__current->fset, dirfd);
        if (strm == NULL)
            return -1;
    }

    fsnode_t *node = vfs_search(__current->vfs, path, NULL, false);
    if (node == NULL)
        return -1;

    if (vfs_lookup(node) != 0) {
        errno = ENOENT;
        return -1;
    }

    // TODO - Lookup or create !?
    int flg = 0;
    if (flags & 2)
        flg |= VM_WR;
    else if (flags & 1)
        flg |= VM_WR;
    else
        flg |= VM_RD;

    fstream_t *strm = stream_put(__current->fset, node, flg);
    vfs_close_fsnode(node);
    return strm->fd;
}

long sys_close(int fd)
{
    fstream_t *strm = stream_get(__current->fset, fd);
    if (strm == NULL)
        return -1;
    fsnode_t *file = strm->file;
    stream_remove(__current->fset, strm);
    vfs_close_fsnode(file);
    return 0;
}

long sys_readdir(int fd, char *buf, size_t len)
{
    if (mspace_check(__current->vm, buf, len, VM_WR) != 0)
        return -1;

    fstream_t *strm = stream_get(__current->fset, fd);
    if (strm == NULL)
        return -1;

    if (strm->dir_ctx == NULL) {
        strm->dir_ctx = vfs_opendir(strm->file, NULL);
        strm->position = 0;
        if (strm->dir_ctx == NULL)
            return -1;
    }

    struct dirent *entry = (void *)buf;
    int count = 0;
    while (len >= sizeof(struct dirent)) {
        fsnode_t *file = vfs_readdir(strm->file, strm->dir_ctx);
        if (file == NULL)
            break;

        strncpy(entry->d_name, file->name, 256);
        entry->d_ino = file->ino->no;
        entry->d_off = strm->position;
        entry->d_reclen = sizeof(struct dirent);

        buf += sizeof(struct dirent);
        len -= sizeof(struct dirent);
        vfs_close_fsnode(file);
        ++count;
        ++strm->position;
    }
    return count;
}


long sys_seek(int fd, xoff_t offset, int whence)
{
    fstream_t *strm = stream_get(__current->fset, fd);
    if (strm == NULL)
        return -1;

    if (whence == 0)
        strm->position = vfs_seek(strm->file->ino, offset);
    else if (whence == 1)
        strm->position = vfs_seek(strm->file->ino, strm->position + offset);
    else if (whence == 2)
        strm->position = vfs_seek(strm->file->ino, strm->file->ino->length - offset);
    else {
        errno = EINVAL;
        return -1;
    }

    return strm->position;
}

long sys_read(int fd, char *buf, int len)
{
    if (mspace_check(__current->vm, buf, len, VM_WR) != 0)
        return -1;

    fstream_t *strm = stream_get(__current->fset, fd);
    if (strm == NULL)
        return -1;
    if ((strm->flags & VM_RD) == 0) {
        errno = EPERM;
        return -1;
    }
    int ret = vfs_read(strm->file->ino, buf, len, strm->position, strm->io_flags);
    if (ret >= 0)
        strm->position += ret;
    return ret;
}


long sys_write(int fd, const char *buf, int len)
{
    if (mspace_check(__current->vm, buf, len, VM_RD) != 0)
        return -1;

    fstream_t *strm = stream_get(__current->fset, fd);
    if (strm == NULL)
        return -1;
    if ((strm->flags & VM_WR) == 0) {
        errno = EPERM;
        return -1;
    }
    int ret = vfs_write(strm->file->ino, buf, len, strm->position, strm->io_flags);
    if (ret >= 0)
        strm->position += ret;
    return ret;
}

long sys_access(int dirfd, const char *path, int flags)
{
    if (mspace_check_str(__current->vm, path, 4096) != 0)
        return -1;

    if (dirfd >= 0) {
        fstream_t *strm = stream_get(__current->fset, dirfd);
        if (strm == NULL)
            return -1;
    }

    fsnode_t *node = vfs_search(__current->vfs, path, NULL, true);
    if (node == NULL)
        return -1;

    int ret = vfs_access(node, flags & VM_RWX);
    vfs_close_fsnode(node);
    return ret;
}

long sys_fcntl(int fd, int cmd, void **args)
{
    fstream_t *strm = stream_get(__current->fset, fd);
    if (strm == NULL)
        return -1;

    int ret = vfs_fcntl(strm->file->ino, cmd, args);
    return ret;
}

long sys_fstat(int dirfd, const char *path, struct filemeta *meta, int flags)
{
    if (mspace_check_str(__current->vm, path, 4096) != 0)
        return -1;

    if (dirfd >= 0) {
        fstream_t *strm = stream_get(__current->fset, dirfd);
        if (strm == NULL)
            return -1;
    }

    fsnode_t *node = vfs_search(__current->vfs, path, NULL, true);
    if (node == NULL)
        return -1;

    meta->ino = node->ino->no;
    meta->ftype = node->ino->type;
    meta->size = node->ino->length;
    meta->ctime = node->ino->ctime;
    meta->mtime = node->ino->mtime;
    meta->atime = node->ino->atime;
    meta->btime = node->ino->btime;
    vfs_close_fsnode(node);
    return 0;
}


// #define SYS_WINDOW  18

long sys_pipe(int *fds, int flags)
{
    fsnode_t *node = vfs_mknod(NULL, NULL, 0);
    if (node == NULL)
        return -1;

    fstream_t *strm0 = stream_put(__current->fset, node, flags | VM_RD);
    fds[0] = strm0->fd;

    fstream_t *strm1 = stream_put(__current->fset, node, flags | VM_WR);
    fds[1] = strm1->fd;

    vfs_close_fsnode(node);
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// Signals
// #define SYS_SIGRAISE  15
// #define SYS_SIGACTION  16
// #define SYS_SIGRETURN  17


int sys_xtime(int name, xtime_t *ptime)
{
    *ptime = xtime_read(name);
    return 0;
}

