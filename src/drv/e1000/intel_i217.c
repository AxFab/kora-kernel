#include <kernel/core.h>
#include <kernel/net.h>
#include <kernel/cpu.h>
#include <kernel/drv/pci.h>
#include <string.h>

#ifdef K_MODULE
#  define E1000_setup setup
#  define E1000_teardown teardown
#endif

#include "e1000.h"

struct E1000_card {
    net_ops_t ops;

    struct PCI_device *pci;
    struct E1000_rx_desc *rx_descs; // Receive Descriptor Buffers
    struct E1000_tx_desc *tx_descs; // Transmit Descriptor Buffers
    uint16_t rx_count; // Count of Receive Descriptor Buffers
    uint16_t tx_count; // Count of Transmit Descriptor Buffers
    uint16_t rx_cur; // Current Receive Descriptor Buffer
    uint16_t tx_cur; // Current Transmit Descriptor Buffer
    uint16_t mx_count; // Count of Descriptor Buffers
    void *mx_descs; // Address of Descriptor Buffers
    uint8_t *rx_bufs[E1000_NUM_RX_DESC];
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static bool E1000_detect_EEProm(struct PCI_device *pci)
{
    int i;
    PCI_write(pci, REG_EERD, 0x1);
    for (i = 0; i < 1000; ++i) {
        if (PCI_read(pci, REG_EERD) & 0x10) {
            return true;
        }
    }
    return false;
}

static uint32_t E1000_read_EEProm(struct PCI_device *pci, uint32_t address)
{
    uint32_t value = 0;
    PCI_write(pci, REG_EERD, (address << 8) | 1);
    while ((value & 0x10) == 0) {
        value = PCI_read(pci, REG_EERD);
    }
    return (value >> 16) & 0xFFFF;
}

static void E1000_rxinit(struct PCI_device *pci, struct E1000_card *card)
{
    int i;
    // Allocate buffer for receive descriptors.
    card->rx_descs = (struct E1000_rx_desc *)
                     card->mx_descs; //kalloc(sizeof(struct E1000_rx_desc) * E1000_NUM_RX_DESC + 16);
    for (i = 0; i < E1000_NUM_RX_DESC; i++) {
        card->rx_bufs[i] = kmap(PAGE_SIZE, NULL, 0, VMA_FG_ANON);
        card->rx_bufs[i][0] = 0;
        card->rx_descs[i].address = mmu_read(
                                        card->rx_bufs[i]); // (uint64_t)(uint8_t *)(kalloc(8192 + 16));
        card->rx_descs[i].status = 0;
    }

    card->rx_count = E1000_NUM_RX_DESC;
    card->rx_cur = 0;
    uint32_t phys = mmu_read(card->rx_descs);
    // Descriptor must be 16bits aligned, length must be 128b aligned! Min is 8 descriptor!
    PCI_write(pci, REG_RDBAL, (uint32_t)((uint64_t)phys & 0xFFFFFFFF));
    PCI_write(pci, REG_RDBAH, (uint32_t)((uint64_t)phys >> 32) );
    PCI_write(pci, REG_RDLEN, E1000_NUM_RX_DESC * 16);
    PCI_write(pci, REG_RDH, 0);
    PCI_write(pci, REG_RDT, E1000_NUM_RX_DESC - 1);
    PCI_write(pci, REG_RCTL, RCTL_EN
              | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE
              | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_2048);
}

static void E1000_txinit(struct PCI_device *pci, struct E1000_card *card)
{
    int i;
    // Allocate buffer for receive descriptors.
    card->tx_descs = &((struct E1000_tx_desc *)
                       card->mx_descs)[E1000_NUM_RX_DESC]; //(struct E1000_tx_desc*)kalloc(sizeof(struct E1000_tx_desc) * E1000_NUM_TX_DESC + 16);
    for (i = 0; i < E1000_NUM_TX_DESC; i++) {
        card->tx_descs[i].address = 0;
        card->tx_descs[i].cmd = 0;
        card->tx_descs[i].status = TSTA_DD;
    }

    card->tx_count = E1000_NUM_TX_DESC;
    card->tx_cur = 0;
    uint32_t phys = mmu_read(card->tx_descs);
    PCI_write(pci, REG_TDBAL, (uint32_t)((uint64_t)phys & 0xFFFFFFFF));
    PCI_write(pci, REG_TDBAH, (uint32_t)((uint64_t)phys >> 32) );
    PCI_write(pci, REG_TDLEN, E1000_NUM_TX_DESC * 16);

    // setup numbers
    PCI_write(pci, REG_TDH, 0);
    PCI_write(pci, REG_TDT, 0);
    PCI_write(pci, REG_TCTL, TCTL_EN
              | TCTL_PSP
              | (15 << TCTL_CT_SHIFT)
              | (64 << TCTL_COLD_SHIFT)
              | TCTL_RTLC);

    // // This line of code overrides the one before it but I left both to highlight that the previous one works with e1000 cards, but for the e1000e cards
    // // you should set the TCTRL register as follows. For detailed description of each bit, please refer to the Intel Manual.
    // // In the case of I217 and 82577LM packets will not be sent if the TCTRL is not configured using the following bits.
    // PCI_write(pci, REG_TCTL, 0b0110000000000111111000011111010);
    PCI_write(pci, REG_TIPG, 0x0060200A);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
int E1000_irq(net_device_t *ndev)
{
    struct E1000_card *card = (struct E1000_card *)ndev->ops;

    uint32_t cause = PCI_read(card->pci, 0xc0);
    if (cause & 0x04) { // Link status change
        // start Link !?
    } else if (cause &
               0x10) { // Min thresohold RX hit -  We should add more RX_DECS
        // good threshold !?
    } else if (cause & 0x80) { // Received ticks
        // We are receiving a new packet.
        uint16_t old_cur;
        while (card->rx_descs[card->rx_cur].status & 1) {

            uint8_t *buffer =
                card->rx_bufs[card->rx_cur]; // (uint8_t*)card->rx_descs[card->rx_cur].address;
            uint16_t length = card->rx_descs[card->rx_cur].length;

            kprintf(0, "E1000] Received packet (%d bytes)\n", length);
            net_receive(ndev, buffer, length); // Inject new packet on network stack

            card->rx_descs[card->rx_cur].status = 0;
            old_cur = card->rx_cur;
            card->rx_cur = (card->rx_cur + 1) % card->rx_count;
            PCI_write(card->pci, REG_RDT, old_cur);
        }
    }
    return 0;
}

int E1000_send(net_device_t *ndev, sk_buffer_t *skb)
{
    struct E1000_card *card = (struct E1000_card *)ndev->ops;

    uint16_t old_cur = card->tx_cur;
    card->tx_descs[card->tx_cur].address = skb->page;
    card->tx_descs[card->tx_cur].length = ALIGN_UP(skb->length, 64);
    kprintf(0,
            "E1000] Waiting (%x:%x) {\n    CSO:%x\n    CMD:%x\n    STA:%x\n    CSS:%x\n    SPC:%x\n  }\n",
            (uint32_t)card->tx_descs[old_cur].address,
            card->tx_descs[old_cur].length,
            card->tx_descs[old_cur].cso,
            card->tx_descs[old_cur].cmd,
            card->tx_descs[old_cur].status,
            card->tx_descs[old_cur].css,
            card->tx_descs[old_cur].special
           );
    card->tx_descs[card->tx_cur].cmd = CMD_EOP | CMD_IFCS | CMD_IC | CMD_RS |
                                       CMD_RPS;
    kprintf(0,
            "E1000] Waiting (%x:%x) {\n    CSO:%x\n    CMD:%x\n    STA:%x\n    CSS:%x\n    SPC:%x\n  }\n",
            (uint32_t)card->tx_descs[old_cur].address,
            card->tx_descs[old_cur].length,
            card->tx_descs[old_cur].cso,
            card->tx_descs[old_cur].cmd,
            card->tx_descs[old_cur].status,
            card->tx_descs[old_cur].css,
            card->tx_descs[old_cur].special
           );
    card->tx_descs[card->tx_cur].status = 0;
    kprintf(0,
            "E1000] Waiting (%x:%x) {\n    CSO:%x\n    CMD:%x\n    STA:%x\n    CSS:%x\n    SPC:%x\n  }\n",
            (uint32_t)card->tx_descs[old_cur].address,
            card->tx_descs[old_cur].length,
            card->tx_descs[old_cur].cso,
            card->tx_descs[old_cur].cmd,
            card->tx_descs[old_cur].status,
            card->tx_descs[old_cur].css,
            card->tx_descs[old_cur].special
           );
    card->tx_cur = (card->tx_cur + 1) % card->tx_count;
    PCI_write(card->pci, REG_TDT, card->tx_cur);
    kprintf(0, "E1000] Sending packet (%d bytes)...\n", skb->length);
    bufdump(skb->address, skb->length);
    while (!(card->tx_descs[old_cur].status & 0xFF)) {
        int j;
        for (j = 50000; j > 0; j--);
        kprintf(0,
                "E1000] Waiting (%x:%x) {\n    CSO:%x\n    CMD:%x\n    STA:%x\n    CSS:%x\n    SPC:%x\n  }\n",
                (uint32_t)card->tx_descs[old_cur].address,
                card->tx_descs[old_cur].length,
                card->tx_descs[old_cur].cso,
                card->tx_descs[old_cur].cmd,
                card->tx_descs[old_cur].status,
                card->tx_descs[old_cur].css,
                card->tx_descs[old_cur].special
               );
    }
    kprintf(0, "E1000] Sended packet\n");
    return 0;
}


int E1000_startup(struct PCI_device *pci, const char *name)
{
    int i;
    unsigned char mac_address[6];

    pci->bar[0].mmio = (uint32_t)kmap(pci->bar[0].size, NULL,
                                      pci->bar[0].base & ~7, VMA_FG_PHYS);
    kprintf(-1, "E1000] MMIO mapped at %x\n", pci->bar[0].mmio);

    // Read mac_address
    if (E1000_detect_EEProm(pci)) {
        for (i = 0; i < 3; ++i) {
            uint32_t tmp = E1000_read_EEProm(pci, i);
            mac_address[2 * i] = tmp & 0xff;
            mac_address[2 * i + 1] = tmp >> 8;
        }
    } else {
        if (*(uint32_t *)(pci->bar[0].mmio + 0x5400) == 0) {
            kprintf(0, "%s Unable to read MAC address\n", name);
            return -1;
        }

        for (i = 0; i < 6; ++i) {
            mac_address[i] = ((uint8_t *)pci->bar[0].mmio + 0x5400)[i];
        }
    }

    kprintf(-1, "E1000] MAC is %02X:%02X:%02X:%02X:%02X:%02X\n",
            mac_address[0], mac_address[1], mac_address[2],
            mac_address[3], mac_address[4], mac_address[5]);

    // ??
    for (i = 0; i < 0x80; i++) {
        PCI_write(pci, 0x5200 + i * 4, 0);
    }

    // Allocat the new card structure
    struct E1000_card *card = (struct E1000_card *)kalloc(sizeof(
                                  struct E1000_card));
    card->pci = pci;
    // card->name = name;
    card->ops.send = E1000_send;

    // Allocate buffer for transfert and receive packets.
    kprintf(0, "E1000] Init buffers\n");
    card->mx_count = PAGE_SIZE / 16;
    card->mx_descs = kmap(PAGE_SIZE, NULL, 0, VMA_FG_ANON);
    E1000_rxinit(pci, card);
    E1000_txinit(pci, card);
    kprintf(0, "E1000] Init buffers done\n");

    net_device_t *ndev = net_register(name, &card->ops, mac_address, 6);
    irq_register(pci->irq, (irq_handler_t)E1000_irq, ndev);

    // Autorize device to send IRQ.
    PCI_write(pci, REG_IMS, ~0); //0x1F6DC);
    // PCI_write(pci, REG_IMASK, 0xff & ~4);
    PCI_read(pci, REG_STATUS);

    kprintf(0, "%s, using MAC address %02X:%02X:%02X:%02X:%02X:%02X\n", name,
            mac_address[0], mac_address[1], mac_address[2],
            mac_address[3], mac_address[4], mac_address[5]);
    return 0;
}


int E1000_setup()
{
    struct PCI_device *pci;
    const char *name;

    for (;;) {
        pci = PCI_search(INTEL_VENDOR, ETERNET_CLASS, E1000_DEV_ID_82540EM);
        name = "E1000 Virtual host";
        // if (pci == NULL) {
        //   pci = PCI_search(INTEL_VEND, ETERNET_CLASS, E1000_I217);
        //   name = "E1000 Intel I217";
        // }
        // if (pci == NULL) {
        //   pci = PCI_search(INTEL_VEND, ETERNET_CLASS, E1000_82577LM);
        //   name = "E1000 Intel 82577LM";
        // }
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
