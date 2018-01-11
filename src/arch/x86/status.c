#include <kernel/core.h>
#include <kernel/sys/vma.h>
#include <kernel/asm/mmu.h>
#include <string.h>


long sys_call_x86(uint32_t *params)
{

  kprintf(-1, "SYS CALL\n");
  for (;;);
}

long sys_wait_x86(uint32_t *params)
{
  kprintf(-1, "SYS WAIT\n");
  for (;;);
}

void cpu_exception_x86(int no)
{
  kprintf(-1, "Detected an cpu exception: %x\n", no);
  for (;;);
}


void page_error_x86(int code)
{
  kprintf(-1, "Detected a page protection error: %x\n", code);
  for (;;);
}

/* Resolve a page fault */
// int page_fault(mspace_t *mspace, size_t address, int reason);

// int page_fault(mspace_t *mspace, size_t address, int reason);


void page_fault_x86(size_t address, int code)
{
  kprintf(-1, "Detected a page fault: @%08x\n", address);
  int ret = page_fault(NULL, address, code);
  if (ret < 0) {
    kprintf(-1, "Detected SIGSEGV\n");
    for (;;);
  }
}
