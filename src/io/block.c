/*
 *      This file is part of the KoraOS project.
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
#include <kernel/core.h>
#include <kernel/drivers.h>
#include <kora/mcrs.h>
#include <string.h>


page_t block_page(inode_t *ino, off_t off)
{
    return 0;
}


int blk_read(inode_t *ino, char *buffer, size_t length, off_t offset)
{
    if (offset >= ino->length)
        return 0;
    off_t poff = -1;
    char *map = NULL;
    int bytes = 0;
    while (length > 0) {
        off_t po = ALIGN_DW(offset, PAGE_SIZE);
        if (poff != po) {
            if (map != NULL)
                kunmap(map, PAGE_SIZE);
            poff = po;
            map = kmap(PAGE_SIZE, ino, poff, VMA_FILE_RO | VMA_RESOLVE);
            if (map == NULL)
                return -1;
        }
        size_t disp = (size_t)(offset & (PAGE_SIZE - 1));
        int cap = MIN(MIN(length, PAGE_SIZE - disp), (size_t)(ino->length - offset));
        if (cap == 0)
            return bytes;
        memcpy(buffer, map + disp, cap);
        length -= cap;
        offset += cap;
        bytes += cap;
    }
    kunmap(map, PAGE_SIZE);
    return bytes;
}


