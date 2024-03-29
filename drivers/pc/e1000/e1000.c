/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#include <kernel/net.h>
#include <kernel/bus/pci.h>
#include <kernel/arch.h>
#include <kernel/tasks.h>
#include <kernel/irq.h>
#include <kernel/mods.h>
#include <kernel/syscalls.h>
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

typedef struct e1000_device {
    ifnet_t *dev;
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

} e1000_device_t;

void e1000_init_hw(e1000_device_t *ifnet);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

//void e1000_link(ifnet_t *net)
//{
//    e1000_device_t *ifnet = net->drv_data;
//    e1000_init_hw(ifnet);
//}

int e1000_send(ifnet_t *net, skb_t *skb)
{
    e1000_device_t *ifnet = net->drv_data;
    struct PCI_device *pci = ifnet->pci;
    mtx_lock(&pci->mtx);
    kprintf(KL_DBG, "REQUEST SEND NETWORK %s (%d)\n", skb->log, skb->length);
    kdump2(skb->buf, skb->length);
    ifnet->tx_index = PCI_rd32(pci, 0, REG_TDT);
    // TODO -- Use semaphore to wait on available sending buffer (Wait IRQ Status 3(ICR_TXDW))

    memcpy(ifnet->tx_virt[ifnet->tx_index], skb->buf, skb->length);
    ifnet->tx_base[ifnet->tx_index].length = skb->length;
    ifnet->tx_base[ifnet->tx_index].cmd = CMD_EOP | CMD_IFCS | CMD_RS;
    ifnet->tx_base[ifnet->tx_index].status = 0;
    ifnet->tx_index = (ifnet->tx_index + 1) % ifnet->tx_count;
    PCI_wr32(pci, 0, REG_TDT, ifnet->tx_index);
    mtx_unlock(&pci->mtx);
    return 0;
}

int e1000_recv(struct PCI_device *pci, e1000_device_t *ifnet)
{
    int count = 0;
    int tail = PCI_rd32(pci, 0, REG_RDT);
    if (ifnet->rx_index != tail) {
        kprintf(-1, "[e1000] rx tail updated %d\n", tail);
        ifnet->rx_index = tail;
    }

    for (;;) {
        int head = PCI_rd32(pci, 0, REG_RDH);
        // if (ifnet->rx_index == head)
        //     break;

        kprintf(-1, "[e1000] Recv [%d-%d/%d]\n", head, ifnet->rx_index, ifnet->rx_count);
        if ((ifnet->rx_base[ifnet->rx_index].status & 1) == 0) {
            // kprintf(-1, "[e1000] not ready - rx status:\n");
            // for (int i = 0; i < ifnet->rx_count; i += 8)
            //     kprintf(-1, "[e1000] %04x - %04x - %04x - %04x - %04x - %04x - %04x - %04x\n",
            //         ifnet->rx_base[i + 0].status & 0xffff,
            //         ifnet->rx_base[i + 1].status & 0xffff,
            //         ifnet->rx_base[i + 2].status & 0xffff,
            //         ifnet->rx_base[i + 3].status & 0xffff,
            //         ifnet->rx_base[i + 4].status & 0xffff,
            //         ifnet->rx_base[i + 5].status & 0xffff,
            //         ifnet->rx_base[i + 6].status & 0xffff,
            //         ifnet->rx_base[i + 7].status & 0xffff);
            break; // Error / Not ready !?
        }

        if (ifnet->rx_base[ifnet->rx_index].errors & 0x97) {
            ifnet->rx_index = (ifnet->rx_index + 1) % ifnet->rx_count;
            kprintf(-1, "[e1000] Error bits are set on rx packet: %x\n", ifnet->rx_base[ifnet->rx_index].errors);
        } else {
            // Inject new packet on network stack rx queue
            uint8_t *buffer = ifnet->rx_virt[ifnet->rx_index];
            uint16_t length = ifnet->rx_base[ifnet->rx_index].length;
            kprintf(KL_DBG, "RECEIVE NETWORK PACKET (%d bytes)\n", length);
            net_skb_recv(ifnet->dev, buffer, length);
        }

        count++;
        ifnet->rx_base[ifnet->rx_index].status = 0;

        if (++ifnet->rx_index >= ifnet->rx_count)
            ifnet->rx_count = 0;
        PCI_wr32(pci, 0, REG_RDT, ifnet->rx_index);
        PCI_rd32(pci, 0, REG_STATUS);
    }
    return count;
}

int e1000_recv_task(e1000_device_t *ifnet)
{
    struct PCI_device *pci = ifnet->pci;
    xtime_t delay = 50000; // µs
    sleep_timer(delay);
    for (;;) {
        mtx_lock(&pci->mtx);
        e1000_recv(pci, ifnet);
        mtx_unlock(&pci->mtx);
        if (false)
            scheduler_switch(TS_READY);
        else
            sleep_timer(delay);
    }
}

int e1000_irq_handler(e1000_device_t *ifnet)
{
    // kprintf(KL_DBG, "[e1000] IRQ %s\n", ifnet->name);
    struct PCI_device *pci = ifnet->pci;
    // mtx_lock(&pci->mtx);
    uint32_t status = PCI_rd32(pci, 0, REG_ICR);
    PCI_wr32(pci, 0, REG_ICR, 0);

    kprintf(KL_DBG, "[e1000] IRQ %s - status %x\n", ifnet->name, status);
    if (status == 0) {
        // mtx_unlock(&pci->mtx);
        return -1;
    }

    if (status & ICR_TXDW) {
        kprintf(KL_DBG, "[e1000] %s - Packet transmited with success\n", ifnet->name);
    }
    if (status & ICR_LSC) {
        // ifnet->dev.flags |= NET_CONNECTED;
        int link_up = PCI_rd32(pci, 0, REG_STATUS) & (1 << 1);
        kprintf(KL_DBG, "[e1000] %s - Link status change: %d!\n", ifnet->name, link_up);
    }
    if (status & ICR_RXDMT0) {
        kprintf(KL_DBG, "[e1000] %s - Min thresohold RX hit \n", ifnet->name);
        // Send ICMP command 3 !?
    }
    if (status & ICR_RXO)
        kprintf(KL_DBG, "[e1000] %s - Receive buffers overrun\n", ifnet->name);

    if (status & ICR_RXT0) {
        e1000_recv(pci, ifnet);
    }

    // e1000_recv(pci, ifnet);
    // mtx_unlock(&pci->mtx);
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// static void e1000_irq_disable(e1000_device_t *ifnet)
// {
//     struct PCI_device *pci = ifnet->pci;
//     PCI_wr32(pci, 0, REG_IMC, ~0);
//     PCI_rd32(pci, 0, REG_STATUS); /* Flush */

// }

// static void e1000_irq_enable(e1000_device_t *ifnet)
// {
//     struct PCI_device *pci = ifnet->pci;
//     PCI_wr32(pci, 0, REG_IMS, ~0);
//     PCI_rd32(pci, 0, REG_STATUS); /* Flush */
// }

static uint32_t e1000_eeprom_read(struct PCI_device *pci, uint8_t addr)
{
    bool has_eeprom = false;
    uint16_t data = 0;
    uint32_t tmp = 0;
    if (has_eeprom)
    {
        PCI_wr32(pci, 0, REG_EERD, ((uint32_t)addr << 8) | 1);
        while(!((tmp = PCI_rd32(pci, 0, REG_EERD)) & (1 << 4)));
    }
    else
    {
        PCI_wr32(pci, 0, REG_EERD, ((uint32_t)addr << 2) | 1);
        while(!((tmp = PCI_rd32(pci, 0, REG_EERD)) & (1 << 1)));
    }
    data = (uint16_t)(tmp >> 16) & 0xFFFF;
    return data;
}


static int e1000_read_mac(struct PCI_device *pci, uint8_t *mac)
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

void e1000_init_hw(e1000_device_t *ifnet)
{
    int i;
    struct PCI_device *pci = ifnet->pci;

    //
    // Start link
    // Clear Multicast table ...
    // Register Interrupt
    // Enable Interrupt
    // Rx Init
    // Tx Init

    irq_register(pci->irq, (irq_handler_t)e1000_irq_handler, ifnet);

    e1000_rx_init(ifnet);
    e1000_tx_init(ifnet);
    ifnet->mx_count = ifnet->rx_count + ifnet->tx_count;

    // Reset
    PCI_wr32(pci, 0, REG_CTRL, (1 << 26));
    sleep_timer(SEC_TO_USEC(2));

    uint32_t status = PCI_rd32(pci, 0, REG_CTRL);
    status |= (1 << 5);   /* set auto speed detection */
    status |= (1 << 6);   /* set link up */
    status &= ~(1 << 3);  /* unset link reset */
    status &= ~(1 << 31); /* unset phy reset */
    status &= ~(1 << 7);  /* unset invert loss-of-signal */
    PCI_wr32(pci, 0, REG_CTRL, status);
    sleep_timer(SEC_TO_USEC(2));

    /* Disables flow control */
    // PCI_wr32(pci, 0, REG_FEXTNVM, 0);
    // PCI_wr32(pci, 0, REG_FCAH, 0);
    // PCI_wr32(pci, 0, REG_FCT, 0);
    // PCI_wr32(pci, 0, REG_FCTTV, 0);
    PCI_wr32(pci, 0, REG_FEXTNVM, 0x002C8001);
    PCI_wr32(pci, 0, REG_FCAH, 0x0100);
    PCI_wr32(pci, 0, REG_FCT, 0x8808);
    PCI_wr32(pci, 0, REG_FCTTV, 0xFFFF);

    /* Unset flow control */
    // status = PCI_rd32(pci, 0, REG_CTRL);
    // status &= ~(1 << 30);
    // PCI_wr32(pci, 0, REG_CTRL, status);

    /* Wait */
    sleep_timer(MSEC_TO_USEC(50)); // 1 sec

    // kprintf(0, "Check E1000 IRQ = %d\n", PCI_cfg_rd16(pci, PCI_INTERRUPT_LINE) & 0xFF);



    /* Clear Multicast Table Array (MTA). */
    for (i = 0; i < 0x80; i++)
        PCI_wr32(pci, 0, REG_MTA + i * 4, 0);
    /* Initialize statistics registers. */
    for (i = 0; i < 0x40; i++)
        PCI_wr32(pci, 0, REG_CRCERRS + i * 4, 0);

    PCI_wr32(pci, 0, REG_CTRL, (1 << 4));


    // Descriptor must be 16bits aligned, length must be 128b aligned! Min is 8 descriptor!
    PCI_wr32(pci, 0, REG_RDBAL, (uint32_t)((uint64_t)ifnet->rx_phys & 0xFFFFFFFF));
    PCI_wr32(pci, 0, REG_RDBAH, (uint32_t)((uint64_t)ifnet->rx_phys >> 32));
    PCI_wr32(pci, 0, REG_RDLEN, NUM_RX_DESC * 16);
    PCI_wr32(pci, 0, REG_RDH, 0);
    PCI_wr32(pci, 0, REG_RDT, NUM_RX_DESC - 1);
    // PCI_wr32(pci, 0, REG_RCTL, RCTL_EN
    //           | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE
    //           | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_2048);
    PCI_wr32(pci, 0, REG_RCTL, RCTL_EN |
             (PCI_rd32(pci, 0, REG_RCTL) & (~((1 << 17) | (1 << 16)))));


    PCI_wr32(pci, 0, REG_TDBAL, (uint32_t)((uint64_t)ifnet->tx_phys & 0xFFFFFFFF));
    PCI_wr32(pci, 0, REG_TDBAH, (uint32_t)((uint64_t)ifnet->tx_phys >> 32));
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

    PCI_wr32(pci, 0, REG_RDTR, 0); // Dealy for rx !? Use IRQ !?
    PCI_wr32(pci, 0, REG_ITR, 500);
    PCI_rd32(pci, 0, REG_STATUS);

    /* Twiddle interrupts */

    // PCI_wr32(pci, 0, REG_IMS, 0xFF);
    // PCI_wr32(pci, 0, REG_IMC, 0xFF);
    // int ints = ICR_LSC | ICR_RXO | ICR_RXT0 | ICR_TXQE | ICR_TXDW | ICR_ACK | ICR_RXDMT0 | ICR_SRPD;
    // int ints = ICR_TXDW | ICR_TXQE | ICR_LSC | ICR_RXO | ICR_RXT0;
    // PCI_wr32(pci, 0, REG_IMS, ints);
    // Enable Interrupt
    PCI_wr32(pci, 0, REG_IMS, 0x1FFFF);
    // PCI_wr32(pci, 0, REG_IMS, 0xFB); // 0xFF & ~4
    PCI_rd32(pci, 0, REG_ICR);


    /* Wait */
    sleep_timer(SEC_TO_USEC(1)); // 1 sec

    status = PCI_rd32(pci, 0, REG_STATUS); // & (1 << 1);
    if (status & (1 << 1)) {
        ifnet->dev->flags |= NET_CONNECTED;
        net_event(ifnet->dev, NET_EV_LINK, 0);
        kprintf(0, "[e1000] %s, DONE %x \n", ifnet->name, status);
    }
    int sp = (status >> 6) & 3;
    static char *spd[] = { "10Mb/s", "100Mb/s", "1Gb/s", "1Gb/s" };
    kprintf(0, "[e1000] %s, Speed: %s\n", ifnet->name, spd[sp]);
}

void e1000_init_hw_task(e1000_device_t *ifnet)
{
    struct PCI_device *pci = ifnet->pci;
    mtx_lock(&pci->mtx);
    xtime_t delay = 5000000; // µs
    e1000_init_hw(ifnet);
    mtx_unlock(&pci->mtx);
    for (;;)
        sleep_timer(delay);
}


net_ops_t e1000_ops = {
    .send = e1000_send,
    .link = e1000_init_hw,
};


/* Allocate buffer for receive descriptors. */
void e1000_rx_init(e1000_device_t *ifnet)
{
    ifnet->rx_base = kmap(PAGE_SIZE, NULL, 0, VM_RW | VM_RESOLVE);
    ifnet->rx_phys = mmu_read((size_t)ifnet->rx_base);
    ifnet->rx_count = MIN(NUM_RX_DESC, PAGE_SIZE / sizeof(struct rx_desc));

    uint8_t *map = kmap(PAGE_SIZE * ifnet->rx_count, NULL, 0, VM_RW | VM_RESOLVE);
    for (int i = 0; i < ifnet->rx_count; ++i) {
        ifnet->rx_virt[i] = &map[PAGE_SIZE * i];
        ifnet->rx_base[i].addr = mmu_read((size_t)ifnet->rx_virt[i]);
        // kprintf(0, "rx[%d] 0x%x -> 0x%x\n", i, ifnet->rx_virt[i], ifnet->rx_base[i].addr);
        ifnet->rx_base[i].status = 0;
    }

    // Commands !?
}

/* Allocate buffer for transfert descriptors. */
void e1000_tx_init(e1000_device_t *ifnet)
{
    ifnet->tx_base = kmap(PAGE_SIZE, NULL, 0, VM_RW | VM_RESOLVE);
    ifnet->tx_phys = mmu_read((size_t)ifnet->tx_base);
    ifnet->tx_count = MIN(NUM_TX_DESC, PAGE_SIZE / sizeof(struct tx_desc));

    uint8_t *map = kmap(PAGE_SIZE * ifnet->tx_count, NULL, 0, VM_RW | VM_RESOLVE);
    for (int i = 0; i < ifnet->tx_count; ++i) {
        ifnet->tx_virt[i] = &map[PAGE_SIZE * i];
        ifnet->tx_base[i].addr = mmu_read((size_t)ifnet->tx_virt[i]);
        // kprintf(0, "tx[%d] 0x%x -> 0x%x\n", i, ifnet->tx_virt[i], ifnet->tx_base[i].addr);
        ifnet->tx_base[i].status = 0;
        ifnet->tx_base[i].cmd = (1 << 0);
    }

    // Commands !?
}


void e1000_startup(struct PCI_device *pci, const char *name)
{
    int i;
    e1000_device_t *ifnet = (e1000_device_t *)kalloc(sizeof(e1000_device_t));

    ifnet->pci = pci;
    ifnet->name = strdup(name);
    splock_init(&ifnet->lock);
    mtx_lock(&pci->mtx);

    pci->bar[0].mmio = (uint32_t)kmap(pci->bar[0].size, NULL, pci->bar[0].base & ~7, VM_RW | VMA_PHYS | VM_UNCACHABLE);
    // kprintf(KL_DBG, "%s MMIO mapped at %x\n", name, pci->bar[0].mmio);

    /* PCI Init command */
    uint16_t cmd = PCI_cfg_rd16(pci, PCI_COMMAND);
    cmd |= (1 << 2) | (1 << 0);
    PCI_cfg_wr32(pci, PCI_COMMAND, cmd);

    sleep_timer(MSEC_TO_USEC(500));

    /* Find MAC address */
    uint8_t hwaddr[6];
    if (e1000_read_mac(pci, hwaddr) != 0) {
        mtx_unlock(&pci->mtx);
        // TODO -- Clean
        return;
    }

    /* initialize */
    ifnet_t *net = net_device(net_stack(), NET_AF_ETH, hwaddr, &e1000_ops, ifnet);
    if (net == NULL) {
        kprintf(-1, "[e1000] Error creating ethernet device: %s\n", name);
        kfree(ifnet);
        mtx_unlock(&pci->mtx);
        return;
    }

    ifnet->dev = net;
    net->mtu = 1500; // Wild guess! TODO - Support for jumbo frames
    char tmp[32];
    snprintf(tmp, 32, "e1000.eth%d.init", net->idx);
    task_start(tmp, e1000_init_hw_task, ifnet);
    mtx_unlock(&pci->mtx);
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


int e1000_match_pci_device(uint16_t vendor, uint32_t class, uint16_t device)
{
    unsigned i;
    if (vendor != INTEL_VENDOR || class != ETERNET_CLASS)
        return -1;
    for (i = 0; i < sizeof(__e1000_ids) / sizeof(struct e1000_id); ++i) {
        if (__e1000_ids[i].id == device)
            return i;
    }

    return -1;
}

void e1000_setup()
{
    struct PCI_device *pci = NULL;
    char name[48];

    int i;
    for (;;) {
        pci = pci_search(e1000_match_pci_device, &i);
        if (pci == NULL)
            break;

        struct e1000_id *info = &__e1000_ids[i];
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


EXPORT_MODULE(e1000, e1000_setup, e1000_teardown);
