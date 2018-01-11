/*
 *      This file is part of the SmokeOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#include <kernel/vfs.h>
#include <kernel/core.h>
#include <kernel/sys/inode.h>
#include <skc/mcrs.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

// void vfs_dirty(inode_t *ino, off_t offset, size_t length)
// {
//   // Just mark as dirty
// }

#if 0
static int vfs_read_blk(inode_t *ino, char *buf, int size, off_t offset)
{
  if (offset + size > ino->length) {
    errno = ERANGE;
    return -1;
  }

  int block = MAX(PAGE_SIZE, ino->block);
  while (size) {
    off_t file_offset = ALIGN_DW(offset, block);
    int pen = offset - file_offset;
    char *ptr = (char*)kmap(block, ino, file_offset, VMA_READ | VMA_FILE);
    if (ino->ops->read(ino, ptr, block, file_offset)) {
      return -1;
    }
    int cap = MAX(size, block - pen);
    assert(cap != 0);
    memcpy(buf, ptr + pen, cap);
    size -= cap;
    offset += cap;
  }

  errno = 0;
  return 0;
}

static int vfs_write_blk(inode_t *ino, const char *buf, int size, off_t offset)
{
  if (offset + size > ino->length) {
    if (ino->ops->truncate(ino, offset + size)) {
      return -1;
    }
  }

  int block = MAX(PAGE_SIZE, ino->block);
  while (size) {
    off_t file_offset = ALIGN_DW(offset, block);
    int pen = offset - file_offset;
    char *ptr = (char*)kmap(block, ino, file_offset, VMA_READ | VMA_WRITE | VMA_FILE);
    if (pen != 0 || size < block) {
      if (ino->ops->read(ino, ptr, block, file_offset)) {
        return -1;
      }
    }
    int cap = MAX(size, block - pen);
    assert(cap != 0);
    memcpy(ptr + pen, buf, cap);
    size -= cap;
    offset += cap;

    // if (flags & O_SYNC) {
    if (ino->ops->write(ino, ptr, block, file_offset)) {
      return -1;
    }
    // } else {
    //   vfs_dirty(ino, file_offset, block);
    // }
  }

  errno = 0;
  return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static int vfs_read_chr(inode_t *ino, char *buf, int size)
{
  int block = ino->block;
  if (size % block) {
    errno = EINVAL;
    return -1;
  }

  while (size) {
    int cap = size;
    if (cap == 0) {
      // BLOCK THE TASK
      continue;
    }

    if (ino->ops->read(ino, buf, cap, 0)) {
      return -1;
    }
    size -= cap;
  }

  errno = 0;
  return 0;
}

static int vfs_write_chr(inode_t *ino, const char *buf, int size)
{
  int block = ino->block;
  if (size % block) {
    errno = EINVAL;
    return -1;
  }

  while (size) {
    int cap = size;
    if (cap == 0) {
      // BLOCK THE TASK
      continue;
    }

    if (ino->ops->write(ino, buf, cap, 0)) {
      return -1;
    }
    size -= cap;
  }

  errno = 0;
  return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int vfs_read(inode_t *ino, char *buf, size_t size, off_t offset)
{
  switch (ino->mode & S_IFSOCK) {
    case S_IFREG:
    case S_IFBLK:
      return vfs_read_blk(ino, buf, size, offset);
    case S_IFCHR:
      return vfs_read_chr(ino, buf, size);
    default:
      errno = ENOSYS;
      return -1;
  }
}

int vfs_write(inode_t *ino, const char *buf, size_t size, off_t offset)
{
  switch (ino->mode & S_IFSOCK) {
    case S_IFREG:
    case S_IFBLK:
      return vfs_write_blk(ino, buf, size, offset);
    case S_IFCHR:
      return vfs_write_chr(ino, buf, size);
    default:
      errno = ENOSYS;
      return -1;
  }
}
#endif
