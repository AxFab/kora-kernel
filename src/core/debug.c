#include <kernel/core.h>

const char *ksymbol(void* eip)
{
  return "???";
}

void stackdump(size_t frame)
{
  size_t *ebp = &frame - 2;
  size_t *args;
  kprintf(0, "Stack trace: [%x]\n", (size_t)ebp);

  while (frame-- > 0) {
    size_t eip = ebp[1];
    if (eip == 0) {
      // No caller on stack
      break;
    }

    // Unwind to previous stack frame
    ebp = (size_t *)(ebp[0]);
    args = &ebp[2];
    kprintf(0, "  0x%x - %s ()         [args: %x] \n", eip, ksymbol((void *)eip), (size_t)args);
  }
}

