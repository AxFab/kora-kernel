#include <stdatomic.h>

int __atomic_fetch_add_4(atomic_int *ref, int val, int mode)
{
    ((void)mode);
    asm volatile("lock xaddl %%eax, %2;"
                 :"=a"(val) :"a"(val), "m"(*ref) :"memory");
    return val;
}

int __atomic_fetch_sub_4(atomic_int *ref, int val, int mode)
{
    return __atomic_fetch_add_4(ref, -val, mode);
}


