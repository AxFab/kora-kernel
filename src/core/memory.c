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
#include <kernel/memory.h>
#include <kernel/core.h>
#include <kernel/sys/vma.h>
#include <skc/mcrs.h>
#include <assert.h>
#include <errno.h>

static int memory_check_availability(mspace_t *mspace, size_t address, size_t length)
{
  if (address < mspace->lower_bound || address + length > mspace->upper_bound) {
    return -1;
  }
  vma_t *vma = bbtree_search_le(&mspace->tree, address, vma_t, node);
  if (vma == NULL) {
    vma = bbtree_first(&mspace->tree, vma_t, node);
    if (vma != NULL && address + length > vma->node.value_) {
      return -1;
    }
  } else {
    if (vma->node.value_ + vma->length > address) {
      return -1;
    }
    vma = bbtree_next(&vma->node, vma_t, node);
    if (vma != NULL && address + length > vma->node.value_) {
      return -1;
    }
  }
  return 0;
}

static size_t memory_find_free(mspace_t *mspace, size_t length)
{
  vma_t *next;
  vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
  if (vma == NULL || mspace->lower_bound + length <= vma->node.value_) {
    return mspace->lower_bound;
  }

  for (;;) {
    next = bbtree_next(&vma->node, vma_t, node);
    if (next == NULL || vma->node.value_ + vma->length + length <= next->node.value_) {
      break;
    }
    vma = next;
  }
  if (next == NULL && vma->node.value_ + vma->length + length > mspace->upper_bound) {
    return 0;
  }
  return vma->node.value_ + vma->length;
}

static vma_t *memory_split(mspace_t *mspace, vma_t *area, size_t length)
{
  assert(area->length > length);

  vma_t *vma = (vma_t*)kalloc(sizeof(vma_t));
  vma->node.value_ = area->node.value_ + length;
  vma->length = area->length - length;
  vma->ino = area->ino;
  vma->offset = area->offset + length;
  vma->flags = area->flags;

  area->length = length;
  bbtree_insert(&mspace->tree, &vma->node);
  return vma;
}

static void memory_close(mspace_t *mspace, vma_t *vma)
{
  if ((vma->flags & VMA_SHARED) == 0) {
    page_sweep(vma->node.value_, vma->length, true);
  }
  bbtree_remove(&mspace->tree, vma->node.value_);
  kprintf(-1, "[TRACE] unmmap - <%x, %x>\n", vma->node.value_, vma->length);
  mspace->v_size -= vma->length;
  kfree(vma);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/* Map a memory area inside the provided address space. */
void *memory_map(mspace_t *mspace, size_t address, size_t length, inode_t* ino, off_t offset, int flags)
{
  vma_t *vma;
  assert((address & (PAGE_SIZE - 1)) == 0);
  assert((length & (PAGE_SIZE - 1)) == 0);
  if (length == 0 || length > VMA_CFG_MAXSIZE) {
    errno = EINVAL;
    return NULL;
  }

  // kprintf(-1, "mspace_t { treecount:%d, dir:%x, lower:%x, upper:%x, lock:%d } \n",
  //   mspace->tree.count_, mspace->directory, mspace->lower_bound, mspace->upper_bound, mspace->lock);

  splock_lock(&mspace->lock);

  // If we have an address, check availability
  if (address != 0) {
    if (memory_check_availability(mspace, address, length) != 0) {
      if (flags & VMA_MAP_FIXED) {
        splock_unlock(&mspace->lock);
        errno = ERANGE;
        return NULL;
      }
      address = 0;
    }
  }

  // If we don't have address yet
  if (address == 0) {
    address = memory_find_free(mspace, length);
    if (address == 0) {
      splock_unlock(&mspace->lock);
      errno = ENOMEM;
      return NULL;
    }
  }

  // Create the VMA
  errno = 0;
  vma = (vma_t*)kalloc(sizeof(vma_t));
  vma->node.value_ = address;
  vma->length = length;
  vma->ino = ino;
  vma->offset = offset;
  vma->flags = flags;
  bbtree_insert(&mspace->tree, &vma->node);
  splock_unlock(&mspace->lock);
  mspace->v_size += length;
  kprintf(-1, "[TRACE] mmap - <%x, %x> [%x:%x]\n", address, length, ino, offset);
  return (void*)address;
}

/* Change the flags of a memory area. */
int memory_flag(mspace_t *mspace, size_t address, size_t length, int flags)
{
  vma_t *vma;
  assert((address & (PAGE_SIZE - 1)) == 0);
  assert((length & (PAGE_SIZE - 1)) == 0);
  if (address == 0 || length == 0 || length > VMA_CFG_MAXSIZE) {
    errno = EINVAL;
    return -1;
  }

  if ((flags & VMA_RIGHTS) != flags && flags != VMA_DEAD) {
    errno = EINVAL;
    return -1;
  }

  splock_lock(&mspace->lock);
  while (length != 0) {
    vma = bbtree_search_le(&mspace->tree, address, vma_t, node);
    if (vma == NULL) {
      errno = ENOENT;
      splock_unlock(&mspace->lock);
      return -1;
    }

    if (vma->node.value_ != address) {
      assert(vma->node.value_ < address);
      vma = memory_split(mspace, vma, address - vma->node.value_);
    }

    assert(vma->node.value_ == address);
    if (vma->length > length) {
      memory_split(mspace, vma, length);
    }

    if (vma->flags & VMA_DEAD) {
      splock_unlock(&mspace->lock);
      errno = ENOENT;
      return -1;
    }

    if (flags & VMA_RIGHTS) {
      // Change access right
      if ((flags & ((vma->flags & VMA_RIGHTS) >> 4)) != flags) {
        // Un-autorized!
      }

      vma->flags &= ~VMA_RIGHTS;
      vma->flags |= flags;
    } else if (flags == VMA_DEAD) {
      vma->flags |= flags;
    }

    address += vma->length;
    length -= vma->length;
  }

  splock_unlock(&mspace->lock);
  errno = 0;
  return 0;
}

/* Remove disabled memory area */
int memory_scavenge(mspace_t *mspace)
{
  vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
  while (vma) {
    vma_t *next = bbtree_next(&vma->node, vma_t, node);
    if (vma->flags & VMA_DEAD) {
      memory_close(mspace, vma);
    }
    vma = next;
  }
  return -1;
}

/* Display the state of the current address space */
void memory_display(int log, mspace_t *mspace)
{
  const char *rights[] = {
    " ---", " --x", " -w-", " -wx",
    " r--", " r-x", " rw-", " rwx",
    "S---", "S--x", "S-w-", "S-wx",
    "Sr--", "Sr-x", "Srw-", "Srwx",
  };
  int idx;
  splock_lock(&mspace->lock);
  vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
  for (idx = 0; vma; ++idx) {
    kprintf(log, "%d] %08x - %08x   %s   %s  %6x\n", idx,
      vma->node.value_, vma->node.value_ + vma->length,
      sztoa(vma->length), rights[vma->flags & 15], vma->flags);
    vma = bbtree_next(&vma->node, vma_t, node);
  }
  splock_unlock(&mspace->lock);
}

/* Create a memory space for a user application */
mspace_t *memory_userspace()
{
  mspace_t *mspace = (mspace_t*)kalloc(sizeof(mspace_t));
  bbtree_init(&mspace->tree);
  splock_init(&mspace->lock);
  mspace->lower_bound = kMMU.uspace_lower_bound;
  mspace->upper_bound = kMMU.uspace_upper_bound;
  mspace->directory = mmu_directory();
  return mspace;
}

void memory_sweep(mspace_t *mspace)
{
  splock_lock(&mspace->lock);
  vma_t *vma = bbtree_first(&mspace->tree, vma_t, node);
  while (vma != NULL) {
    memory_close(mspace, vma);
    vma = bbtree_first(&mspace->tree, vma_t, node);
  }

  mmu_release_dir(mspace->directory);
  splock_unlock(&mspace->lock);
  kfree(mspace);
}

