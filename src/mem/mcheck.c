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
int vmsp_check(vmsp_t *vmsp, const void *ptr, size_t len, int flags)
{
    splock_lock(&vmsp->lock);
    vma_t *vma = vmsp_find_area(vmsp, (size_t)ptr);
    if (vma == NULL) {
        splock_unlock(&vmsp->lock);
        errno = EINVAL;
        return -1;
    }

    size_t max = vma->node.value_ + vma->length - (size_t)ptr;
    if (max < len) {
        splock_unlock(&vmsp->lock);
        errno = EINVAL;
        return -1;
    }

    if ((flags & VM_WR) && !(vma->flags & VM_WR)) {
        splock_unlock(&vmsp->lock);
        errno = EPERM;
        return -1;
    }

    splock_unlock(&vmsp->lock);
    return 0;
}

/* - */
int vmsp_check_str(vmsp_t *vmsp, const char *str, size_t max)
{
    splock_lock(&vmsp->lock);
    vma_t *vma = vmsp_find_area(vmsp, (size_t)str);
    if (vma == NULL) {
        splock_unlock(&vmsp->lock);
        errno = EINVAL;
        return -1;
    }

    max = MIN(max, vma->node.value_ + vma->length - (size_t)str);
    splock_unlock(&vmsp->lock);

    if (strnlen(str, max) >= max) {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

/* - */
int vmsp_check_strarray(vmsp_t *vmsp, const char **str)
{
    splock_lock(&vmsp->lock);
    vma_t *vma = vmsp_find_area(vmsp, (size_t)str);
    if (vma == NULL) {
        splock_unlock(&vmsp->lock);
        errno = EINVAL;
        return -1;
    }

    size_t max = vma->node.value_ + vma->length - (size_t)str;
    splock_unlock(&vmsp->lock);

    size_t len = sizeof(char *);
    for (int i = 0; len < max; ++i, len += sizeof(char*)) {
        if (*str == NULL)
            break;
        if (vmsp_check_str(vmsp, *str, 4096) != 0)
            return -1;
    }

    return len < max ? 0 : -1;
}
