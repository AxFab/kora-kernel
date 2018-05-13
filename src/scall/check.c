#include <kernel/core.h>
#include <kernel/scall.h>
#include <errno.h>

void usr_check_cstr(const char *str, unsigned len)
{
    // vma_t *vma = mspace_search_vma(kCPU.current->mspace);
    // CHECK POINTER / NULL TERM / VALID UTF-8
}

void usr_check_buf(const char *buf, unsigned len)
{
    // CHECK POINTER AND SIZE!
}

