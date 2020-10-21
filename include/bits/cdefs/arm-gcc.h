
#define _Noreturn __attribute__((noreturn))
#define PACK(decl) decl __attribute__((packed))
#define unlikely(c)  c
#define likely(c)  c

#define PAGE_SIZE  4096
#define WORDSIZE 32
#define __ARCH  "arm"
#define __ILP32
#define __ILPx

