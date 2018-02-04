#ifndef _KERNEL_TYPES
#define _KERNEL_TYPES 1

#include <kora/stddef.h>
#include <kernel/asm/mmu.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/types.h>
// typedef unsigned long long off_t;


typedef struct inode inode_t;

#if _ADD_STDC_MISS
struct timespec {
    long tv_sec;
    long tv_nsec;
};
typedef unsigned long id_t;
typedef  id_t uid_t;
typedef  id_t gid_t;
#endif

#endif  /* _KERNEL_TYPES */
