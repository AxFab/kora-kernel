#include <stddef.h>

typedef size_t cpu_state_t[6];

#define IRQ_ON ((void)0)
#define IRQ_OFF ((void)0)

#define KSTACK (PAGE_SIZE)

#define MMU_BMP ((unsigned char*)0)
#define MMU_LG  0xffffff

