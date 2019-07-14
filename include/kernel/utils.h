#ifndef __KERNEL_UTILS_H
#define __KERNEL_UTILS_H 1

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <kora/llist.h>
#include <kora/bbtree.h>
#include <kora/splock.h>

/* Format string and write on syslog. */
void kprintf(int log, const char *msg, ...);
/* Allocate a block of memory and initialize it to zero */
void *kalloc(size_t len);
/* Free a block of memory previously allocated by `kalloc' */
void kfree(void *ptr);
/* Map a area into kernel memory */
void *kmap(size_t length, inode_t *ino, off_t offset, int flags);
/* Unmap a area into kernel memory */
void kunmap(void *address, size_t length);


uint8_t rand8();
uint16_t rand16();
uint32_t rand32();
uint64_t rand64();



typedef struct inode inode_t;

#endif  /* __KERNEL_UTILS_H */
