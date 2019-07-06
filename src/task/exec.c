#include <kernel/core.h>

void exec_kloader()
{
    kprintf(-1, "New kernel loader\n");
    for (; ;);
}

void exec_init()
{
    kprintf(-1, "First process\n");
    for (; ;);
}

void exec_process()
{
    kprintf(-1, "New process\n");
    for (; ;);
}

void exec_thread()
{
    kprintf(-1, "New thread\n");
    for (; ;);
}

