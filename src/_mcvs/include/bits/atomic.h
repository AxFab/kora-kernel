#ifndef __BITS_ATOMIC_H
#define __BITS_ATOMIC_H 1

static inline void __atomic_dec_4(atomic_int* ptr)
{
    (*ptr)--;
}


static inline void __atomic_inc_4(atomic_int* ptr)
{
    (*ptr)++;
}

static inline int __atomic_fetch_add_4(atomic_int* ref, int val, int mode)
{
    ((void)mode);
    int prev = *ref;
    *ref += val;
    return prev;
}


static inline int __atomic_fetch_sub_4(atomic_int* ref, int val, int mode)
{
    return __atomic_fetch_add_4(ref, -val, mode);
}


static inline int __atomic_exchange_4(atomic_int* ref, int val, int mode)
{
    ((void)mode);
    int prev = *ref;
    *ref = val;
    return prev;
}


static inline bool __atomic_compare_exchange_4(atomic_int* ref, int* ptr, int val, int mode)
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


#endif /* __BITS_ATOMIC_H */
