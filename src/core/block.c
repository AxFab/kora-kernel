#include <kernel/core.h>
#include <kernel/sys/inode.h>
#include <kora/mcrs.h>
#include <string.h>


int blk_read(inode_t *ino, char *buffer, int length, off_t offset)
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
            map = kmap(PAGE_SIZE, ino, poff, VMA_FG_RO_FILE);
            if (map == NULL)
                return -1;
        }
        int disp = offset & (PAGE_SIZE - 1);
        int cap = MIN(MIN(length, PAGE_SIZE - disp), ino->length - offset);
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


