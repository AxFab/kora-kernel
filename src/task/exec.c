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
    char *execname = "krish";
    char *exec_args[] = {
        "krish", "-x", "-s", NULL
    };

    task_t *task = kCPU.running;
    kprintf(-1, "First process\n");

     // Create a new memory space
    if (task->usmem == NULL)
        task->usmem = mspace_create();
    mspace_t *mspace = task->usmem;
    mmu_context(mspace);
    task->usmem = mspace;


    // Looking for root !
    inode_t *root;
    for (;;) {
        root = vfs_mount("sdC", "isofs", "cdrom");
        if (root != NULL)
            break;
        kprintf(-1, "Waiting for 'sdC' volume !\n");
        sys_sleep(MSEC_TO_KTIME(1000));
    }

    // Looking for exec dir
    inode_t *pwd = vfs_lookup(root, "usr");
    pwd = vfs_lookup(pwd, "bin");

    // Create a new process structure
    proc_t *proc = dlib_process(task->resx_fs, mspace);
    task->proc = proc;
    proc->root = root;
    proc->pwd = pwd;

    kprintf(-1, "Loading '%s'\n", execname);
    rxfs_chroot(task->fs, root);
    rxfs_chdir(task->fs, pwd);


    // We call the linker
    int ret = dlib_openexec(proc, execname);
    if (ret != 0) {
        kprintf(-1, "PROCESS] %s Unable to create executable image\n", execname);
        sys_exit(-1, 0);
    }

    ret = dlib_map_all(proc);
    if (ret != 0) {
        kprintf(-1, "PROCESS] %s Error while mapping executable\n", execname);
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
    void *stack = mspace_map(mspace, 0, _Mib_, NULL, 0, VMA_STACK_RW);
    stack = ADDR_OFF(stack, _Mib_ - sizeof(size_t));
    // kprintf(-1, "%s: start:%p, stack:%p\n", execname, start, stack);
    // mspace_display(mspace);

    // Write arguments on stack
    int i, argc = 3;
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

    // kprintf(-1, "%s: start:%p, stack:%p\n", execname, start, stack);
    irq_reset(false);
    cpu_tss(task);
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
    proc->root = task->fs->root;
    proc->pwd = task->fs->pwd;


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
    strcpy(argv[0], info->path);

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


