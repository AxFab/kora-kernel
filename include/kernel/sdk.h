#ifndef _KERNEL_SDK_H
#define _KERNEL_SDK_H 1

#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/device.h>
#include <kernel/files.h>
#include <kernel/memory.h>
#include <kernel/task.h>
#include <kernel/net.h>
#include <kernel/dlib.h>
#include <kernel/syscalls.h>
#include <string.h>
#include <time.h>
#include <errno.h>


#define KAPI(n) { &(n), 0, #n }
struct kdk_api {
    void *address;
    size_t length;
    const char *name;
};

extern proc_t kproc;

void kmod_symbol(CSTR name, void *ptr);
void kmod_symbols(kdk_api_t* kapi);


#endif /* _KERNEL_SDK_H */
