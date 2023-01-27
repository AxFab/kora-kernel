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

static size_t mspace_find_max_length(mspace_t *mspace, const void *ptr, vma_t **pvma)
{
    vma_t *vma = mspace_search_vma(mspace, (size_t)ptr);
    if (vma == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (vma->mspace == __mmu.kspace) {
        splock_unlock(&vma->mspace->lock);
        errno = EINVAL;
        return -1;
    }

    size_t max = vma->node.value_ + vma->length - (size_t)ptr;
    return max;
}

/* - */
int mspace_check(mspace_t *mspace, const void *ptr, size_t len, int flags)
{
    vma_t *vma;
    size_t max = mspace_find_max_length(mspace, ptr, &vma);
    if (max < len) {
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
    vma_t *vma;
    max = MIN(max, mspace_find_max_length(mspace, str, &vma));
    splock_unlock(&vma->mspace->lock);
    if (strnlen(str, max) >= max) {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

/* - */
int mspace_check_strarray(mspace_t *mspace, const char **str)
{
    vma_t *vma;
    size_t max = mspace_find_max_length(mspace, str, &vma);
    splock_unlock(&vma->mspace->lock);

    size_t len = sizeof(char *);
    for (int i = 0; len < max; ++i, len += sizeof(char*)) {
        if (*str == NULL)
            break;
        if (mspace_check_str(mspace, *str, 4096) != 0)
            return -1;
    }

    return len < max ? 0 : -1;
}
