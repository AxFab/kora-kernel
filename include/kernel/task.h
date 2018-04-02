#ifndef _KERNEL_TASK_H
#define _KERNEL_TASK_H 1

#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kernel/memory.h>
#include <kora/bbtree.h>
#include <kora/splock.h>

typedef struct regs regs_t;
typedef struct user user_t;
typedef struct sig_handler sig_handler_t;

typedef struct stream stream_t;
typedef struct resx resx_t;


struct user {
    uint8_t id[16];
};

enum TS_TaskState {
    TS_ZOMBIE = 0,  /* The task structure is not used. */
    TS_BLOCKED,  /* The task is paused and listen only for its own events. */
    TS_INTERRUPTIBLE,  /* The task is paused and wait for any events. */
    TS_READY,  /* The task is waiting for cpu time. */
    TS_RUNNING,  /* Task is currently running on a cpu. */
    TS_ABORTED,  /* Task should be aborded, but is currently running. */
};

#define CLONE_FILES     0x001  /* Share open files */
#define CLONE_FS        0x002  /* Share file system information */
#define CLONE_PARENT    0x004  /* Share parent and siblings */
#define CLONE_TLS       0x008  /* Share the thread local storage */
#define CLONE_SIGNAL    0x010  /* Share signal handler and blocked signal */
#define CLONE_SCALL     0x020  /* Share syscalls table */
#define CLONE_THREAD    0x040  /* The clone is placed on the same thread group */
#define CLONE_MSPACE    0x080  /* Share the memory address space */
#define CLONE_USER      0x100  /* Share the same user credential */

struct sig_handler {
    int num;
    void *type;
};

struct task {
    size_t *kstack;  /* Kernel stack base address */
    size_t *ustack;  /* User space stack base address */
    size_t kstack_len;  /* Kernel stack length */
    size_t ustack_len;  /* User space stack length */
    uint8_t state;  /* Task state (see enum TS_TaskState) */
    int prio;
    int exit_code;
    // int rp;
    regs_t *regs/*[8]*/;
    regs_t *sig_regs;
    splock_t lock;

    sig_handler_t shandler[32];

    uint32_t cpu; /* The cpu which run the task. */
    int errno;
    uint32_t recieved_signal;
    bbnode_t bnode;

    task_t *parent;

    /* Scheduler entity */
    task_t *prev;
    task_t *next;

    // unsigned long stack_canary;

    // struct timespec start_time;
    uint64_t last;  /* Register to compute elpased time. Unit is platform dependant. */
    uint64_t user_elapsed;  /* Time spend into user space code */
    uint64_t sys_elapsed;  /* Time spend into kernel space code */
    uint64_t other_elapsed;  /* Time spend on other task */

    user_t *user;

    /* Open files */
    resx_t *resx;

    /* File system information */
    inode_t *root;  /* Inode used as a root for this task */
    inode_t *pwd;

    /* Thread Local Storage */

    /* Signal handling */

    /* System calls */

    /* Thread group */
    pid_t pid;

    /* Memory address space */
    mspace_t *usmem;  /* User space memory */

};

#define TSK_USER_SPACE  0x001


void task_enter_sys(regs_t *regs, bool kernel);
void task_leave_sys();


void task_start(task_t *task, size_t entry, long args);
int task_stop(task_t *task, int code);
int task_kill(task_t *task, int signum);
int task_resume(task_t *task);

_Noreturn int task_pause(int state);
void task_signals();

task_t *task_create(user_t *user, inode_t *root, int flags);
task_t *task_clone(task_t *model, int clone, int flags);
task_t *task_search(pid_t pid);

_Noreturn void cpu_halt();
_Noreturn void cpu_run(task_t *task);

void scheduler_add(task_t *item);
void scheduler_rm(task_t *item);
_Noreturn void scheduler_next();
void scheduler_ticks();


int elf_open(task_t *task, inode_t *ino);

void cpu_setup_task(task_t *task, size_t entry, long args);
void cpu_setup_signal(task_t *task, size_t entry, long args);
void cpu_return_signal(task_t *task, regs_t *regs);

bool cpu_task_return_uspace(task_t *task);


#endif  /* _KERNEL_TASK_H */
