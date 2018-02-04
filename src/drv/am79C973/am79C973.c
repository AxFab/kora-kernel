#include <kernel/core.h>
#include <kernel/net.h>
#include <kernel/cpu.h>
#include <kernel/drv/pci.h>


#ifdef K_MODULE
#  define AM79C973_setup setup
#  define AM79C973_teardown teardown
#endif

#define AMD_VENDOR 0x1002
#define ETERNET_CLASS  0x020000
#define AMD_AM79C973 0x2000

#define REG_DATA  0x10
#define REG_CMD  0x12
#define REG_RESET  0x14
#define REG_BUS  0x16

#define RX_BUF_COUNT 8
#define TX_BUF_COUNT 8

struct AM79C973_mx_desc {
    page_t page;
    size_t length;
    int flags;
    bool avail;
    uint8_t *address;
};

struct AM79C973_card {
    net_ops_t ops;
    struct PCI_device *pci;
    int block;
    int rx_cur;
    int tx_cur;
    int rx_count;
    int tx_count;
    struct AM79C973_mx_desc rx_desc[RX_BUF_COUNT];
    struct AM79C973_mx_desc tx_desc[TX_BUF_COUNT];
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int AM79C973_receive(net_device_t *ndev)
{
    struct AM79C973_card *card = (struct AM79C973_card *)&ndev->ops;

    while (!(card->rx_desc[card->rx_cur].flags & 0x80000000)) {
        int idx = card->rx_cur;
        card->rx_cur = ((card->rx_cur + 1) % card->rx_count);
        if (!(card->rx_desc[idx].flags & 0x40000000)) {
            continue;
        }
        if (!(card->rx_desc[idx].flags & 0x03000000)) {
            continue;
        }

        uint32_t length = card->rx_desc[idx].flags & 0xFFF;
        uint8_t *buffer = (uint8_t *)card->rx_desc[idx].address;

        net_receive(ndev, buffer, length); // Inject new packet on network stack

        card->rx_desc[idx].flags = 0x8000F7FF;
    }
    return 0;
}

int AM79C973_send(net_device_t *ndev, sk_buffer_t *skb)
{
    struct AM79C973_card *card = (struct AM79C973_card *)&ndev->ops;

    int idx = card->tx_cur;
    card->tx_cur = ((card->tx_cur + 1) % card->tx_count);

    card->tx_desc[idx].address = (uint8_t *)skb->address;
    card->tx_desc[idx].flags = 0x8300F000 | ((-skb->length) & 0xFFF);

    PCI_write16(card->pci, REG_CMD, 0);
    PCI_write16(card->pci, REG_DATA, 0x48);
    // Check !?
    return 0;
}

int AM79C973_irq(net_device_t *ndev)
{
    struct AM79C973_card *card = (struct AM79C973_card *)&ndev->ops;

    PCI_write16(card->pci, REG_CMD, 0);
    uint16_t status = PCI_read16(card->pci, REG_DATA);

    if (status & 0x8000) {
        // Error
    } else if (status & 0x2000) {
        // Collision error
    } else if (status & 0x1000) {
        // Missed frame
    } else if (status & 0x0800) {
        // Memory error
    } else if (status & 0x0200) {
        // Sent data
    } else if (status & 0x0400) {
        // Received data
    } else if (status & 0x0100) {
        // Init completed
        AM79C973_receive(ndev);
    }

    PCI_write16(card->pci, REG_CMD, 0);
    PCI_write16(card->pci, REG_DATA, status);
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void AM79C973_activate(net_device_t *ndev)
{
    struct AM79C973_card *card = (struct AM79C973_card *)&ndev->ops;

    PCI_write16(card->pci, REG_CMD, 0);
    PCI_write16(card->pci, REG_DATA, 0x41);

    PCI_write16(card->pci, REG_RESET, 4);
    uint16_t status = PCI_read16(card->pci, REG_DATA);

    PCI_write16(card->pci, REG_CMD, 4);
    PCI_write16(card->pci, REG_DATA, status | 0xC00);

    PCI_write16(card->pci, REG_CMD, 0);
    PCI_write16(card->pci, REG_DATA, 0x42);
}

void AM79C973_reset(net_device_t *ndev)
{
    struct AM79C973_card *card = (struct AM79C973_card *)&ndev->ops;

    PCI_read16(card->pci, REG_RESET);
    PCI_write16(card->pci, REG_RESET, 0);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void AM79C973_startup(struct PCI_device *pci, const char *name)
{
    int i;
    unsigned char mac_address[6];

    ((uint16_t *)mac_address)[0] = PCI_read16(pci, 0);
    ((uint16_t *)mac_address)[1] = PCI_read16(pci, 2);
    ((uint16_t *)mac_address)[2] = PCI_read16(pci, 4);

    PCI_write16(pci, REG_CMD, 20);
    PCI_write16(pci, REG_BUS, 0x102);
    PCI_write16(pci, REG_CMD, 0);
    PCI_write16(pci, REG_DATA, 4);

    struct AM79C973_card *card = (struct AM79C973_card *)kalloc(sizeof(
                                     struct AM79C973_card));
    card->pci = pci;
    for (i = 0; i < RX_BUF_COUNT; ++i) {
        card->rx_desc[i].address = 0;
        card->rx_desc[i].flags = 0xF7FF;
        card->rx_desc[i].avail = false;
    }
    for (i = 0; i < TX_BUF_COUNT; ++i) {
        card->tx_desc[i].address = 0;
        card->tx_desc[i].flags = 0x800007FF;
        card->tx_desc[i].avail = false;
    }

    PCI_write16(pci, REG_CMD, 1);
    PCI_write16(pci, REG_DATA, (card->block & 0xFFFF));

    PCI_write16(pci, REG_CMD, 2);
    PCI_write16(pci, REG_DATA, ((card->block >> 16) & 0xFFFF));
}

void AM79C973_setup()
{
    struct PCI_device *pci;
    const char *name;

    for (;;) {
        pci = PCI_search(AMD_VENDOR, ETERNET_CLASS, AMD_AM79C973);
        name = "PCnet-Fast III (AMD AM79C973)";
        if (pci == NULL) {
            return;
        }

        kprintf(0, "Found %s\n", name);
        AM79C973_startup(pci, name);
    }
}
