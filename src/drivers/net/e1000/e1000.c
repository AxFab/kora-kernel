#include <kernel/net.h>
#include <kernel/drv/pci.h>
#include <kernel/cpu.h>
#include <kora/mcrs.h>
#include <string.h>
#include "e1000_hw.h"

#define INTEL_VENDOR  0x8086
#define ETERNET_CLASS  0x020000

#define NUM_RX_DESC 32
#define NUM_TX_DESC 8

#define _B(n)  (1<<(n))

PACK(struct rx_desc {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint16_t checksum;
    volatile uint8_t  status;
    volatile uint8_t  errors;
    volatile uint16_t special;
});

PACK(struct tx_desc {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint8_t  cso;
    volatile uint8_t  cmd;
    volatile uint8_t  status;
    volatile uint8_t  css;
    volatile uint16_t special;
});

typedef struct E1000_inet {
    netdev_t dev;
    struct PCI_device *pci;
    const char *name;
    splock_t lock;

    size_t rx_phys;
    uint8_t rx_index;
    uint8_t *rx_virt[NUM_RX_DESC];
    struct rx_desc *rx_base;
    int rx_count;

    size_t tx_phys;
    uint8_t tx_index;
    uint8_t *tx_virt[NUM_TX_DESC];
    struct tx_desc *tx_base;
    int tx_count;

    int mx_count;

} E1000_inet_t;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int e1000_send(E1000_inet_t *ifnet, skb_t *skb)
{
    kprintf(KLOG_DBG, "REQUEST SEND NETWORK %s (%d)\n", skb->log, skb->length);
    kdump(skb->buf, skb->length);
    splock_lock(&ifnet->lock);
    struct PCI_device *pci = ifnet->pci;
    ifnet->tx_index = PCI_rd32(pci, 0, REG_TDT);

    memcpy(ifnet->tx_virt[ifnet->tx_index], skb->buf, skb->length);
    ifnet->tx_base[ifnet->tx_index].length = skb->length;
    ifnet->tx_base[ifnet->tx_index].cmd = CMD_EOP | CMD_IFCS | CMD_RS;
    ifnet->tx_base[ifnet->tx_index].status = 0;
    ifnet->tx_index = (ifnet->tx_index + 1) % ifnet->tx_count;
    PCI_wr32(pci, 0, REG_TDT, ifnet->tx_index);
    splock_unlock(&ifnet->lock);
    return 0;
}

int e1000_irq_handler(E1000_inet_t *ifnet)
{
    struct PCI_device *pci = ifnet->pci;
    uint32_t status = PCI_rd32(pci, 0, 0xc0);
    kprintf(KLOG_DBG, "%s IRQ status: %x\n", ifnet->name, status);
    if (status == 0)
        return -1;

    if (status & 0x04) {
        ifnet->dev.flags |= NET_CONNECTED;
        // int link_up = PCI_rd32(pci, 0, REG_STATUS) & (1 << 1)
        kprintf(KLOG_DBG, "%s Link UP \n", ifnet->name);
    } else if (status & 0x10) {
        kprintf(KLOG_DBG, "%s Min thresohold RX hit \n", ifnet->name);
        // Send ICMP command 3 !?
    } else if (status & 0xC0) {
        kprintf(KLOG_DBG, "%s Receive ticks \n", ifnet->name);
        // uint16_t old_idx;
        // while (ifnet->rx_descs[ifnet->rx_index].status & 1) {
            // uint8_t *buffer =
            //     ifnet->rx_bufs[card->rx_cur]; // (uint8_t*)card->rx_descs[card->rx_cur].address;
            // uint16_t length = card->rx_descs[card->rx_cur].length;

            // kprintf(KLOG_DBG, "E1000] Received packet (%d bytes)\n", length);
            // net_receive(ndev, buffer, length); // Inject new packet on network stack

            // card->rx_descs[card->rx_cur].status = 0;
            // old_cur = card->rx_cur;
            // card->rx_cur = (card->rx_cur + 1) % card->rx_count;
            // PCI_write(card->pci, REG_RDT, old_cur);
        // }
    }
    return 0;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// static void e1000_irq_disable(E1000_inet_t *ifnet)
// {
//     struct PCI_device *pci = ifnet->pci;
//     PCI_wr32(pci, 0, REG_IMC, ~0);
//     PCI_rd32(pci, 0, REG_STATUS); /* Flush */

// }

// static void e1000_irq_enable(E1000_inet_t *ifnet)
// {
//     struct PCI_device *pci = ifnet->pci;
//     PCI_wr32(pci, 0, REG_IMS, ~0);
//     PCI_rd32(pci, 0, REG_STATUS); /* Flush */
// }

int e1000_read_mac(struct PCI_device *pci, uint8_t *mac)
{
    int i;
    bool has_eeprom = false;
    // Detect eeprom
    PCI_wr32(pci, 0, REG_EERD, 0x1);
    for (i = 0; i < 1000; ++i) {
        if (PCI_rd32(pci, 0, REG_EERD) & 0x10) {
            has_eeprom = true;
            break;
        }
    }


    if (has_eeprom) {
        uint32_t tmp;
        for (i = 0; i < 3; ++i) {
            PCI_wr32(pci, 0, REG_EERD, 1 | (i << 8));
            while (!((tmp = PCI_rd32(pci, 0, REG_EERD)) & 0x10));
            mac[i * 2 + 0] = (tmp >> 16) & 0xFF;
            mac[i * 2 + 1] = (tmp >> 24) & 0xFF;
        }
    } else {
        if (*(uint32_t *)(pci->bar[0].mmio + 0x5400) == 0)
            return -1;
        for (i = 0; i < 6; ++i)
            mac[i] = ((uint8_t *)pci->bar[0].mmio + 0x5400)[i];
    }
    return 0;
}

void e1000_init_hw(E1000_inet_t *ifnet)
{
    int i;
    struct PCI_device *pci = ifnet->pci;

    /* Wait */
    advent_wait(NULL, NULL, 1000000); // 1 sec

    uint32_t status = PCI_rd32(pci, 0, REG_CTRL);
    status |= (1 << 5);   /* set auto speed detection */
    status |= (1 << 6);   /* set link up */
    status &= ~(1 << 3);  /* unset link reset */
    status &= ~(1 << 31); /* unset phy reset */
    status &= ~(1 << 7);  /* unset invert loss-of-signal */
    PCI_wr32(pci, 0, REG_CTRL, status);

    /* Disables flow control */
    PCI_wr32(pci, 0, REG_FEXTNVM, 0);
    PCI_wr32(pci, 0, REG_FCAH, 0);
    PCI_wr32(pci, 0, REG_FCT, 0);
    PCI_wr32(pci, 0, REG_FCTTV, 0);

    /* Unset flow control */
    status = PCI_rd32(pci, 0, REG_CTRL);
    status &= ~(1 << 30);
    PCI_wr32(pci, 0, REG_CTRL, status);

    /* Wait */
    advent_wait(NULL, NULL, 1000000); // 1 sec

    // kprintf(0, "Check E1000 IRQ = %d\n", PCI_cfg_rd16(pci, PCI_INTERRUPT_LINE) & 0xFF);
    irq_register(pci->irq, (irq_handler_t)e1000_irq_handler, ifnet);


    /* Clear Multicast Table Array (MTA). */
    for (i = 0; i < 0x80; i++)
        PCI_wr32(pci, 0, REG_MTA + i * 4, 0);
    /* Initialize statistics registers. */
    for (i = 0; i < 0x40; i++)
        PCI_wr32(pci, 0, REG_CRCERRS + i * 4, 0);

    PCI_wr32(pci, 0, REG_CTRL, (1 << 4));


    // Descriptor must be 16bits aligned, length must be 128b aligned! Min is 8 descriptor!
    PCI_wr32(pci, 0, REG_RDBAL, (uint32_t)((uint64_t)ifnet->rx_phys & 0xFFFFFFFF));
    PCI_wr32(pci, 0, REG_RDBAH, (uint32_t)((uint64_t)ifnet->rx_phys >> 32) );
    PCI_wr32(pci, 0, REG_RDLEN, NUM_RX_DESC * 16);
    PCI_wr32(pci, 0, REG_RDH, 0);
    PCI_wr32(pci, 0, REG_RDT, NUM_RX_DESC - 1);
    // PCI_wr32(pci, 0, REG_RCTL, RCTL_EN
    //           | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE
    //           | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_2048);
    PCI_wr32(pci, 0, REG_RCTL, RCTL_EN |
        (PCI_rd32(pci, 0, REG_RCTL) & (~((1 << 17) | (1 << 16)))));


    PCI_wr32(pci, 0, REG_TDBAL, (uint32_t)((uint64_t)ifnet->tx_phys & 0xFFFFFFFF));
    PCI_wr32(pci, 0, REG_TDBAH, (uint32_t)((uint64_t)ifnet->tx_phys >> 32) );
    PCI_wr32(pci, 0, REG_TDLEN, NUM_TX_DESC * 16);
    PCI_wr32(pci, 0, REG_TDH, 0);
    PCI_wr32(pci, 0, REG_TDT, 0);
    // PCI_wr32(pci, 0, REG_TCTL, TCTL_EN
    //           | TCTL_PSP
    //           | (15 << TCTL_CT_SHIFT)
    //           | (64 << TCTL_COLD_SHIFT)
    //           | TCTL_RTLC);
    PCI_wr32(pci, 0, REG_TCTL, TCTL_EN | TCTL_PSP | PCI_rd32(pci, 0, REG_TCTL));

    // // This line of code overrides the one before it but I left both to highlight that the previous one works with e1000 cards, but for the e1000e cards
    // // you should set the TCTRL register as follows. For detailed description of each bit, please refer to the Intel Manual.
    // // In the case of I217 and 82577LM packets will not be sent if the TCTRL is not configured using the following bits.
    // PCI_write(pci, REG_TCTL, 0b0110000000000111111000011111010);
    // PCI_write(pci, REG_TIPG, 0x0060200A);

    /* Twiddle interrupts */
    PCI_wr32(pci, 0, REG_IMS, 0xFF);
    PCI_wr32(pci, 0, REG_IMC, 0xFF);
    PCI_wr32(pci, 0, REG_IMS, _B(0) | _B(1) | _B(2) | _B(6) | _B(7));

    /* Wait */
    advent_wait(NULL, NULL, 1000000); // 1 sec

    uint32_t is_up = PCI_rd32(pci, 0, REG_STATUS); // & (1 << 1);
    if (is_up & (1 << 1)) {
        ifnet->dev.flags |= NET_CONNECTED;
        kprintf(0, "%s, DONE %x \n", ifnet->name, is_up);
    }
}

// void e1000_start(E1000_inet_t *ifnet)
// {
//     e1000_init_hw(ifnet);
//     // net_tasket(ifnet);

//     ifnet->dev.ip4_addr[0] = 192;
//     ifnet->dev.ip4_addr[1] = 168;
//     ifnet->dev.ip4_addr[2] = 1;
//     ifnet->dev.ip4_addr[3] = 9;
//     uint32_t ip = 0x0101a8c0;
//     arp_query(&ifnet->dev, (uint8_t*)&ip);


//     for (;;) {
//         advent_wait(NULL, NULL, 1000000); // 1 sec
//         kprintf(0, "E1000's waiting...\n");
//     }
// }


void e1000_startup(struct PCI_device *pci, const char *name)
{
    int i;
    E1000_inet_t *ifnet = (E1000_inet_t*)kalloc(sizeof(E1000_inet_t));

    ifnet->pci = pci;
    ifnet->name = strdup(name);
    ifnet->dev.mtu = 1500;
    splock_init(&ifnet->lock);

    pci->bar[0].mmio = (uint32_t)kmap(pci->bar[0].size, NULL, pci->bar[0].base & ~7, VMA_PHYSIQ);
    // kprintf(KLOG_DBG, "%s MMIO mapped at %x\n", name, pci->bar[0].mmio);

    ifnet->rx_base = kmap(4096, NULL, 0, VMA_ANON_RW | VMA_RESOLVE);
    ifnet->rx_phys = mmu_read(NULL, (size_t)ifnet->rx_base);
    ifnet->rx_count = MIN(NUM_RX_DESC, 4096 / sizeof(struct rx_desc));

    ifnet->tx_base = kmap(4096, NULL, 0, VMA_ANON_RW | VMA_RESOLVE);
    ifnet->tx_phys = mmu_read(NULL, (size_t)ifnet->tx_base);
    ifnet->tx_count = MIN(NUM_TX_DESC, 4096 / sizeof(struct tx_desc));

    ifnet->mx_count = ifnet->rx_count + ifnet->tx_count;
    uint8_t *map = kmap(4096 * ifnet->mx_count, NULL, 0, VMA_ANON_RW | VMA_RESOLVE);

    for (i = 0; i < ifnet->rx_count; ++i) {
        ifnet->rx_virt[i] = &map[PAGE_SIZE * i];
        ifnet->rx_base[i].addr = mmu_read(NULL, (size_t)ifnet->rx_virt[i]);
        // kprintf(0, "rx[%d] 0x%x -> 0x%x\n", i, ifnet->rx_virt[i], ifnet->rx_base[i].addr);
        ifnet->rx_base[i].status = 0;
    }

    for (i = 0; i < ifnet->tx_count; ++i) {
        ifnet->tx_virt[i] = &map[PAGE_SIZE * (i + ifnet->rx_count)];
        ifnet->tx_base[i].addr = mmu_read(NULL, (size_t)ifnet->tx_virt[i]);
        // kprintf(0, "tx[%d] 0x%x -> 0x%x\n", i, ifnet->tx_virt[i], ifnet->tx_base[i].addr);
        ifnet->tx_base[i].status = 0;
        ifnet->tx_base[i].cmd = (1 << 0);
    }

    /* PCI Init command */
    uint16_t cmd = PCI_cfg_rd16(pci, PCI_COMMAND);
    cmd |= (1 << 2) | (1 << 0);
    PCI_cfg_wr32(pci, PCI_COMMAND, cmd);

    /* Find MAC address */
    if (e1000_read_mac(pci, ifnet->dev.eth_addr) != 0)
        return;

    /* initialize */
    PCI_wr32(pci, 0, REG_CTRL, (1 << 26));

    ifnet->dev.send = (void*)e1000_send;
    ifnet->dev.link = (void*)e1000_init_hw;

    net_device(&ifnet->dev);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct e1000_id {
    uint16_t id;
    const char *name;
};


struct e1000_id __e1000_ids[] = {
    { .id = E1000_DEV_ID_82542, .name = "E1000 82542" },
    { .id = E1000_DEV_ID_82543GC_FIBER, .name = "T Server (82543GC-fiber)" },
    { .id = E1000_DEV_ID_82543GC_COPPER, .name = "T Server (82543GC-copper)" },
    { .id = E1000_DEV_ID_82544EI_COPPER, .name = "E1000 82544EI COPPER" },
    { .id = E1000_DEV_ID_82544EI_FIBER, .name = "E1000 82544EI FIBER" },
    { .id = E1000_DEV_ID_82544GC_COPPER, .name = "E1000 82544GC COPPER" },
    { .id = E1000_DEV_ID_82544GC_LOM, .name = "E1000 82544GC LOM" },
    { .id = E1000_DEV_ID_82540EM, .name = "MT Desktop (82540EM)" },
    { .id = E1000_DEV_ID_82540EM_LOM, .name = "E1000 82540EM LOM" },
    { .id = E1000_DEV_ID_82540EP_LOM, .name = "E1000 82540EP LOM" },
    { .id = E1000_DEV_ID_82540EP, .name = "E1000 82540EP" },
    { .id = E1000_DEV_ID_82540EP_LP, .name = "E1000 82540EP LP" },
    { .id = E1000_DEV_ID_82545EM_COPPER, .name = "E1000 82545EM COPPER" },
    { .id = E1000_DEV_ID_82545EM_FIBER, .name = "E1000 82545EM FIBER" },
    { .id = E1000_DEV_ID_82545GM_COPPER, .name = "E1000 82545GM COPPER" },
    { .id = E1000_DEV_ID_82545GM_FIBER, .name = "E1000 82545GM FIBER" },
    { .id = E1000_DEV_ID_82545GM_SERDES, .name = "E1000 82545GM SERDES" },
    { .id = E1000_DEV_ID_82546EB_COPPER, .name = "E1000 82546EB COPPER" },
    { .id = E1000_DEV_ID_82546EB_FIBER, .name = "E1000 82546EB FIBER" },
    { .id = E1000_DEV_ID_82546EB_QUAD_COPPER, .name = "E1000 82546EB QUAD COPPER" },
    { .id = E1000_DEV_ID_82541EI, .name = "E1000 82541EI" },
    { .id = E1000_DEV_ID_82541EI_MOBILE, .name = "E1000 82541EI MOBILE" },
    { .id = E1000_DEV_ID_82541ER_LOM, .name = "E1000 82541ER LOM" },
    { .id = E1000_DEV_ID_82541ER, .name = "E1000 82541ER" },
    { .id = E1000_DEV_ID_82547GI, .name = "E1000 82547GI" },
    { .id = E1000_DEV_ID_82541GI, .name = "E1000 82541GI" },
    { .id = E1000_DEV_ID_82541GI_MOBILE, .name = "E1000 82541GI MOBILE" },
    { .id = E1000_DEV_ID_82541GI_LF, .name = "E1000 82541GI LF" },
    { .id = E1000_DEV_ID_82546GB_COPPER, .name = "E1000 82546GB COPPER" },
    { .id = E1000_DEV_ID_82546GB_FIBER, .name = "E1000 82546GB FIBER" },
    { .id = E1000_DEV_ID_82546GB_SERDES, .name = "E1000 82546GB SERDES" },
    { .id = E1000_DEV_ID_82546GB_PCIE, .name = "E1000 82546GB PCIE" },
    { .id = E1000_DEV_ID_82546GB_QUAD_COPPER, .name = "E1000 82546GB QUAD COPPER" },
    { .id = E1000_DEV_ID_82547EI, .name = "E1000 82547EI" },
    { .id = E1000_DEV_ID_82547EI_MOBILE, .name = "E1000 82547EI MOBILE" },
    { .id = E1000_DEV_ID_82546GB_QUAD_COPPER_KSP3, .name = "E1000 82546GB QUAD COPPER KSP3" },
    { .id = E1000_DEV_ID_INTEL_CE4100_GBE, .name = "E1000 INTEL CE4100 GBE" },
};


struct e1000_id *e1000_pci_info(struct PCI_device *pci)
{
    unsigned i;
    for (i = 0; i < sizeof(__e1000_ids)/sizeof(struct e1000_id); ++i) {
        if (__e1000_ids[i].id == pci->device_id)
            return &__e1000_ids[i];
    }

    return NULL;
}


int e1000_match_pci_device(uint16_t vendor, uint32_t class, uint16_t device)
{
    unsigned i;
    if (vendor != INTEL_VENDOR || class != ETERNET_CLASS)
        return -1;
    for (i = 0; i < sizeof(__e1000_ids)/sizeof(struct e1000_id); ++i) {
        if (__e1000_ids[i].id == device)
            return 0;
    }

    return -1;
}

void e1000_setup()
{
    struct PCI_device *pci = NULL;
    char name[48];

    for(;;) {
        pci = PCI_search2(e1000_match_pci_device);
        if (pci == NULL)
            break;

        struct e1000_id *info = e1000_pci_info(pci);
        strcpy(name, "Intel PRO/1000");
        if (info) {
            strcat(name, " ");
            strcat(name, info->name);
        }
        // name = "Intel PRO/1000 ";
        // kprintf(0, "Found %s (PCI.%02d.%02d)\n", name, pci->bus, pci->slot);
        e1000_startup(pci, name);
    }
}

void e1000_teardown()
{
}


MODULE(e1000, MOD_AGPL, e1000_setup, e1000_teardown);
// MOD_REQUIRE(pci);
