#ifndef __UNISTD_H
#define __UNISTD_H 1

#include <stddef.h>
#include <sys/types.h>


// #define SEEK_SET 0 /* Seek from beginning of file.  */
// #define SEEK_CUR 1  Seek from current position.
// #define SEEK_END 2 /* Seek from end of file.  */

int open(const char* path, int flags, ...);

int read(int fd, void *buf, int len);
int write(int fd, const void*buf, int len);

int lseek(int fd, off_t off, int whence);
void close(int fd);



static inline void* memrchr(const void* s, int c, size_t n)
{
    const char* p = ((char*)s) + n;
    while (p > (const char *)s) {
        p--;
        if (*p == c)
            return (void *)p;
    }
    return (void*)s;
}

#endif  /* __UNISTD_H */
