#ifndef __KERNEL_MMU_H
#define __KERNEL_MMU_H 1

#include <stddef.h>

typedef size_t page_t;

#ifdef _WIN32
// Windows
#include <windows.h>

static inline void *__valloc(size_t len)
{
    return _aligned_malloc(len, PAGE_SIZE);
}

static inline void __vfree(void *ptr, size_t len)
{
    (void)len;
    _aligned_free(ptr);
}


#else
// Linux
#include <sys/mman.h>

static inline void *__valloc(size_t len)
{
    return mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
}

static inline void __vfree(void *ptr, size_t len)
{
    munmap(ptr, len);
}

#endif

#endif  /* __KERNEL_MMU_H */

