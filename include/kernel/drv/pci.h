#ifndef __KERNEL_DRV_PCI_H
#define __KERNEL_DRV_PCI_H 1

#include <kernel/core.h>

struct PCI_device {
  uint8_t bus;
  uint8_t slot;
  uint8_t irq;
  uint8_t busy;
  uint16_t vendor_id;
  uint16_t device_id;
  uint32_t class_id;
  struct {
    uint32_t base;
    uint32_t size;
    uint32_t mmio;
  } bar[4];
};

struct PCI_device *PCI_search(uint16_t vendor_id, uint32_t class_id, uint16_t device_id);

__stinline void PCI_write(struct PCI_device *pci, uint16_t address, uint32_t value)
{
  if (pci->bar[0].base & 1) {
    int iobase = pci->bar[0].base  & 0xFFFFFFFC;
    outl(iobase, address);
    outl(iobase + 4, value);
  } else {
    *((volatile uint32_t*)(pci->bar[0].mmio + address)) = value;
  }
}

__stinline uint32_t PCI_read(struct PCI_device *pci, uint16_t address)
{
  if (pci->bar[0].base & 1) {
    int iobase = pci->bar[0].base  & 0xFFFFFFFC;
    outl(iobase, address);
    return inl(iobase + 4);
  } else {
    return *((volatile uint32_t*)(pci->bar[0].mmio + address));
  }
}


#endif  /* __KERNEL_DRV_PCI_H */
