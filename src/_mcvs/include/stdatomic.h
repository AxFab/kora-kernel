#ifndef __STDATOMIC_H
#define __STDATOMIC_H 1

#include <stdbool.h>

typedef volatile int atomic_int;
typedef volatile int atomic_intptr_t;

int __atomic_fetch_add_4(atomic_int *ref, int val, int mode);
int __atomic_fetch_sub_4(atomic_int *ref, int val, int mode);
int __atomic_exchange_4(atomic_int *ref, int val, int mode);

#define ATOMIC_2(n,c,p,v)  __atomic_ ## n ## _ ## c (p, v, 0)
#define ATOMIC_3(n,c,r,p,v)  __atomic_ ## n ## _ ## c (r, p, v, 0)

#define atomic_store(p,v)  (*(p) = (v))
#define atomic_init(p,v)  (*(p) = (v))
#define atomic_load(p)  (*(p))

#define atomic_fetch_add(p,v)  ATOMIC_2(fetch_add, 4, p, v)
#define atomic_fetch_sub(p,v)  ATOMIC_2(fetch_sub, 4, p, v)
#define atomic_exchange(p,v)  ATOMIC_2(exchange, 4, p, v)
#define atomic_compare_exchange(r,p,v)  ATOMIC_3(compare_exchange, 4, r, p, v)


#endif  /* __STDATOMIC_H */

