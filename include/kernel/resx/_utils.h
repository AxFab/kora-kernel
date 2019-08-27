#ifndef __KERNEL_RESX__UTILS_H
#define __KERNEL_RESX__UTILS_H 1

#include <stddef.h>


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
