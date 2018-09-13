#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/task.h>
#include <kora/hmap.h>
#include <kora/splock.h>

int __errno;
splock_t klog_lock = INIT_SPLOCK;

int *__errno_location()
{
    return &__errno;
}

_Noreturn void __assert_fail(CSTR expr, CSTR file, int line)
{
    kprintf(KLOG_ERR, "Assertion failed :%s - %s:%d\n", expr, file, line);
    exit(-1);
}

void *kalloc(size_t size)
{
    return calloc(size, 1);
}

void kfree(void *ptr)
{
    free(ptr);
}

void kprintf(int lvl, CSTR msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    splock_lock(&klog_lock);
    vprintf(msg, ap);
    splock_unlock(&klog_lock);
    va_end(ap);
}

_Noreturn void kpanic(CSTR msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    splock_lock(&klog_lock);
    vprintf(msg, ap);
    splock_unlock(&klog_lock);
    va_end(ap);
    __assert_fail("Kernel panic!", __FILE__, __LINE__);
}

void kclock(struct timespec *ts)
{
    time64_t ticks = time64(); 
    ts->tv_sec = ticks / _PwNano_;
    ts->tv_nsec = ticks % _PwNano_;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct kSys kSYS;

#ifdef _FAKE_TIME
#if !defined(_WIN32)
time64_t time64()
{
    clock_t ticks = clock();
    if (_PwNano_ > CLOCKS_PER_SEC)
        ticks *= _PwNano_ / CLOCKS_PER_SEC;
    else
        ticks /= CLOCKS_PER_SEC / _PwNano_;
    return ticks;
}
#else
#include <windows.h>
time64_t time64()
{
    const INT64 UNIX_START = 0x019DB1DED53E8000;

	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	
	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	return (li.QuadPart - UNIX_START) * 100;
}
#endif
#endif

int cpu_no()
{
    return 0;
}

void irq_ack() 
{
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

bool krn_mmap_init = false;
HMP_map krn_mmap;

struct vma {
	void *address;
	size_t length;
	off_t offset;
	inode_t *ino;
	int flags;
};

void *kmap(size_t length, inode_t *ino, off_t offset, int flags)
{
	if (!IS_ALIGNED(length, PAGE_SIZE) || !IS_ALIGNED(length, PAGE_SIZE))
		assert("unautorized operation");
	if (!krn_mmap_init) {
		hmp_init(&krn_mmap, 16);
		krn_mmap_init = true;
	}
    void *ptr = _aligned_malloc(length, PAGE_SIZE);
    struct vma *vma = (struct vma*)kalloc(sizeof(struct vma));
    vma->length = length;
    vma->offset = offset;
    vma->ino = ino;
    vma->flags = flags;
    hmp_put(&krn_mmap, (char*)&ptr, sizeof(void*), vma);
    switch (flags & VMA_TYPE) {
    case VMA_FILE:
        assert(ino != NULL);
        vfs_read(ino, ptr, length, offset);
        break;
    default:
        assert(false);
        break;
    }
    return ptr;
}

void kunmap(size_t addr, size_t length)
{
    struct vma *vma = (struct vma*)hmp_get(&krn_mmap, (char*)&addr, sizeof(void*));
    assert(vma != NULL);
    assert(vma->length == length);
    hmp_remove(&krn_mmap, (char*)&addr, sizeof(void*));
    switch (vma->flags & VMA_TYPE) {
    case VMA_FILE:
        assert(vma->ino != NULL);
        if (vma->flags & VMA_WRITE)
            vfs_write(vma->ino, (void*)addr, length, vma->offset);
        break;
    default:
        assert(false);
        break;
    }
    free(vma);
    _aligned_free((void*)addr);
}

page_t mmu_read(page_t addr) { return 0; }
void page_release(page_t addr) {}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


#if _FAKE_TASK
_Noreturn void task_switch(int status, int retcode) 
{ 
    assert(false);
    for(;;);
}
#else

void task_switch(int status, int retcode)
{
    assert(kCPU.irq_semaphore == 1);
    assert(status >= TS_ZOMBIE && status <= TS_READY);
    task_t *task = kCPU.running;
    if (task) {
        // kprintf(-1, "Leaving Task %d\n", task->pid);
        splock_lock(&task->lock);
        task->retcode = retcode;
        if (setjmp(task->state.jbuf) != 0)
            return;
        // kprintf(-1, "Saved Task %d\n", task->pid);

        // TODO Stop task chrono
        if (task->status == TS_ABORTED) {
            if (status == TS_BLOCKED) {
                // TODO - We have advent structure to clean
            }
            status = TS_ZOMBIE;
        }
        if (status == TS_ZOMBIE) {
            /* Quit the task */
            async_raise(&task->wlist, 0);
            // task_zombie(task);
        } else if (status == TS_READY)
            scheduler_add(task);
        task->status = status;
        splock_unlock(&task->lock);
    }

    task = scheduler_next();
    task_t *prev = kCPU.running;
    kCPU.running = task;
    irq_reset(false);
    if (task == NULL)
        cpu_halt();
    // TODO Start task chrono!
    if (task->usmem)
        mmu_context(task->usmem);
    // kprintf(-1, "Start Task %d\n", task->pid)

    longjmp(prev->state.jbuf, 1);
}

#endif
