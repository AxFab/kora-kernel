#ifndef __BITS_CDEFS_H

#if defined __amd64 || defined __x86_64
#   include <bits/cdefs/x86_64-gcc.h>
#elif defined __arm__
#   include <bits/cdefs/arm-gcc.h>
#elif defined __aarch64__
#   include <bits/cdefs/aarch64-gcc.h>
#elif defined __i386
#   include <bits/cdefs/i386-gcc.h>
#elif defined __ia64__
#   include <bits/cdefs/ia64-gcc.h>
#elif defined _M_ARM
#   include <bits/cdefs/arm-mcvs.h>
#elif defined _M_IX86
#   include <bits/cdefs/i386-mcvs.h>
#elif defined _M_IA64
#   include <bits/cdefs/ia64-mcvs.h>
#else
#   error Unknown compiler
#endif

#endif  /* __BITS_CDEFS_H */

