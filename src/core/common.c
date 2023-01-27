/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#include <kernel/stdc.h>
#include <kernel/memory.h>
#include <kernel/tasks.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

_Noreturn void __assert_fail(const char *expr, const char *file, unsigned line, const char *func)
{
    kprintf(KL_ERR, "Assertion failed CPU%d (%s) at %s:%d -- %s\n", cpu_no(), expr, file, line);
    mspace_display(NULL);
    stackdump(12);
    for (;;);
}


int __errno = 0;

int *__errno_location()
{
    if (__current != NULL)
        return &__current->err_no;
    return &__errno;
}

EXPORT_SYMBOL(__errno_location, 0);
EXPORT_SYMBOL(__assert_fail, 0);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void *kmap(size_t length, void *ino, xoff_t offset, int flags)
{
    length = ALIGN_UP(length, PAGE_SIZE);
    int type = flags & VMA_TYPE;
    if (type == 0)
        flags |= (ino != NULL ? VMA_FILE : VMA_ANON);
    void *ptr = mspace_map(__mmu.kspace, 0, length, (inode_t *)ino, offset, flags);
    if (ptr == NULL) {
        kprintf(KL_ERR, "Unable to map memory on the kernel\n");
        for (;;);
    }
    return ptr;
}

void kunmap(void *address, size_t length)
{
    mspace_unmap(__mmu.kspace, (size_t)address, length);
}

EXPORT_SYMBOL(kmap, 0);
EXPORT_SYMBOL(kunmap, 0);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
void *malloc(size_t size);
void free(void *ptr);

void *kalloc_(size_t size, const char *expr)
{
    return kalloc(size);
}

void *kalloc(size_t size)
{
    if (size > PAGE_SIZE) {
        kprintf(-1, "Page is size %x\n", size);
        stackdump(12);
    }
    assert(size <= PAGE_SIZE);
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void kfree(void *ptr)
{
    size_t size = 8; // TODO szofalloc(ptr);
    memset(ptr, 0, size);
    free(ptr);
}

#undef kstrdup
#undef kstrndup

char *kstrdup(const char *str)
{
    int len = strlen(str) + 1;
    void *ptr = kalloc(len);
    memcpy(ptr, str, len);
    return ptr;
}

char *kstrndup(const char *str, size_t len)
{
    len = strnlen(str, len) + 1;
    void *ptr = kalloc(len);
    memcpy(ptr, str, len);
    return ptr;
}

EXPORT_SYMBOL(kalloc_, 0);
EXPORT_SYMBOL(kalloc, 0);
EXPORT_SYMBOL(kfree, 0);
EXPORT_SYMBOL(kstrdup, 0);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

splock_t kplock = INIT_SPLOCK;
char kpbuf[1024];
char kpbuf2[1024];
#define KL_NONE  ((char *)0xAC)

// Change kernel log settings
char *klog_lvl[KL_MAX] = {
    [KL_FSA] = KL_NONE, // "\033[35m%s\033[0m",
    [KL_BIO] = KL_NONE,
};


void kprintf(klog_t log, const char *msg, ...)
{
    char *pk = log >= 0 && log < KL_MAX ? klog_lvl[log] : NULL;
    if (pk == KL_NONE)
        return;
    splock_lock(&kplock);
    va_list ap;
    va_start(ap, msg);
    int lg = vsnprintf(kpbuf, 1024, msg, ap);
    va_end(ap);
    if (pk != NULL) {
        lg = snprintf(kpbuf2, 1024, pk, kpbuf);
        kwrite(kpbuf2);
    } else {
        kwrite(kpbuf);
    }
    splock_unlock(&kplock);
}
EXPORT_SYMBOL(kprintf, 0);


xtime_t xtime_read(xtime_name_t name)
{
    return clock_read(&__clock, name);
}
EXPORT_SYMBOL(xtime_read, 0);


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

unsigned __rseed = 0;

unsigned rand_r(unsigned *);

uint8_t rand8()
{
    return rand_r(&__rseed) & 0xff;
}

uint16_t rand16()
{
    return (rand_r(&__rseed) & 0xff) |
           ((rand_r(&__rseed) & 0xff) << 8);
}

uint32_t rand32()
{
    return (rand_r(&__rseed) & 0xff) |
           ((rand_r(&__rseed) & 0xff) << 8) |
           ((rand_r(&__rseed) & 0xff) << 16) |
           ((rand_r(&__rseed) & 0xff) << 24);
}

EXPORT_SYMBOL(rand8, 0);
EXPORT_SYMBOL(rand16, 0);
EXPORT_SYMBOL(rand32, 0);