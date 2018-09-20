#include <threads.h>
#include <time.h>
#include <windows.h>
#include <stdbool.h>
#include <kora/hmap.h>

struct _US_THREAD {
	thrd_start_t func;
	void* arg;
	DWORD id;
	HANDLE handle;
};

typedef void(*tss_dtor_t)(void*);

bool tss_thread_init = false;
tss_t tss_key_thread;

static void thrd_init()
{
	tss_thread_init = true;
	tss_create(&tss_key_thread, (tss_dtor_t)free);
	ptrd_t thread = malloc(sizeof(* thread));
	thread->handle = GetCurrentThread();
	thread->id = GetThreadId(thread->handle);
	tss_set(tss_key_thread, thread);
}

DWORD WINAPI thrd_start(LPVOID param)
{
	struct _US_THREAD *thread = param;
	tss_set(tss_key_thread, thread);
	int ret = thread->func(thread->arg);
	thrd_exit(ret);
	return ret;
}


/* Creates a thread */
int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
	if (!tss_thread_init)
	    thrd_init();
	ptrd_t thread = malloc(sizeof(* thread));
	thread->func = func;
	thread->arg = arg;
	thread->handle = CreateThread (0, 0, thrd_start, thread, 0, &thread->id);
	return 0;
}

/* Checks if two identifiers refer to the same thread */
int thrd_equal(thrd_t lhs, thrd_t rhs)
{
	return lhs == rhs;
}

/* Obtains the current thread identifier */
thrd_t thrd_current(void)
{
	if (!tss_thread_init)
	    thrd_init();
	return tss_get(tss_key_thread);
}
/* Suspends execution of the calling thread for the given period of time */
int thrd_sleep(const struct timespec* duration, struct timespec* remaining)
{
	long dms = duration->tv_sec * 1000 + duration->tv_nsec / 1000000;
	Sleep(dms);
	if (remaining) {
	}
	return 0;
}

/* Yields the current time slice */
void thrd_yield(void)
{
	Yield();
}

/* Terminates the calling thread */
Noreturn void thrd_exit(int res)
{
	tss_remove(tss_key_thread);
	for (;;) ThreadTerminate(res);
}

/* Detaches a thread */
int thrd_detach(thrd_t thr)
{
	return -1;
}
	
/* Blocks until a thread terminates */
int thrd_join(thrd_t thr, int *res)
{
	WaitForSingleObject(thr->handle, INFINITE);
	return 0;
}







/* Creates thread-specific storage pointer with a given destructor */
int tss_create(tss_t* tss_key, tss_dtor_t destructor);
/* Reads from thread-specific storage */void *tss_get(tss_t tss_key);
/* Write to thread-specific storage */
int tss_set(tss_t tss_id, void *val);
/* Releases the resources held by a given thread-specific pointer */
void tss_delete(tss_t tss_id);
