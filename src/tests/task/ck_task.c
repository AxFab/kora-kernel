/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#include <kernel/core.h>
#include <kernel/task.h>
#include <kora/mcrs.h>
#include <errno.h>

struct kSys kSYS;
struct kCpu kCPU0;


void mspace_display(mspace_t *mspace) {}
void mmu_context(mspace_t *mspace) {}
mspace_t *mspace_create()
{
    return NULL;
}
mspace_t *mspace_open(mspace_t *mspace)
{
    return NULL;
}
void mspace_close(mspace_t *mspace) {}
// void mspace_map(mspace_t *mspace) {}
void *mspace_map(mspace_t *mspace, size_t address, size_t length, inode_t *ino, off_t off, int flags)
{
    return NULL;
}

inode_t *vfs_open(inode_t *ino)
{
    return NULL;
}
void vfs_close(inode_t *ino) {}
int vfs_read(inode_t *ino)
{
    return -1;
}
// void vfs_close() {}

void task_1(long arg)
{
    for (;;) {
        advent_wait(NULL, NULL, 50000);
        kprintf(-1, "A\n");
    }
}

void task_2(long arg)
{
    for (;;) {
        advent_wait(NULL, NULL, 50000);
        kprintf(-1, "B\n");
    }
}

void sys_ticks_um()
{
    irq_disable();
    assert(kCPU.irq_semaphore == 1);
    // task_enter_sys(regs, regs->cs == SGM_CODE_KERNEL);
    // kTSK.regs = regs;
    // kprintf(KLOG_DBG, "[x86 ] IRQ %d\n", no);
    // bufdump(regs, 0x60);
    sys_ticks(0);
    kCPU.io_elapsed += clock_elapsed(&kCPU.last);
    if (kCPU.running)
        kCPU.running->other_elapsed += clock_elapsed(&kCPU.running->last);
    // task_signals();
    // task_leave_sys();
    assert(kCPU.irq_semaphore == 1);
    irq_reset(false);
}

cpu_state_t state;
void test_01()
{
    int loop = 50;
    clock_init();
    if (cpu_save(state) == 0) {
        task_create(task_1, (void *)1, "T1");
        task_create(task_2, (void *)2, "T2");

    }

    irq_reset(true);
    while (loop-- > 0) {
        kprintf(-1, ".");
        sys_ticks_um();
    }

    task_t *task;
    task = task_search(1);
    if (task)
        task_destroy(task);
    task = task_search(2);
    if (task)
        task_destroy(task);
    kprintf(-1, "Bye!\n");
}

void test_02()
{
}

int main()
{
    kprintf(-1, "\e[1;97mKora TASK check - " __ARCH " - v" _VTAG_ "\e[0m\n");
    kSYS.cpus = &kCPU0;

    test_01();
    test_02();
    return 0;
}



void cpu_halt()
{
    cpu_restore(state, 1);
}





static void _exit()
{
    for (;;)
        scheduler_switch(TS_ZOMBIE, -42);
}

void cpu_stack(task_t *task, size_t entry, size_t param)
{
    size_t *stack = (size_t *)task->kstack + (task->kstack_len / sizeof(size_t));
    task->state[7] = entry;
    task->state[5] = (size_t)stack;

    stack--;
    *stack = param;
    stack--;
    *stack = (size_t)_exit;
    task->state[6] = (size_t)stack;
}





