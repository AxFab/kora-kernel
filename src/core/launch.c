#include <time.h>
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/memory.h>
#include <kernel/cpu.h>

int IMG_setup();
int ATA_setup();
int PCI_setup();
int E1000_setup();

void kernel_start()
{
  kprintf(-1, "Kora OS "/* _VTAG_*/ ", build at " __DATE__ ".\n");
  page_initialize();

  time_t now = cpu_time();
  kprintf(-1, "Date: %s\n", asctime(gmtime(&now)));

  kprintf (-1, "Memory available ");
  kprintf (-1, "%s on ", sztoa((uintmax_t)kMMU.pages_amount * PAGE_SIZE));
  kprintf (-1, "%s\n", sztoa((uintmax_t)kMMU.upper_physical_page * PAGE_SIZE));

  cpu_awake();
  // kprintf(-1, "Cpus count: %d\n", kSYS.cpuCount_);
  // for (i=0; i<kSYS.cpuCount_; ++i)
    // kprintf("  - CPU %d :: %s\n", i, kSYS._cpu[0].vendor_);
  // kprintf("%s\n", kSYS._cpu[0].spec_);

  kprintf (-1, "\n\e[94m  Greetings...\e[0m\n\n");

  mmu_dump_x86();

  vfs_initialize();
    /*
  IMG_setup();
    */
  ATA_setup();
  PCI_setup();
  E1000_setup();
  // for (;;);
}


void kernel_ready()
{
  for (;;);
}

