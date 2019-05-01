// CPU

void cpu_setup();
void cpu_sweep();
int cpu_no();
time_t cpu_time()

_Noreturn void cpu_reboot()

void cpu_setstack(cjmp_t *state, size_t entry, size_t param)
int cpu_save(cjmp_t *state)
_Noreturn void cpu_restore(cjmp_t *state)
_Noreturn void cpu_halt()

_Noreturn void cpu_username(task_t *task, size_t entry, size_t stack)

// - —------------

cjmp_t state_cpu0;
int cpu_level = 0;
atomic_int auto_incrementcpuid =1;
atomic_int cpus_up =0
int power_off = 0;

void cpu_setup()
{
	kSYS. Cpus = kalloc(128, sizeof(struct kCpu)) ;
	kCpu.arch.state = & state_cpu0;
}
void cpu_sweep()
{
	free(kSYS. Cpus)
}
time_t cpu_time() {
}

_Noreturn void cpu_reboot(int action)
{
	assert (state is cpu0)
	Power_off = 1;
	usleep(_PwMicro_ / HZ);
	longjmp(statecpu0, ++cpu_level) ;
}

// - —------------

extern __thread int __cpu_no;

static void new_cpu_thread(cjmp_t *state)
{
	__cpu_no = state.pid;
	kCpu.arch.state = state;
	kCPU.running = NULL;
	int val = setjmp(state->jbuf);
	if (state->started == 0)
	    cpu_halt () ;
	((thrd_start)state->entry) ((void*) state->param) ;
	assert(! "Can't leave thread entry function.");
}

void cpu_setstack (cjmp_t *state, size_t entry, size_t param)
{
	state->started = 0;
	state->entry = entry;
	state->param = param;
	state - >pid = atomic_fetch_add(&auto_incrementcpuid, 1) ;
	atomic_inc(&cpus_up) ;
	thrd_create(&state->thrd, (thrd_start)new_cpu_thread, state) ;
}

_Noreturn void cpu_restore(cjmp_t *state)
{
	if (power_off) {
		atomic_dec(&cpus_up) ;
		thrd_exit(0);
	}
	if (state == kCpu.arch.state)
	    longjmp(state->jbuf, 1);
	assert(kCpu.arch.state == & cpu0)
	assert(state->started == 0);
	state->started =1
	longjmp(kCpu.arch.state->jbuf, ++cpu_level);
}

_Noreturn void cpu_halt()
{
	usleep(_PwMicro_ / HZ);
	if (power_off) {
		atomic_fetch_sub(&cpus_up, 1)
		thrd_exit(0);
	}
	longjmp(kCpu.arch.state->jbuf, 2);
}

_Noreturn void cpu_username(task_t *task, size_t entry, size_t stack)
{
	assert(kCpu.arch.state != & cpu0)
	// TODO - use straces
    longjmp(kCpu.arch.state->jbuf, 3)
}

// - —------------
// MMU

void mmu_setup();
void mmu_sweep();

void mmu_context(mspace_t *);
void mmu_create_ctx(mspace_t *);
void mmu_destroy_ctx(mspace_t *);

page_t mmu_read(mspace_t *, size_t addr);
page_t mmu_drop(mspace_t *, size_t addr);
page_t mmu_resolve(mspace_t *, size_t addr, page_t phys, int vma);
page_t mmu_protect(mspace_t *, size_t addr, int vma);

// - —------------

typedef struct {
	int info;
	int count;
	page_t *pages;
	page_t *tables;
} pgd_t;

void mmu_setup()
{
	mmu_range(16 * PAGe_size, 36 * page_size);
	mmu_range(48 * PAGe_size, 96 * page_size);

	mmu_create_uspace(KSYS.kspace);
}

void mmu_sweep()
{
	mmu_destroy_uspace(KSYS.kspace);
}

void mmu_context(mspace_t *mspace)
{
	kCPU.arch .mspace = mspace;
}

void mmu_create_ctx(mspace_t *)
{
	memset(mspace, 0, sizeof(*mspwce) ;
	size_t size = KSYS. Arch. Mmu_size * PAGe_size ;
	mspace->lower_bound = (size_t) valloc(size);
	mspace->upper_bound = +size
	pgd_t *pgd = kalloc(sizeof(pgd_t))
	pgd-> count =
	mspace->dire tory = pgd
	pgd->pages = kalloc(size * sizeof(page_t));
	pgd->tables = kalloc(size/16 * sizeof(page_t));
	atomic_inc(&mspace->t_pages);
}
void mmu_destroy_ctx(mspace_t *)
{
	int i;
	pgd_t *pgd =
	assert(mspace->p_pages == 0)
	for (i = 0; i < pgd-> count ; ++i) {
	    if (pgd->tables[i]! = 0) {
		    mmu_release(pgd->tables[i]) ;
		    atomic_dec(&mspace->t_pages
	    }
	}
	free(pdg->pages) ;
	free(pdg->tables) ;
	free(pgd) ;
	atomic_dec(&mspace->t_pages) ;
	assert(mspace->t_pages == 0
	vfree(mspace->lower_bound) ;
}


page_t mmu_drop(mspace_t *, size_t addr)
{
}

page_t mmu_resolve(mspace_t *, size_t addr, page_t phys, int vma);
page_t mmu_protect(mspace_t *, size_t addr, int vma);



// - —------------

struct pg_section {
	uint8_t *bmp;
	pg_section_t *next;
	size_t base;
	int count, free;
	splock_t lock;
};

#define PAGE_PER_SECTION (PAGE_SIZE / 8)

void mmu_range(uint64_t lower, uint64_t upper)
{
	assert(IS_ALIGNED(lower, PAGE_SIZE));
	assert(IS_ALIGNED(upper, PAGE_SIZE));
	lower /= PAGE_SIZE;
	upper /= PAGE_SIZE;

	while (lower < upper) {
		pg_section_t *sec = kalloc(sizeof(pg_section_t));
		size_t up = MIN(lower + PAGE_PER_SECTION, upper);
		if (up != upper)
		    up = ALIGN_DOWN(up, PAGE_PER_SECTION);
		int cnt = up - lower;
		kSYS.pages_available += cnt;
		kSYS.pages_total += cnt;

		sec->base = lower * PAGE_SIZE;
		sec->count = sec->free = cnt;
		kSYS.pages_sections ++;

		if (kSYS.pages_head == NULL)
		    kSYS.pages_head = sec;
		else
		    kSYS.pages_tail->next = sec;
		kSYS.pages_tail = sec;

		lower = up;
	}

	if (upper > kSYS.pages_upper)
	    kSYS.pages_upper = upper;
}


// - —------------
// INO



volume_t *ino_volume()
{
	volume_t *vol = kalloc(sizeof(volume_t) ) ;
	bbtree_init(&vol->tree);
	rwlock_init(&vol->lock);
	vol->rcu = 1;

	return vol;
}

inode_t *ino_create(int no, ftype_t type, volume_t *vol)
{
	rwlock_rdlock(&vol->lock);
	inode_t *ino = bbtree_search_eq(&vol->tree, no, inode_t, bnode);
	if (ino != NULL) {
		atomic_inc(&ino->rcu);
		// remove from volume rcu!
		rwlock_rdunlock(&vol->lock);
	}

	rwlock_upgrade(&vol->lock);
	ino = kalloc(sizeof(inode_t));
	ino->no = no;
	ino->vol = vol;
	ino->type = type;
	ino->rcu = 1;
	atomic_inc(&vol->rcu);

	bbtree_insert(&vol->tree, no, inode_t, bnode);

	rwlock_wrunlock(&vol->lock);
	return ino;
}

inode_t *ino_open(inode_t *ino)
{
	if (ino != NULL) {
		atomic_inc(&ino->rcu);
		// remove from volume rcu!
	}
	return ino;
}

void ino_close(inode_t *ino)
{
	if (ino == NULL)
	    return;
	int ret = atomic_dec(&ino->rcu);
	if (ret <= -1) {
		// Push on volume Rcu!
	}
}

bool ino_haveops(inode_t *ino)
{
	return ino->ops != NULL;
}

void *ino_data(inode_t *ino)
{
	return ino->info;
}

void ino_config(inode_t *ino, void *data, ino_ops_t *ops) ;

void ino_fcntl(inode_t *);

page_t ino_fetch()
void ino_sync()
void ino_release()

// MEM (MMU / INO)

void memory_setup();
void memory_sweep();
void memory_info();

mspace_t* mspace_create();
mspace_t* mspace_open();
mspace_t* mspace_clone();
mspace_t* mspace_close();
void *mspace_map();
void mspace_unmap();
void mspace_protect();
void page_fault();

// TASK (CPU / MEM / INO)
void task_setup();
void task_sweep();
void task_info();
task_wait();
task_tick();
task_sleep();
task_create();
task_clone();
task_search();
task_close();
task_switch();
task_stop();
task_kill();

// DEV (INO) / VFS (INO)
vfs_setup();
vfs_sweep();
vfs_info();
vfs_mkdev();
vfs_rmdev();
vfs_mount();
vfs_rmdev();
vfs_lookup();
vfs_search();


// NET (INO / TASK)
void net_setup();
void net_sweep();
void net_device();
// void net_resolve();
void net_socket(int protocol, char *addr, int port);
void net_listen();


// WMGR (INO / GFX)




