#ifndef __CRTC_H
#define __CRTC_H 1

#include <stddef.h>
#include <stdarg.h>
#include <bits/cdefs.h>
#include <bits/timespec.h>

void *calloc(size_t len, size_t cnt);
void free();
void *valloc(size_t len);

int printf(const char *msg, ...);
int vprintf(const char *msg, va_list ap);

long long clock_read(int clockid);
void usleep(long usecs);

void clock_gettime(int clockid, struct timespec *time_point);

_Noreturn void abort();



int sched_yield();

#ifndef KORA_KRN
#define __CPU_MASK_TYPE int
#include <pthread.h>
#endif

/*
typedef unsigned long int pthread_t;

int pthread_create(pthread_t *restrict thrd, const void *restrict attr, void *(*func)(void *), void *restrict arg);
int pthread_join(pthread_t thrd, void **ret);
int pthread_equal(pthread_t thrd1, pthread_t thrd2);
_Noreturn void pthread_exit(void *retval);
int pthread_detach(pthread_t thrd);
pthread_t pthread_self(void);
*/

#endif  /* __CRTC_H */
