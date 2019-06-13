/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#include <kernel/syscalls.h>
#include <kernel/memory.h>
#include <kernel/vfs.h>
#include <kernel/files.h>
#include <kernel/device.h>
#include <kernel/net.h>
#include <kernel/task.h>
#include <threads.h>
#include <string.h>
#include <errno.h>

int scall_check_str(CSTR str)
{
    return 0;
}

int scall_check_buf(const void *buf, int len)
{
    return 0;
}


/* --------
  Tasks, Process & Sessions
--------- */

// /* Prepare system shutdown, sleep or partial shutdown (kill session) */
// long sys_power(unsigned type, long delay)
// {
//  // TODO
// }

/* Kill a thread */
long sys_stop(unsigned tid, int status)
{
    task_t *task = kCPU.running;
    if (tid != 0) {
        task = task_search(tid);
        if (task == NULL) {
            errno = EINVAL;
            return -1;
        }
        // TODO - check rights
        errno = EPERM;
        return -1;
    }

    return task_stop(task, status);
}

/* Kill all the thread of the current process */
long sys_exit(int status)
{
    // TODO -- All threads!
    return sys_stop(0, status);
}

// /* Start a thread on a new session */
// long sys_start(unsigned uid, int exec, int in, int out, CSTR command)
// {
//  // TODO
// }

// /* Fork the current task and copy some attribute */
// long sys_fork(int clone)
// {
//  // TODO
// }

long sys_sleep(long timeout)
{
    return async_wait(NULL, NULL, timeout);
}

// long sys_wait(int what, unsigned id, long timeout)
// {
//     return async_wait(NULL, NULL, timeout);
// }

// // exec ve


// /* --------
//   Input & Output
// --------- */

long sys_read(int fd, char *buf, int len)
{
    if (scall_check_buf(buf, len))
        return -1;
    resx_t *resx = kCPU.running->resx;
    stream_t *stream = resx_get(resx, fd);
    if (stream == NULL) {
        errno = EBADF;
        return -1;
        // } else if ((stream->flags & R_OK) == 0) {
        //     errno = EACCES;
        //     return -1;
    }
    // mtx_lock(&stream->lock) ;
    int ret = vfs_read(stream->ino, buf, len, stream->off, stream->flags);
    if (ret >= 0) {
        errno = 0;
        stream->off += ret;
    }
    // mtx_unlock(&stream->lock) ;
    return ret;
}

long sys_write(int fd, const char *buf, int len)
{
    if (scall_check_buf(buf, len))
        return -1;
    if (fd == 1)
        kprintf(-1, "USR #%d.1 - %s\n", kCPU.running->pid, buf);
    else if (fd == 2)
        kprintf(-1, "USR #%d.2 - %s\n", kCPU.running->pid, buf);
    resx_t *resx = kCPU.running->resx;
    stream_t *stream = resx_get(resx, fd);
    if (stream == NULL) {
        errno = EBADF;
        return -1;
        // } else if ((stream->flags & W_OK) == 0) {
        //     errno = EACCES;
        //     return -1;
    }
    // mtx_lock(&stream->lock) ;
    int ret = vfs_write(stream->ino, buf, len, stream->off, stream->flags);
    if (ret >= 0) {
        errno = 0;
        stream->off += ret;
    }
    // mtx_unlock(&stream->lock) ;
    return ret;
}


long sys_access(int fd, CSTR path, int flags)
{
    if (scall_check_str(path))
        return -1;
    resx_t *resx = kCPU.running->resx;
    inode_t *dir = resx_fs_pwd(kCPU.running->resx_fs);
    if (fd != -1) {
        stream_t *stream = resx_get(resx, fd);
        if (stream == NULL) {
            errno = EBADF;
            return -1;
        }
        vfs_close(dir);
        dir = vfs_open(stream->ino);
    }
    inode_t *root = resx_fs_root(kCPU.running->resx_fs);
    inode_t *ino = vfs_search(root, dir, path, NULL);
    vfs_close(root);
    vfs_close(dir);
    if (ino == NULL) {
        assert(errno != 0);
        return -1;
    }
    errno = 0;
    vfs_close(ino);
    return 0;
}

long sys_open(int fd, CSTR path, int flags)
{
    if (scall_check_str(path))
        return -1;
    resx_t *resx = kCPU.running->resx;
    inode_t *dir = resx_fs_pwd(kCPU.running->resx_fs);
    if (fd != -1) {
        stream_t *stream = resx_get(resx, fd);
        if (stream == NULL) {
            errno = EBADF;
            return -1;
        }
        vfs_close(dir);
        dir = vfs_open(stream->ino);
    }
    inode_t *root = resx_fs_root(kCPU.running->resx_fs);
    inode_t *ino = vfs_search(root, dir, path, NULL);
    vfs_close(root);
    vfs_close(dir);
    if (ino == NULL) {
        assert(errno != 0);
        return -1;
    }
    errno = 0;
    // TODO -- truncate, append!
    stream_t *stream = resx_set(resx, ino);
    vfs_close(ino);
    if (stream == NULL)
        return 0;
    return stream->node.value_;
}

long sys_close(int fd)
{
    resx_t *resx = kCPU.running->resx;
    return resx_rm(resx, fd);
}

// lseek
// sync
// umask

// access
// ioctl
// fcntl

/* --------
  File system
--------- */

// stat (at)
// chmod
// chown
// utimes

// link
// unlink!
// rename
// mkdir
// rmdir

// dup

int sys_pipe(int *fds)
{
    if (scall_check_buf(fds, 2 * sizeof(int)))
        return -1;
    resx_t *resx = kCPU.running->resx;
    inode_t *ino = pipe_inode();
    if (ino == NULL)
        return -1;
    stream_t *sout = resx_set(resx, ino);
    sout->flags = W_OK;
    stream_t *sin = resx_set(resx, ino);
    sin->flags = R_OK;
    fds[0] = sout->node.value_;
    fds[1] = sout->node.value_;
    errno = 0;
    return 0;
}


int sys_window(int ctx, int width, int height, unsigned flags)
{
    // TODO - Look for the desktop attached to the session
    resx_t *resx = kCPU.running->resx;
    inode_t *ino = wmgr_create_window(NULL, width, height);
    if (ino == NULL)
        return -1;

    stream_t *stream = resx_set(resx, ino);
    errno = 0;
    return stream->node.value_;
}

int sys_fcntl(int fd, int cmd, void *args)
{
    resx_t *resx = kCPU.running->resx;
    stream_t *stream = resx_get(resx, fd);
    if (stream == NULL) {
        errno = EBADF;
        return -1;
    }

    // mtx_lock(&stream->lock) ;
    if (stream->ino->ops->fcntl == NULL) {
        errno = ENOSYS;
        mtx_unlock(&stream->lock);
        return -1;
    }

    int ret = stream->ino->ops->fcntl(stream->ino, cmd, args);
    if (ret >= 0)
        errno = 0;
    // mtx_unlock(&stream->lock);
    return ret;
}

int sys_socket(int protocol, const char *address, int port)
{
    inode_t *ino = NULL;
    resx_t *resx = kCPU.running->resx;
    if (scall_check_buf(address, IP4_ALEN))
        return -1;

    errno = EINVAL;
    if (ino == NULL)
        return -1;
    stream_t *stream = resx_set(resx, ino);
    return stream->node.value_;
}

// socket

/* --------
  Memory
--------- */

void *sys_mmap(void *addr, size_t length, unsigned flags, int fd, off_t off)
{
    mspace_t *mspace = kCPU.running->usmem;
    if (mspace == NULL) {
        errno = EACCES;
        return (void *) - 1;
    }

    inode_t *ino = NULL;
    resx_t *resx = kCPU.running->resx;
    int vma = (flags & 7) | ((flags & 7) << 4);
    if (fd >= 0) {
        stream_t *stream = resx_get(resx, fd);
        if (stream == NULL) {
            errno = EBADF;
            return (void*)-1;
            // } else if ((stream->flags & R_OK) == 0) {
            //     errno = EACCES;
            //     return -1;
        }
        ino = stream->ino;
        vma |= VMA_FILE;
    } else {
        // TODO - flags
        vma |= VMA_HEAP;
    }

    if (flags & 0x10000)
        vma |= VMA_RESOLVE;

    return mspace_map(mspace, (size_t)addr, length, ino, off, vma);
}

long sys_munmap(void *addr, size_t length)
{
    mspace_t *mspace = kCPU.running->usmem;
    if (mspace == NULL) {
        errno = EACCES;
        return -1;
    }
    return mspace_unmap(mspace, (size_t)addr, length);
}

// long sys_mprotect(void *address, size_t length, unsigned flags)
// {
//  mspace_t *mspace = kCPU.running->uspace;
//  if (mspace == NULL) {
//      errno = EACCESS;
//      return -1;
//  }
//  // TODO - flags
//  return mspace_protect(address, length,flags);
// }

// /* --------
//   Signals
// --------- */

// // kill
// // sigaction
// // sigmask


// /* --------
//   System
// --------- */

static long ginfo(bool expr, CSTR info, void *buf, int len)
{
    if (!expr)  {
        errno = EPERM;
        return -1;
    } else if (len <= (int)strlen(info)) {
        errno = EAGAIN;
        return -1;
    }
    strcpy((char *) buf, info);
    errno = 0;
    return 0;
}

static long sinfo(bool expr, char **info, const void *buf, int len)
{
    if (!expr)  {
        errno = EPERM;
        return -1;
    } else if (len <= (int)strnlen((char *) buf, len)) {
        errno = EINVAL;
        return -1;
    }
    kfree(*info) ;
    *info = strdup((char *) buf);
    errno = 0;
    return 0;
}

long sys_ginfo(unsigned info, void *buf, int len)
{
    int ret;
    if (scall_check_buf(buf, len))
        return -1;
    switch (info) {
    case SNFO_ARCH:
        return ginfo(true, __ARCH, buf, len);
    case SNFO_GITH:
        return ginfo(true, _GITH_, buf, len);
    case SNFO_SNAME:
        return ginfo(true, "Kora", buf, len);
    case SNFO_VERSION:
        return ginfo(true, _MP_ " Kora core " _VTAG_ "-" _GITH_ " ("_DATE_")", buf, len);
    case SNFO_RELEASE:
        return ginfo(true, _VTAG_"-"__ARCH, buf, len);
    case SNFO_OSNAME:
        return ginfo(true, "KoraOS", buf, len);

    case SNFO_USER:
        return ginfo(true, "Visitor", buf, len);
    case SNFO_PWD:
        return ginfo(true, "/", buf, len);
        // return ginfo(true, kCPU.running->user->login, buf, len);
    // case SNFO_USERNAME:
    //     return ginfo(true, kCPU.running->user->username, buf, len);
    // case SNFO_USERMAIL:
    //     return ginfo(true, kCPU.running->user->email, buf, len);
    case SNFO_HOSTNAME:
        rwlock_rdlock(&kSYS.lock) ;
        ret = ginfo(true, kSYS.hostname, buf, len);
        rwlock_rdunlock(&kSYS.lock) ;
        return ret;
    case SNFO_DOMAIN:
        rwlock_rdlock(&kSYS.lock) ;
        ret = ginfo(true, kSYS.domain, buf, len);
        rwlock_rdunlock(&kSYS.lock) ;
        return ret;
    default:
        errno = EINVAL;
        return -1;
    }
}

long sys_sinfo(unsigned info, const void *buf, int len)
{
    int ret;
    if (scall_check_buf(buf, len))
        return -1;
    switch (info) {
    case SNFO_HOSTNAME:
        rwlock_wrlock(&kSYS.lock) ;
        ret = sinfo(true, &kSYS.hostname, buf, len);
        rwlock_wrunlock(&kSYS.lock) ;
        return ret;
    case SNFO_DOMAIN:
        rwlock_wrlock(&kSYS.lock) ;
        ret = sinfo(true, &kSYS.domain, buf, len);
        rwlock_wrunlock(&kSYS.lock) ;
        return ret;
    default:
        errno = EINVAL;
        return -1;
    }
}

// long sys_log(CSTR msg)
// {
//  if (scall_check_str(msg))
//      return -1;
//  if (strchr(msg, '\n') || strchr(msg, '\033'))
//      return -1;
//  kprintf(KLOG_MSG, "[T.%d] %s\n", kCPU.running->tid, msg);
//  return 0;
// }

// long sys_sysctl(int cmd, void* args)
// {
//  switch (info) {
//  default:
//      errno = EINVAL;
//      return -1;
//  }
//  errno = 0;
//  return 0;
// }


// long sys_copy(int out, int in, size_t size, int count)
// {
//  resx_pool_t *pool = kCPU.running->resxs;
//  resx_t *rxout = resx_get(pool, out);
//  resx_t *rxin = resx_get(pool, in);
//  if (rxout == NULL || rxin == NULL) {
//      errno = EBADF;
//      return -1;
//  }

//  size_t len = ALIGN_UP(size, PAGE_SIZE) ;
//  void *buf = kmap(len, NULL, 0, VMA_ANON_RW | VMA_RESOLVE);
//  int blocks = 0;
//  while (count-- > 0) {
//      if (vfs_read(rxin->ino, buf, size, rxin->off, rxin->flags))
//          return blocks;
//      if (vfs_write(rxout->ino, buf, size, rxout->off, rxout->flags))
//          return blocks;
//      blocks++;
//  }
//     // TODO - OFF set, align !?
//     errno = 0;
//     return blocks;
// }
