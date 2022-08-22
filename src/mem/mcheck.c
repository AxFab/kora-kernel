/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#include <errno.h>

/* - */
int mspace_check(mspace_t *mspace, const void *ptr, size_t len, int flags)
{
    vma_t *vma = mspace_search_vma(mspace, (size_t)ptr);
    if (vma == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (vma->mspace == kMMU.kspace) {
        splock_unlock(&vma->mspace->lock);
        errno = EINVAL;
        return -1;
    }

    if (vma->node.value_ + vma->length <= (size_t)ptr + len) {
        splock_unlock(&vma->mspace->lock);
        errno = EINVAL;
        return -1;
    }

    if ((flags & VM_WR) && !(vma->flags & VM_WR)) {
        splock_unlock(&vma->mspace->lock);
        errno = EINVAL;
        return -1;
    }

    splock_unlock(&vma->mspace->lock);
    return 0;
}

/* - */
int mspace_check_str(mspace_t *mspace, const char *str, size_t max)
{
    vma_t *vma = mspace_search_vma(mspace, (size_t)str);
    if (vma == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (vma->mspace == kMMU.kspace) {
        splock_unlock(&vma->mspace->lock);
        errno = EINVAL;
        return -1;
    }

    max = MIN(max, vma->node.value_ + vma->length - (size_t)str);
    if (strnlen(str, max) >= max) {
        splock_unlock(&vma->mspace->lock);
        errno = EINVAL;
        return -1;
    }

    splock_unlock(&vma->mspace->lock);
    return 0;
}

