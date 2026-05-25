

#ifndef _ASSERT_H
#define _ASSERT_H 1

#include <bits/cdefs.h>

#ifdef NDEBUG
# define assert(EX)
#else
# define assert(EX) (void)((EX) || (__assert_fail(#EX, __FILE__, __LINE__, __func__),0))
#endif

_Noreturn void __assert_fail(const char *expr, const char *file, unsigned line, const char *func);

#endif /* _ASSERT_H */
