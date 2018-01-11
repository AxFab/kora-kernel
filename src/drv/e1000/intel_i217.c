#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kernel/drv/pci.h>
#include <string.h>

#ifdef K_MODULE
#  define E1000_setup setup
#  define E1000_teardown teardown
#endif


#define INTEL_VEND     0x8086  // Vendor ID for Intel
#define ETERNET_CLASS  0x020000 // Class ID for Ethernet controller
#define E1000_DEV      0x100E  // Device ID for the e1000 Qemu, Bochs, and VirtualBox emmulated NICs
#define E1000_I217     0x153A  // Device ID for Intel I217
#define E1000_82577LM  0x10EA  // Device ID for Intel 82577LM


// I have gathered those from different Hobby online operating systems
// instead of getting them one by one from the manual
#define REG_CTRL        0x0000
#define REG_STATUS      0x0008
#define REG_EEPROM      0x0014
#define REG_CTRL_EXT    0x0018
#define REG_IMASK       0x00D0
#define REG_RCTRL       0x0100
#define REG_RXDESCLO    0x2800
#define REG_RXDESCHI    0x2804
#define REG_RXDESCLEN   0x2808
#define REG_RXDESCHEAD  0x2810
#define REG_RXDESCTAIL  0x2818

#define REG_TCTRL       0x0400
#define REG_TXDESCLO    0x3800
#define REG_TXDESCHI    0x3804
#define REG_TXDESCLEN   0x3808
#define REG_TXDESCHEAD  0x3810
#define REG_TXDESCTAIL  0x3818


#define REG_RDTR         0x2820 // RX Delay Timer Register
#define REG_RXDCTL       0x3828 // RX Descriptor Control
#define REG_RADV         0x282C // RX Int. Absolute Delay Timer
#define REG_RSRPD        0x2C00 // RX Small Packet Detect Interrupt



#define REG_TIPG         0x0410      // Transmit Inter Packet Gap
#define ECTRL_SLU        0x40        //set link up


#define RCTL_EN                         (1 << 1)    // Receiver Enable
#define RCTL_SBP                        (1 << 2)    // Store Bad Packets
#define RCTL_UPE                        (1 << 3)    // Unicast Promiscuous Enabled
#define RCTL_MPE                        (1 << 4)    // Multicast Promiscuous Enabled
#define RCTL_LPE                        (1 << 5)    // Long Packet Reception Enable
#define RCTL_LBM_NONE                   (0 << 6)    // No Loopback
#define RCTL_LBM_PHY                    (3 << 6)    // PHY or external SerDesc loopback
#define RTCL_RDMTS_HALF                 (0 << 8)    // Free Buffer Threshold is 1/2 of RDLEN
#define RTCL_RDMTS_QUARTER              (1 << 8)    // Free Buffer Threshold is 1/4 of RDLEN
#define RTCL_RDMTS_EIGHTH               (2 << 8)    // Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_MO_36                      (0 << 12)   // Multicast Offset - bits 47:36
#define RCTL_MO_35                      (1 << 12)   // Multicast Offset - bits 46:35
#define RCTL_MO_34                      (2 << 12)   // Multicast Offset - bits 45:34
#define RCTL_MO_32                      (3 << 12)   // Multicast Offset - bits 43:32
#define RCTL_BAM                        (1 << 15)   // Broadcast Accept Mode
#define RCTL_VFE                        (1 << 18)   // VLAN Filter Enable
#define RCTL_CFIEN                      (1 << 19)   // Canonical Form Indicator Enable
#define RCTL_CFI                        (1 << 20)   // Canonical Form Indicator Bit Value
#define RCTL_DPF                        (1 << 22)   // Discard Pause Frames
#define RCTL_PMCF                       (1 << 23)   // Pass MAC Control Frames
#define RCTL_SECRC                      (1 << 26)   // Strip Ethernet CRC

// Buffer Sizes
#define RCTL_BSIZE_256                  (3 << 16)
#define RCTL_BSIZE_512                  (2 << 16)
#define RCTL_BSIZE_1024                 (1 << 16)
#define RCTL_BSIZE_2048                 (0 << 16)
#define RCTL_BSIZE_4096                 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192                 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384                ((1 << 16) | (1 << 25))


// Transmit Command

#define CMD_EOP                         (1 << 0)    // End of Packet
#define CMD_IFCS                        (1 << 1)    // Insert FCS
#define CMD_IC                          (1 << 2)    // Insert Checksum
#define CMD_RS                          (1 << 3)    // Report Status
#define CMD_RPS                         (1 << 4)    // Report Packet Sent
#define CMD_VLE                         (1 << 6)    // VLAN Packet Enable
#define CMD_IDE                         (1 << 7)    // Interrupt Delay Enable


// TCTL Register

#define TCTL_EN                         (1 << 1)    // Transmit Enable
#define TCTL_PSP                        (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT                   4           // Collision Threshold
#define TCTL_COLD_SHIFT                 12          // Collision Distance
#define TCTL_SWXOFF                     (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC                       (1 << 24)   // Re-transmit on Late Collision

#define TSTA_DD                         (1 << 0)    // Descriptor Done
#define TSTA_EC                         (1 << 1)    // Excess Collisions
#define TSTA_LC                         (1 << 2)    // Late Collision
#define LSTA_TU                         (1 << 3)    // Transmit Underrun


#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8


PACK(struct E1000_rx_desc {
  volatile uint64_t addr;
  volatile uint16_t length;
  volatile uint16_t checksum;
  volatile uint8_t status;
  volatile uint8_t errors;
  volatile uint16_t special;
});

PACK(struct E1000_tx_desc {
  volatile uint64_t addr;
  volatile uint16_t length;
  volatile uint8_t cso;
  volatile uint8_t cmd;
  volatile uint8_t status;
  volatile uint8_t css;
  volatile uint16_t special;
});


struct E1000_card
{
  unsigned char mac_address[6];
  const char *name;
  struct PCI_device *pci;
  struct E1000_rx_desc *rx_descs; // Receive Descriptor Buffers
  struct E1000_tx_desc *tx_descs; // Transmit Descriptor Buffers
  uint16_t rx_count; // Count of Receive Descriptor Buffers
  uint16_t tx_count; // Count of Transmit Descriptor Buffers
  uint16_t rx_cur; // Current Receive Descriptor Buffer
  uint16_t tx_cur; // Current Transmit Descriptor Buffer
  uint16_t mx_count; // Count of Descriptor Buffers
  void *mx_descs; // Address of Descriptor Buffers
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static bool E1000_detect_EEProm(struct PCI_device *pci)
{
  int i;
  PCI_write(pci, REG_EEPROM, 0x1);
  for (i = 0; i < 1000; ++i) {
    if (PCI_read(pci, REG_EEPROM) & 0x10) {
      return true;
    }
  }
  return false;
}

static uint32_t E1000_read_EEProm(struct PCI_device *pci, uint32_t address)
{
  uint32_t value = 0;
  PCI_write(pci, REG_EEPROM, (address << 8) | 1);
  while ((value & 0x10) == 0) {
    value = PCI_read(pci, REG_EEPROM);
  }
  return (value >> 16) & 0xFFFF;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
int E1000_irq(struct E1000_card *card)
{
  return 0;
}

int E1000_startup(struct PCI_device *pci, const char* name)
{
  int i;
  unsigned char mac_address[6];

  // Read mac_address
  if (E1000_detect_EEProm(pci)) {
    kprintf(-1, "E1000] MAC using EEProm\n");
    for (i = 0; i < 3; ++i) {
      uint32_t tmp = E1000_read_EEProm(pci,i);
      mac_address[2 * i] = tmp & 0xff;
      mac_address[2 * i + 1] = tmp >> 8;
    }
  } else {
    kprintf(-1, "E1000] MAC using MMIO\n");
    pci->bar[0].mmio = (uint32_t)kmap(pci->bar[0].size, NULL, pci->bar[0].base & ~7, VMA_FG_PHYS);
    kprintf(-1, "E1000] MMIO mapped at %x\n", pci->bar[0].mmio);


    // page_fault(NULL, (size_t)pci->bar[0].mmio + 0x5400, 0);

    for (i = 0; i < 0x800000; ++i) asm volatile("nop");
    mmu_explain_x86((size_t)pci->bar[0].mmio + 0x5400);
    stackdump(8);

    if (*(uint32_t *)(pci->bar[0].mmio + 0x5400) == 0) {
      kprintf(0, "%s Unable to read MAC address\n", name);
      return -1;
    }

    for (i = 0; i < 0x800000; ++i) asm volatile("nop");
    mmu_explain_x86((size_t)pci->bar[0].mmio + 0x5400);

    kprintf(-1, "E1000] Reading MAC\n");
    for (i = 0; i < 6; ++i) {
      mac_address[i] = ((uint8_t *)pci->bar[0].mmio + 0x5400)[i];
    }

  }

  kprintf(-1, "E1000] MAC is %x:%x:%x:%x:%x:%x\n", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);

    for (i = 0; i < 0x800000; ++i) asm volatile("nop");
    mmu_explain_x86((size_t)pci->bar[0].mmio + 0x5400);

  // ??
  for(i = 0; i < 0x80; i++) {
    PCI_write(pci, 0x5200 + i * 4, 0);
  }

  // Allocat the new card structure
  struct E1000_card *card = (struct E1000_card *)kalloc(sizeof(struct E1000_card));
  memcpy(card->mac_address, mac_address, 6);
  card->pci = pci;
  card->name = name;

  // Allocate buffer for transfert and receive packets.
  card->mx_count = PAGE_SIZE / 16;
  // card->mx_descs = kmap(PAGE_SIZE, VMA_READ | VMA_WRITE);
  // E1000_rxinit(pci, driver);
  // E1000_txinit(pci, driver);

  irq_register(pci->irq, (irq_handler_t)E1000_irq, card);
  // TODO Register on NETWORD STACK

  // Autorize device to send IRQ.
  PCI_write(pci, REG_IMASK, 0x1F6DC);
  PCI_write(pci, REG_IMASK, 0xff & ~4);
  PCI_read(pci, 0xc0);

  kprintf(0, "%s, using MAC address %02x-%02x-%02x-%02x-%02x-%02x\n", name,
    mac_address[0], mac_address[1], mac_address[2],
    mac_address[3], mac_address[4], mac_address[5]);
  return 0;
}


int E1000_setup()
{
  struct PCI_device *pci;
  const char* name;

  for (;;) {
    pci = PCI_search(INTEL_VEND, ETERNET_CLASS, E1000_DEV);
    name = "E1000 Virtual host";
    if (pci == NULL) {
      pci = PCI_search(INTEL_VEND, ETERNET_CLASS, E1000_I217);
      name = "E1000 Intel I217";
    }
    if (pci == NULL) {
      pci = PCI_search(INTEL_VEND, ETERNET_CLASS, E1000_82577LM);
      name = "E1000 Intel 82577LM";
    }
    if (pci == NULL) {
      return 0;
    }

    kprintf(0, "Found %s\n", name);
    E1000_startup(pci, name);
  }
}

int E1000_teardown()
{
  return 0;
}
