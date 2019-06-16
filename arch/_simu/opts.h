

#define PAGE_SIZE 4096

struct opts {
    int cpu_count;
    const char *cpu_vendor;
};

extern struct opts opts;

void mmu_setup();
void cpu_setup();
void vfs_init();
void futex_init();
void scheduler_init();
