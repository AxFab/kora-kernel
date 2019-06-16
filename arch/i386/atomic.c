#include <stdatomic.h>
#include <stdbool.h>

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

bool __atomic_compare_exchange_4(atomic_int *ref, int *ptr, int val, int mode)
{
    ((void)mode);
    int prev = *ptr;
    asm volatile("cmpxchgl  %1, %2;"
                 :"=a"(val) :"r"(val), "m"(*ref), "a"(prev) :"memory");
    *ptr = val;
    return val == prev;
}
