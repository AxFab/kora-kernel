/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
mspace_t *mspace_create() {}
mspace_t *mspace_open(mspace_t *mspace) {}
void mspace_close(mspace_t *mspace) {}
// void mspace_map(mspace_t *mspace) {}
void *mspace_map(mspace_t *mspace, size_t address, size_t length, inode_t *ino, off_t off, off_t limit, int flags){}

inode_t *vfs_open(inode_t *ino) {}
void vfs_close(inode_t *ino) {}
int vfs_read(inode_t *ino) {}
// void vfs_close() {}

void kernel_tasklet(void* start, long arg, CSTR name)
{
    task_t *task = task_create(NULL, NULL, 0, name);
    task_start(task, (size_t)start, arg);
}

void task_1(long arg) {
    for (;;) {
        advent_wait(NULL, NULL, 50000);
        printf("A\n");
    }
}

void task_2(long arg) {
    for (;;) {
        advent_wait(NULL, NULL, 50000);
        printf("B\n");
    }
}

cpu_state_t state;
void test_01()
{
    int loop = 50;
    clock_init();
    if (cpu_save(state) == 0) {
        kernel_tasklet(task_1, 1, "T1");
        kernel_tasklet(task_2, 2, "T2");

    }

    while (loop-- > 0) {
        printf(".");
        irq_reset(false);
        sys_ticks();
    }

    printf("Bye!\n");
}

void test_02()
{
}

int main ()
{
    kprintf(-1, "\e[1;97mKora TASK check - " __ARCH " - v" _VTAG_ "\e[0m\n");
    kSYS.cpus[0] = &kCPU0;

    test_01();
    test_02();
    return 0;
}



void cpu_halt()
{
    cpu_restore(state, 1);
}





static void _exit() {
    for (;;) task_switch(TS_ZOMBIE, -42);
}

int cpu_tasklet(task_t* task, void *entry, void *param)
{
    void **stack = (void**)task->kstack + (task->kstack_len / sizeof(void*));
    task->state[7] = (size_t)entry;
    task->state[5] = (size_t)task->kstack;

    stack--; *stack = param;
    stack--; *stack = _exit;
    task->state[6] = (size_t)stack;
}





