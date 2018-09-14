#include <kernel/core.h>

void mmu_enable() {}
void mmu_leave() {}
page_t mmu_read(size_t addr) { return 0; }
page_t mmu_drop(size_t addr) { return 0; }
page_t mmu_protect(size_t addr, int flags) { return 0; }
page_t mmu_resolve(size_t addr, page_t phys, int flags) { return 0; }
void mmu_context(mspace_t *mspace) {}
void mmu_create_uspace(mspace_t *mspace) {}
void mmu_destroy_uspace(mspace_t *mspace) {}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void cpu_halt() {}
time64_t cpu_clock() { return 0; }
time_t cpu_time() { return 0; }
void cpu_setup() 
{
    kSYS.cpus = kalloc(sizeof(struct kCpu) * 32);
    kSYS.cpus[0].irq_semaphore = 1;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void platform_setup()
{

}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#include <kernel/task.h>
#include <kernel/mmu.h>
#include <windows.h>

extern __declspec(thread) int __cpu_no;

DWORD WINAPI new_cpu_thread(LPVOID lpParameter)
{
	task_t *task = (task_t*)lpParameter;
	__cpu_no = task->pid;
	kCPU.running = NULL;
	
	if (setjmp(task->state.jbuf) == 0) {
	    for (;;) {
		    Sleep(10);
		    irq_enter(0);
		}
    }
}

void cpu_stack(task_t *task, size_t entry, size_t param)
{
    task->state.entry = (entry_t)entry;
    task->state.param = param;
    DWORD myThreadId;
    HANDLE myHandle = CreateThread(0, 0, new_cpu_thread, task, 0, &myThreadId);
}


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
    if (task == NULL) {
        Sleep(2);
        kCPU.running = NULL;
        irq_disable();
        continue;
    }
    // TODO Start task chrono!
    if (task->usmem)
        mmu_context(task->usmem);
    // kprintf(-1, "Start Task %d\n", task->pid)

    if (task->pid == cpu_no())
        longjmp(prev->state.jbuf, 1);
        
    Sleep(2);
    splock_lock(&task->lock);
    task->status = TS_READY;
    scheduler_add(task);
    splock_unlock(&task->lock);
    kCPU.running = NULL;
    irq_disable();
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int main()
{
    kernel_start();
    assert(kCPU.irq_semaphore == 0);
    for (;;) {
        sys_ticks();
        async_timesup();
        Sleep(10);
    }
    return 0;
}

