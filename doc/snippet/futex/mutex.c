#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/futex.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>

#include <sys/syscall.h>


#define NB_SECONDS 20
static char *str_second[NB_SECONDS] =
{
  "zero", "un", "deux", "trois", "quatre", "cinq",
  "six", "sept", "huit", "neuf", "dix",
  "onze", "douze", "treize", "quatorze", "quinze",
  "seize", "dix sept", "dix huit", "dix neuf"
};
char data[100];



static inline int futex(int *addr, int op, int val, struct timespec *timeout, int *addr2, int val2) {
    return syscall(SYS_futex, addr, op, val, timeout, addr2, val2);
}

int futex_var;



struct xtime {
    time_t sec;
    long nsec;
};
typedef struct xtime xtime;

typedef int (*thrd_start_t)(void*);

/*-------------------- enumeration constants --------------------*/
enum {
    mtx_plain     = 0,
    mtx_try       = 1,
    mtx_timed     = 2,
    mtx_recursive = 4
};

enum {
    thrd_success = 0, // succeeded
    thrd_timeout,     // timeout
    thrd_error,       // failed
    thrd_busy,        // resource busy
    thrd_nomem        // out of memory
};

static inline void __pfails(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    abort();
}

/* Futex -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
#define futex_wait(addr, val) syscall(SYS_futex, addr, FUTEX_WAIT, val, NULL)
#define futex_wakeup(addr, nb)  syscall(SYS_futex, addr, FUTEX_WAKE, nb)

int __futex_wait(int *addr, int val, const xtime *xt)
{
    futex_t *ftx;
    while (*uddr == val) {

    }
    return -1;
}

int __futex_wakeup(int *addr, int cnt)
{
    return -1;
}

/* Thread =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
typedef pthread_t thrd_t;
// typedef int thrd_t;


/*
Implementation limits:
  - Conditionally emulation for "mutex with timeout"
    (see EMULATED_THREADS_USE_NATIVE_TIMEDLOCK macro)
*/
struct impl_thrd_param {
    thrd_start_t func;
    void *arg;
};

void *impl_thrd_routine(void *p)
{
    struct impl_thrd_param pack = *((struct impl_thrd_param *)p);
    free(p);
    return (void*)pack.func(pack.arg);
}


int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    struct impl_thrd_param *pack;
    if (!thr) return thrd_error;
    pack = malloc(sizeof(struct impl_thrd_param));
    if (!pack) return thrd_nomem;
    pack->func = func;
    pack->arg = arg;
    if (pthread_create(thr, NULL, impl_thrd_routine, pack) != 0) {
        free(pack);
        return thrd_error;
    }
    return thrd_success;
}

// 7.25.5.2
thrd_t thrd_current(void)
{
    return pthread_self();
}

// 7.25.5.3
int thrd_detach(thrd_t thr)
{
    return (pthread_detach(thr) == 0) ? thrd_success : thrd_error;
}

// 7.25.5.4
int thrd_equal(thrd_t thr0, thrd_t thr1)
{
    return pthread_equal(thr0, thr1);
}

// 7.25.5.5
void thrd_exit(int res)
{
    pthread_exit((void*)res);
}

// 7.25.5.6
int thrd_join(thrd_t thr, int *res)
{
    void *code;
    if (pthread_join(thr, &code) != 0)
        return thrd_error;
    if (res)
        *res = (int)code;
    return thrd_success;
}

// 7.25.5.7
void thrd_sleep(const xtime *xt)
{
    struct timespec req;
    // assert(xt);
    req.tv_sec = xt->sec;
    req.tv_nsec = xt->nsec;
    nanosleep(&req, NULL);
}

// 7.25.5.8
void thrd_yield(void)
{
    sched_yield();
}


/* Mutex -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
typedef struct mtx {
    volatile int val;
    thrd_t tid;
    int flg;
    int cnt;
} mtx_t;


void mtx_destroy(mtx_t *mtx)
{
}

int mtx_init(mtx_t *mtx, int type)
{
    mtx->val = 0;
    mtx->tid = 0;
    mtx->flg = type;
    mtx->cnt = 0;
    return thrd_success;
}

int mtx_timedlock(mtx_t *mtx, const xtime *xt)
{
    thrd_t tid = thrd_current();
    if (mtx->tid == tid) {
        if (!(mtx->flg & mtx_recursive))
            __pfails("Deadlock");
        ++mtx->cnt;
        return thrd_success;
    }
    int old = __sync_val_compare_and_swap(&mtx->val, 0, 1);
    if (0 == old) {
        // Le futex était libre car il contenait la valeur 0
        // et maintenant il contient la valeur 1
        mtx->cnt = 0;
        mtx->tid = tid;
        return thrd_success;
    }

    old = __sync_lock_test_and_set(&mtx->val, 2);
    while (old != 0) {
        // Le mutex contient la valeur 1 : l’autre thread l’a pris
        // ==> Attente tant que le futex a la valeur 1
        futex_wait(&mtx->val, 2); // TODO -- timeout
        old = __sync_lock_test_and_set(&mtx->val, 2);
    }
    mtx->cnt = 0;
    mtx->tid = tid;
    return thrd_success;
}

int mtx_lock(mtx_t *mtx)
{
    static const xtime xt = { INT_MAX, LONG_MAX };
    return mtx_timedlock(mtx, &xt);
}

int mtx_trylock(mtx_t *mtx)
{
    thrd_t tid = thrd_current();
    if (mtx->tid == tid) {
        if (!(mtx->flg & mtx_recursive))
            __pfails("Deadlock");
        ++mtx->cnt;
        return thrd_success;
    }
    int old = __sync_val_compare_and_swap(&mtx->val, 0, 1);
    if (0 != old)
        return thrd_busy;
    // Le futex était libre car il contenait la valeur 0
    // et maintenant il contient la valeur 1
    mtx->cnt = 0;
    mtx->tid = tid;
    return thrd_success;
}

int mtx_unlock(mtx_t *mtx)
{
    if (mtx->cnt > 0) {
        --mtx->cnt;
        return thrd_success;
    }
    mtx->tid = 0;
    int old = __sync_fetch_and_sub(&mtx->val, 1);
    if (old == 1)
        return thrd_success;
    old = __sync_lock_test_and_set(&mtx->val, 0);
    futex_wakeup(&mtx->val, 1);
    return thrd_success;
}


static void LOCK(int *f)
{
    int old = __sync_val_compare_and_swap(f, 0, 1);
    if (0 == old)
        // Le futex était libre car il contenait la valeur 0
        // et maintenant il contient la valeur 1
        return;

    old = __sync_lock_test_and_set(f, 2);
    while (old != 0) {
        // Le mutex contient la valeur 1 : l’autre thread l’a pris
        // ==> Attente tant que le futex a la valeur 1
        futex_wait(f, 2);
        old = __sync_lock_test_and_set(f, 2);
    }
}

static void UNLOCK(int *f)
{
    int old = __sync_fetch_and_sub(f, 1);
    if (old == 1)
        return;
    old = __sync_lock_test_and_set(f, 0);
    futex_wakeup(f, 1);
}

static void *thd_main(void *p)
{
    unsigned int i = 0;
    while(1)
    {
        // Mise à jour du compteur en ASCII
        LOCK(&futex_var);
        strcpy(data, str_second[i]);

        UNLOCK(&futex_var);
        // Attente d’une seconde
        sleep(1);
        // Incrémentation du compteur dans la plage [0, NB_SECONDS[
        i = (i + 1) % NB_SECONDS;
    }
    return NULL;
}

int main(int ac, char *av[])
{
    int rc;
    pthread_t tid;
    unsigned short val = 1;
    // Création du thread
    rc = pthread_create(&tid, NULL, thd_main, NULL);
    // Interaction avec l’opérateur
    while(fgetc(stdin) != EOF)
    {
        // Affichage de la valeur ASCII courante du compteur
        LOCK(&futex_var);
        printf("Compteur courant: %s\n", data);
        UNLOCK(&futex_var);
    }
}

