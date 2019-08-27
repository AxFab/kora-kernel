#ifndef __KERNEL_RESX__TASK_H
#define __KERNEL_RESX__TASK_H 1

#include <stddef.h>

void task_close(task_t *task);
void task_core(task_t *task);
void task_setup(task_t *task, void *entry, void *param);
void task_show_all();
void task_signals();
void task_wait(void *listener, long timeout_ms);
task_t *task_clone(task_t *model, int clone, int flags);
task_t *task_create(void *entry, void *param, const char *name);
task_t *task_fork(task_t *parent, int keep, const char **envs);
task_t *task_open(task_t *parent, usr_t *usr, rxfs_t *fs, const char **envs);
task_t *task_search(pid_t pid);
_Noreturn int task_pause(int state);
_Noreturn void task_fatal(const char *error, unsigned signum);

int task_kill(task_t *task, unsigned signum);
int task_resume(task_t *task);
int task_stop(task_t *task, int code);

void scheduler_add(task_t *item);
void scheduler_rm(task_t *item, int status);
void scheduler_switch(int state, int code);

void exec_init();
void exec_kloader();
void exec_process(proc_start_t *info);
void exec_thread(task_start_t *info);


#endif  /* __KERNEL_RESX__TASK_H */
