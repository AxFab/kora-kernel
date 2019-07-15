#ifndef __KERNEL_UTILS_H
#define __KERNEL_UTILS_H 1

#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include "llist.h"
// bbtree / splock
//

#include <stdlib.h>

typedef struct inode inode_t;

void kprintf(int log, const char *msg, ...);

// void *kalloc(size_t len);
// void kfree(void *ptr);

void *kmap(size_t length, inode_t *ino, off_t offset, int flags);
void kunmap(void *address, size_t length);

uint8_t rand8();
uint16_t rand16();
uint32_t rand32();
uint64_t rand64();


static inline void *kalloc(size_t len)
{
    void *ptr = malloc(len);
    memset(ptr, 0, len);
    return ptr;
}

static inline void kfree(void *ptr)
{
    free(ptr);
}



#endif  /* __KERNEL_UTILS_H */

