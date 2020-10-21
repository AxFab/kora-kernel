
#define _Noreturn __attribute__((noreturn))
#define PACK(decl) decl __attribute__((packed))
#define unlikely(c)  c
#define likely(c)  c

#define PAGE_SIZE  4096
#define WORDSIZE 64
#define __ARCH  "aarch64"
#define __LP64
#define __LPx

