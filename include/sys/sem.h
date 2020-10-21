#ifndef SYS_SEM_H
#define SYS_SEM_H 1

#include <threads.h>

typedef struct sem sem_t;

struct sem {
    mtx_t mtx;
    cnd_t cv;
    int count;
};

int sem_init(sem_t *sem, int count);
void sem_destroy(sem_t *sem);
void sem_acquire(sem_t *sem);
void sem_acquire_many(sem_t *sem, int count);
int sem_tryacquire(sem_t *sem);
void sem_release(sem_t *sem);
void sem_release_many(sem_t *sem, int count);


#endif /* SYS_SEM_H */
