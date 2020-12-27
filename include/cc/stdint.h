
#ifndef __STDINT_H 
#define __STDINT_H 1

#include <bits/cdefs.h>


#define __CHAR16_TYPE__ signed short
#define __CHAR32_TYPE__ signed int
#define __WCHAR_TYPE__ signed int

#define __INT8_TYPE__ signed char
#define __UINT8_TYPE__ unsigned char
#define __INT16_TYPE__ signed short
#define __UINT16_TYPE__ unsigned short
#define __INT32_TYPE__ signed int
#define __UINT32_TYPE__ unsigned int
#define __INT64_TYPE__ signed long long
#define __UINT64_TYPE__ unsigned long long

#define __INT_LEAST8_TYPE__ signed char
#define __UINT_LEAST8_TYPE__ unsigned char
#define __INT_LEAST16_TYPE__ signed int
#define __UINT_LEAST16_TYPE__ unsigned int
#define __INT_LEAST32_TYPE__ signed int
#define __UINT_LEAST32_TYPE__ unsigned int
#define __INT_LEAST64_TYPE__ signed long long
#define __UINT_LEAST64_TYPE__ unsigned long long

#define __INT_FAST8_TYPE__ signed char
#define __UINT_FAST8_TYPE__ unsigned char
#define __INT_FAST16_TYPE__ signed int
#define __UINT_FAST16_TYPE__ unsigned int
#define __INT_FAST32_TYPE__ signed int
#define __UINT_FAST32_TYPE__ unsigned int
#define __INT_FAST64_TYPE__ signed long long
#define __UINT_FAST64_TYPE__ unsigned long long

#define __INTPTR_TYPE__ signed long
#define __UINTPTR_TYPE__ unsigned long
#define __INTMAX_TYPE__ signed long long
#define __UINTMAX_TYPE__ unsigned long long


/* 7.8.1.1 Exact-width integer types */
typedef __INT8_TYPE__ int8_t;
typedef __INT16_TYPE__ int16_t;
typedef __INT32_TYPE__ int32_t;
typedef __INT64_TYPE__ int64_t;
typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;

/* 7.8.1.2 Minimum-width integer types */
typedef __INT_LEAST8_TYPE__ int_least8_t;
typedef __INT_LEAST16_TYPE__ int_least16_t;
typedef __INT_LEAST32_TYPE__ int_least32_t;
typedef __INT_LEAST64_TYPE__ int_least64_t;
typedef __UINT_LEAST8_TYPE__ uint_least8_t;
typedef __UINT_LEAST16_TYPE__ uint_least16_t;
typedef __UINT_LEAST32_TYPE__ uint_least32_t;
typedef __UINT_LEAST64_TYPE__ uint_least64_t;

/* 7.8.1.3 Fastest minimum-width integer types */
typedef __INT_FAST8_TYPE__ int_fast8_t;
typedef __INT_FAST16_TYPE__ int_fast16_t;
typedef __INT_FAST32_TYPE__ int_fast32_t;
typedef __INT_FAST64_TYPE__ int_fast64_t;
typedef __UINT_FAST8_TYPE__ uint_fast8_t;
typedef __UINT_FAST16_TYPE__ uint_fast16_t;
typedef __UINT_FAST32_TYPE__ uint_fast32_t;
typedef __UINT_FAST64_TYPE__ uint_fast64_t;

/* 7.8.1.4 Integer types capable of holding object pointers */
#ifdef __INTPTR_TYPE__
typedef __INTPTR_TYPE__ intptr_t;
#endif
#ifdef __UINTPTR_TYPE__
typedef __UINTPTR_TYPE__ uintptr_t;
#endif

/* 7.8.1.5 Greatest-width integer types */
typedef __INTMAX_TYPE__ intmax_t;
typedef __UINTMAX_TYPE__ uintmax_t;

#if (!defined __cplusplus || __cplusplus >= 201103L \
     || defined __STDC_LIMIT_MACROS)

/* 7.18.2 Limits of specified-width integer types */

#ifdef __INT8_MAX__
# undef INT8_MAX
# define INT8_MAX __INT8_MAX__
# undef INT8_MIN
# define INT8_MIN (-INT8_MAX - 1)
#endif
#ifdef __UINT8_MAX__
# undef UINT8_MAX
# define UINT8_MAX __UINT8_MAX__
#endif
#ifdef __INT16_MAX__
# undef INT16_MAX
# define INT16_MAX __INT16_MAX__
# undef INT16_MIN
# define INT16_MIN (-INT16_MAX - 1)
#endif
#ifdef __UINT16_MAX__
# undef UINT16_MAX
# define UINT16_MAX __UINT16_MAX__
#endif
#ifdef __INT32_MAX__
# undef INT32_MAX
# define INT32_MAX __INT32_MAX__
# undef INT32_MIN
# define INT32_MIN (-INT32_MAX - 1)
#endif
#ifdef __UINT32_MAX__
# undef UINT32_MAX
# define UINT32_MAX __UINT32_MAX__
#endif
#ifdef __INT64_MAX__
# undef INT64_MAX
# define INT64_MAX __INT64_MAX__
# undef INT64_MIN
# define INT64_MIN (-INT64_MAX - 1)
#endif
#ifdef __UINT64_MAX__
# undef UINT64_MAX
# define UINT64_MAX __UINT64_MAX__
#endif

#undef INT_LEAST8_MAX
#define INT_LEAST8_MAX __INT_LEAST8_MAX__
#undef INT_LEAST8_MIN
#define INT_LEAST8_MIN (-INT_LEAST8_MAX - 1)
#undef UINT_LEAST8_MAX
#define UINT_LEAST8_MAX __UINT_LEAST8_MAX__
#undef INT_LEAST16_MAX
#define INT_LEAST16_MAX __INT_LEAST16_MAX__
#undef INT_LEAST16_MIN
#define INT_LEAST16_MIN (-INT_LEAST16_MAX - 1)
#undef UINT_LEAST16_MAX
#define UINT_LEAST16_MAX __UINT_LEAST16_MAX__
#undef INT_LEAST32_MAX
#define INT_LEAST32_MAX __INT_LEAST32_MAX__
#undef INT_LEAST32_MIN
#define INT_LEAST32_MIN (-INT_LEAST32_MAX - 1)
#undef UINT_LEAST32_MAX
#define UINT_LEAST32_MAX __UINT_LEAST32_MAX__
#undef INT_LEAST64_MAX
#define INT_LEAST64_MAX __INT_LEAST64_MAX__
#undef INT_LEAST64_MIN
#define INT_LEAST64_MIN (-INT_LEAST64_MAX - 1)
#undef UINT_LEAST64_MAX
#define UINT_LEAST64_MAX __UINT_LEAST64_MAX__

#undef INT_FAST8_MAX
#define INT_FAST8_MAX __INT_FAST8_MAX__
#undef INT_FAST8_MIN
#define INT_FAST8_MIN (-INT_FAST8_MAX - 1)
#undef UINT_FAST8_MAX
#define UINT_FAST8_MAX __UINT_FAST8_MAX__
#undef INT_FAST16_MAX
#define INT_FAST16_MAX __INT_FAST16_MAX__
#undef INT_FAST16_MIN
#define INT_FAST16_MIN (-INT_FAST16_MAX - 1)
#undef UINT_FAST16_MAX
#define UINT_FAST16_MAX __UINT_FAST16_MAX__
#undef INT_FAST32_MAX
#define INT_FAST32_MAX __INT_FAST32_MAX__
#undef INT_FAST32_MIN
#define INT_FAST32_MIN (-INT_FAST32_MAX - 1)
#undef UINT_FAST32_MAX
#define UINT_FAST32_MAX __UINT_FAST32_MAX__
#undef INT_FAST64_MAX
#define INT_FAST64_MAX __INT_FAST64_MAX__
#undef INT_FAST64_MIN
#define INT_FAST64_MIN (-INT_FAST64_MAX - 1)
#undef UINT_FAST64_MAX
#define UINT_FAST64_MAX __UINT_FAST64_MAX__

#ifdef __INTPTR_MAX__
# undef INTPTR_MAX
# define INTPTR_MAX __INTPTR_MAX__
# undef INTPTR_MIN
# define INTPTR_MIN (-INTPTR_MAX - 1)
#endif
#ifdef __UINTPTR_MAX__
# undef UINTPTR_MAX
# define UINTPTR_MAX __UINTPTR_MAX__
#endif

#undef INTMAX_MAX
#define INTMAX_MAX __INTMAX_MAX__
#undef INTMAX_MIN
#define INTMAX_MIN (-INTMAX_MAX - 1)
#undef UINTMAX_MAX
#define UINTMAX_MAX __UINTMAX_MAX__

/* 7.18.3 Limits of other integer types */

#undef PTRDIFF_MAX
#define PTRDIFF_MAX __PTRDIFF_MAX__
#undef PTRDIFF_MIN
#define PTRDIFF_MIN (-PTRDIFF_MAX - 1)

#undef SIG_ATOMIC_MAX
#define SIG_ATOMIC_MAX __SIG_ATOMIC_MAX__
#undef SIG_ATOMIC_MIN
#define SIG_ATOMIC_MIN __SIG_ATOMIC_MIN__

#undef SIZE_MAX
#define SIZE_MAX __SIZE_MAX__

#undef WCHAR_MAX
#define WCHAR_MAX __WCHAR_MAX__
#undef WCHAR_MIN
#define WCHAR_MIN __WCHAR_MIN__

#undef WINT_MAX
#define WINT_MAX __WINT_MAX__
#undef WINT_MIN
#define WINT_MIN __WINT_MIN__

#endif /* (!defined __cplusplus || __cplusplus >= 201103L
       || defined __STDC_LIMIT_MACROS)  */

#if (!defined __cplusplus || __cplusplus >= 201103L \
     || defined __STDC_CONSTANT_MACROS)

#undef INT8_C
#define INT8_C(c) __INT8_C(c)
#undef INT16_C
#define INT16_C(c) __INT16_C(c)
#undef INT32_C
#define INT32_C(c) __INT32_C(c)
#undef INT64_C
#define INT64_C(c) __INT64_C(c)
#undef UINT8_C
#define UINT8_C(c) __UINT8_C(c)
#undef UINT16_C
#define UINT16_C(c) __UINT16_C(c)
#undef UINT32_C
#define UINT32_C(c) __UINT32_C(c)
#undef UINT64_C
#define UINT64_C(c) __UINT64_C(c)
#undef INTMAX_C
#define INTMAX_C(c) __INTMAX_C(c)
#undef UINTMAX_C
#define UINTMAX_C(c) __UINTMAX_C(c)

#endif /* (!defined __cplusplus || __cplusplus >= 201103L
       || defined __STDC_CONSTANT_MACROS) */

#endif /* __STDINT_H */
