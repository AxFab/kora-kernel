#include <kernel/core.h>
#include <kernel/task.h>
#include <kernel/files.h>
#include <kernel/vfs.h>
#include <kernel/dlib.h>
#include <kernel/syscalls.h>
#include <string.h>


void exec_kloader()
{
    kprintf(-1, "New kernel loader\n");
    for (;;)
        sys_sleep(SEC_TO_KTIME(1));
}

void exec_init()
{
    kprintf(-1, "First process\n");

    task_t *task = kCPU.running;
    // Create a new memory space
    if (task->usmem == NULL)
        task->usmem = mspace_create();
    mmu_context(task->usmem);

    const char **exec_args = { "krish", NULL };

    inode_t *root;
    for (;;) {
        root = vfs_mount("sdC", "isofs", "cdrom");
        if (root != NULL)
            break;
        kprintf(-1, "Waiting for 'sdC' volume !\n");
        sys_sleep(MSEC_TO_KTIME(1000));
    }

    // Create a new process structure
    proc_t *proc = dlib_process(kCPU.running->resx_fs, task->usmem);
    kCPU.running->proc = proc;


    // inode_t *dir = kSYS.dev_ino;
    // kprintf(-1, "List /dev\n");
    // list_dir(dir);
    // dir = vfs_lookup(dir, "mnt");
    // kprintf(-1, "List /dev/mnt\n");
    // list_dir(dir);
    // dir = vfs_lookup(dir, "boot");
    // kprintf(-1, "List /dev/mnt/boot\n");
    // list_dir(dir);

    // kprintf(-1, "List /dev/mnt/cdrom\n");
    // list_dir(root);
    proc->root = root;

    root = vfs_lookup(root, "usr");
    // kprintf(-1, "List /dev/mnt/cdrom/usr\n");
    // list_dir(root);

    root = vfs_lookup(root, "bin");
    // kprintf(-1, "List /dev/mnt/cdrom/usr/bin\n");
    // list_dir(root);
    proc->pwd = root;

    kprintf(-1, "Loading '%s'\n", exec_args[0]);



    // We call the linker
    int ret = dlib_openexec(proc, exec_args[0]);
    if (ret != 0) {
        kprintf(-1, "PROCESS] %s Unable to create executable image\n", exec_args[0]);
        sys_exit(-1, 0);
    }

    ret = dlib_map_all(proc);
    if (ret != 0) {
        kprintf(-1, "PROCESS] %s Error while mapping executable\n", exec_args[0]);
        sys_exit(-1, 0);
    }

    inode_t *in_tty = pipe_inode();
    inode_t *out_tty = pipe_inode();
    // inode_t *std_tty = tty_inode(tty);
    // stream_t *std_in =
    resx_set(kCPU.running->resx, in_tty);
    resx_set(kCPU.running->resx, out_tty);
    resx_set(kCPU.running->resx, out_tty);


    void *start = dlib_exec_entry(proc);
    void *stack = mspace_map(task->usmem, 0, _Mib_, NULL, 0, VMA_STACK_RW);
    stack = ADDR_OFF(stack, _Mib_ - sizeof(size_t));
    // kprintf(-1, "%s: start:%p, stack:%p\n", exec_args[0], start, stack);
    // mspace_display(task->usmem);

    // Write arguments on stack
    int i, argc;
    for (argc = 0; exec_args[argc]; ++argc);
    char **argv = ADDR_PUSH(stack, argc * sizeof(char *));
    for (i = 0; i < argc; ++i) {
        int lg = strlen(exec_args[i]) + 1;
        argv[i] = ADDR_PUSH(stack, ALIGN_UP(lg, 4));
        strcpy(argv[i], exec_args[i]);
        // kprintf(-1, "Set arg.%d: '%s'\n", i, argv[i]);
    }

    size_t *args = ADDR_PUSH(stack, 4 * sizeof(char *));
    args[1] = argc;
    args[2] = (size_t)argv;
    args[3] = 0;

    irq_reset(false);
    cpu_tss(kCPU.running);
    cpu_usermode(start, stack);
}

void exec_process(proc_start_t *info)
{
    kprintf(-1, "New process\n");
    task_t *task = kCPU.running;
    mmu_context(task->usmem);

     // Create a new process structure
    proc_t *proc = dlib_process(task->resx_fs, task->usmem);
    task->proc = proc;
    // proc->root = root;


    kprintf(-1, "Loading '%s'\n", info->path);


    // We call the linker
    int ret = dlib_openexec(proc, info->path);
    if (ret != 0) {
        kprintf(-1, "PROCESS] %s Unable to create executable image\n", info->path);
        sys_exit(-1, 0);
    }

    ret = dlib_map_all(proc);
    if (ret != 0) {
        kprintf(-1, "PROCESS] %s Error while mapping executable\n", info->path);
        sys_exit(-1, 0);
    }

    inode_t *in_tty = pipe_inode();
    inode_t *out_tty = pipe_inode();
    // inode_t *std_tty = tty_inode(tty);
    // stream_t *std_in =
    resx_set(task->resx, in_tty);
    resx_set(task->resx, out_tty);
    resx_set(task->resx, out_tty);


    void *start = dlib_exec_entry(proc);
    void *stack = mspace_map(task->usmem, 0, _Mib_, NULL, 0, VMA_STACK_RW);
    stack = ADDR_OFF(stack, _Mib_ - sizeof(size_t));
    // kprintf(-1, "%s: start:%p, stack:%p\n", path, start, stack);
    // mspace_display(task->usmem);

    // Write arguments on stack
    int i, argc;
    for (argc = 1; info->argv[argc]; ++argc);
    char **argv = ADDR_PUSH(stack, argc * sizeof(char *));

    int lg = strlen(info->path + 1);
    argv[0] = ADDR_PUSH(stack, ALIGN_UP(lg, 4));
    strcpy(argv[i], info->path);

    for (i = 1; i < argc; ++i) {
        lg = strlen(info->argv[i]) + 1;
        argv[i] = ADDR_PUSH(stack, ALIGN_UP(lg, 4));
        strcpy(argv[i], info->argv[i-1]);
        // kprintf(-1, "Set arg.%d: '%s'\n", i, argv[i]);
    }

    size_t *args = ADDR_PUSH(stack, 4 * sizeof(char *));
    args[1] = argc;
    args[2] = (size_t)argv;
    args[3] = 0;

    irq_reset(false);
    cpu_tss(kCPU.running);
    cpu_usermode(start, stack);

    for (;;)
        sys_sleep(SEC_TO_KTIME(1));
}

void exec_thread(task_start_t *info)
{
    kprintf(-1, "New thread\n");
    for (;;)
        sys_sleep(SEC_TO_KTIME(1));
}


