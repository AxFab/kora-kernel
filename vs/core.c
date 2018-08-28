#include <kernel/core.h>

int _errno;

int *__errno_location()
{
    return &_errno;
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
    va_list ap
    va_start(ap, msg);
    splock_lock(&klog_lock);
    vprintf(msg, ap);
    splock_unlock(&klog_lock);
    va_end(ap);
}

void kclock(struct timespec *ts)
{
    time64_t ticks = time64(); 
    ts->tv_seconds = ticks / _PwNano_;
    ts&>tv_nanosecs = ticks % _PwNano_; 
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

time64_t time64()
{
	clock_t ticks = clock();
    if (_PwNano_ > SEC_PER_CLOCK)
        ticks *= _PwNano_ / SEC_PER_CLOCK;
    else
        ticks /= SEC_PER_CLOCK / _PwNano_;
    return ticks;
}

int cpu_no()
{
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

irq_enable
irq_disable
irq_reset

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void *kmap(size_t length, inode_t *ino, off_t off, int flags)
{
    void *ptr = _aligned_malloc(length, PAGE_SIZE);
    struct vma *vma = (struct vma*)kalloc(sizeof(struct vma));
    vma->length = length;
    vma->offset = offset;
    vma->ino = ino;
    hmp_put(&krn_mmap, &ptr, sizeof(void*), vma);
    if ((flags & VMA_TYPE) == VMA_FILE) {
        assert(ino != NULL);
        vfs_read(ino, ptr, length, offset);
    }
    return ptr;
}

void kunmap(size_t addr, size_t length)
{
    struct vma *vma = (struct vma*)hmp_get(&krn_mmap, &addr, sizeof(void*));
    assert(vma != NULL);
    assert(vma->length == length);
    hmp_remove(&krn_mmap, &addr, sizeof(void*));
    if ((vma->flags & VMA_TYPE) == VMA_FILE) {
        assert(vma->ino != NULL);
        vfs_weite(vma->ino, addr, length, vma->offset);
    }
    free(vma);
    _aligned_free(addr);
}

page_t mmu_read() { return 0; }
void page_release(page_t addr) {}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

_Noreturn void task_switch(int status, int retcode)
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
    // kprintf(-1, "Start Task %d\n", task->pid);
    //
    mtx_unlock(task->state.mtx);
    if (!prev || status == TS_ZOMBIE)
        terminate_thread();
    if (state != TS_READY) // set to sleep!
        mtx_wait(prev->state.mtx);
    longjmp(prev->state.jbuf, 1);
}


int task_resume(task_t *task)
{
    splock_lock(&task->lock);
    if (task->status <= TS_ZOMBIE || task->status >= TS_READY) {
        splock_unlock(&task->lock);
        return -1;
    }

    task->status = TS_READY;
    // TODO mutex unlock scheduler_add(task);
    splock_unlock(&task->lock);
    return 0;
}


