#include <bits/atomic.h>

void __lock(atomic_int *lock)
{
    if (*lock < 0)
        return;
    return;
}

void __unlock(atomic_int *lock)
{
    if (*lock < 0)
        return;
    return;
}

