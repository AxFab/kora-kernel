#ifndef __KERNEL_RESX__UTILS_H
#define __KERNEL_RESX__UTILS_H 1

#include <stddef.h>



#define rcu_open(n)  rcu_open_(n, &(n)->rcu)
#define rcu_close(n,dtor) do { if (rcu_close_(n, &(n)->rcu)) dtor(n); } while(0)

static inline void *rcu_open_(void *ptr, atomic_int *rcu)
{
    atomic_fetch_add(rcu, 1);
    return ptr;
}

static inline bool rcu_close_(void *ptr, atomic_int *rcu)
{
    int val = atomic_fetch_sub(rcu, 1);
    return val == 1;
}

void *kalloc(size_t len);
void kfree(void *ptr);
void *kmap(size_t len, void *ino, size_t off, unsigned flags);
void kunmap(void *addr, size_t len);
void kprintf(int log, const char *msg, ...);
_Noreturn void kpanic(const char *ms, ...);
int kwrite(FILE *fp, const char *buf, size_t len);
clock64_t kclock();

uint8_t rand8();
uint16_t rand16();
uint32_t rand32();
uint64_t rand64();
uint32_t crc32(const void *buf, size_t len);
char *sztoa(size_t number);
char *sztoa_r(size_t number, char *sz_format);
void *memcpy32(void *dest, void *src, size_t lg);
void *memset32(void *dest, uint32_t val, size_t lg);
const char *ksymbol(void *eip, char *buf, int lg);
void kdump(const void *buf, int len);
void stackdump(size_t frame);

int rand(void);
void *malloc(size_t len);
void free(void *ptr);
int snprintf(char *buf, size_t len, const char *msg, ...);
int vprintf(const char *format, va_list ap);
int vfprintf(FILE *fp, const char *msg, va_list ap);
int vsnprintf(char *buf, size_t lg, const char *msg, va_list ap);
unsigned long strtoul(const char *nptr, char **endptr, int base);



#endif  /* __KERNEL_RESX__UTILS_H */
