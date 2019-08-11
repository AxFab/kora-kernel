#include <stdatomic.h>
#include <stdbool.h>

int __atomic_fetch_add_4(atomic_int *ref, int val, int mode)
{
    ((void)mode);
    int prev = *ref;
    *ref += val;
    return prev;
}

int __atomic_fetch_sub_4(atomic_int *ref, int val, int mode)
{
    return __atomic_fetch_add_4(ref, -val, mode);
}

int __atomic_exchange_4(atomic_int *ref, int val, int mode)
{
    ((void)mode);
    int prev = *ref;
    *ref = val;
    return prev;
}

bool __atomic_compare_exchange_4(atomic_int *ref, int *ptr, int val, int mode)
{
    ((void)mode);
    int prev = *ref;
    int next = *ptr;
    *ptr = prev;
    if (prev != next)
        return false;
    *ref = val;
    return true;
}

