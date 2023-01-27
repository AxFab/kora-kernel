#include <stdint.h>
#include <kernel/core.h>

int cpu_no();

typedef struct x86_tss x86_tss_t;
struct x86_tss {
    uint16_t    previous_task, __previous_task_unused;
    uint32_t    esp0;
    uint16_t    ss0, __ss0_unused;
    uint32_t    esp1;
    uint16_t    ss1, __ss1_unused;
    uint32_t    esp2;
    uint16_t    ss2, __ss2_unused;
    uint32_t    cr3;
    uint32_t    eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint16_t    es, __es_unused;
    uint16_t    cs, __cs_unused;
    uint16_t    ss, __ss_unused;
    uint16_t    ds, __ds_unused;
    uint16_t    fs, __fs_unused;
    uint16_t    gs, __gs_unused;
    uint16_t    ldt_selector, __ldt_sel_unused;
    uint16_t    debug_flag, io_map;
    uint16_t    padding[12];
};

void x86_write_gdesc(int no, uint32_t base, uint32_t limit, unsigned access, unsigned other);
void x86_set_tss(int);

void tss_setup(sys_info_t *sysinfo)
{
    int no = cpu_no();
    cpu_info_t *cpu = &sysinfo->cpu_table[no];
    x86_tss_t *tss = (void *)0x1000;

    tss[no].debug_flag = 0;
    tss[no].io_map = 0x68;
    tss[no].esp0 = (size_t)cpu->stack + 0xFFC;
    tss[no].ss0 = 0x18;

    x86_write_gdesc(no + 7, (uint32_t)&tss[no], 0x68, 0x89, 4);
    x86_set_tss(no + 7);
    kprintf(-1, "CPU.%d TSS using stack %x\n", no, cpu->stack);
}
