#define _Noreturn __declspec(noreturn)
#define PACK(decl) __pragma(pack(push,1)) decl __pragma(pack(pop))
#define thread_local __declspec(thread)
#define unlikely(c) c
#define likely(c) c

#if WORDSIZE == 32
#    define __ILP32
#    define __ILPx
#else
#    define __LLP64
#    define __LLPx
#endif

#define __asm_mb
