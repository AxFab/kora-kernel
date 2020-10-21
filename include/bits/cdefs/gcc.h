#define _Noreturn __attribute__((noreturn))
#define PACK(decl) decl __attribute__((packed))
#define thread_local __thread
#define unlikely(c)  c
#define likely(c)  c

#if WORDSIZE == 32
#    define __ILP32
#    define __ILPx
#else
#    define __LP64
#    define __LPx
#endif

#define __asm_irq_on_  asm("sti")
#define __asm_irq_off_  asm("cli")
#define  __asm_pause_  asm("pause")

// Memory barriers
#define __asm_rmb       asm("")
#define __asm_wmb       asm("")
#define __asm_mb       asm("")

