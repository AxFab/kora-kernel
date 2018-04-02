#ifndef _E1000_H
#define _E1000_H 1

#include <kernel/net.h>
#include <kernel/drv/pci.h>
#include "e1000_hw.h"

#define E1000_NUM_RX_DESC 8 // 32
#define E1000_NUM_TX_DESC 8


struct E1000_adapter {
    struct PCI_device *pci;

};

#endif  /* _E1000_H */
