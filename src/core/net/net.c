#include <kernel/net.h>
#include <kora/mcrs.h>
#include <string.h>
#include <errno.h>


void ETH_init(net_device_t *ndev);
void IPv4_init(net_device_t *ndev);


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

net_device_t *eth;

net_device_t *net_register(const char *name, net_ops_t *ops,
                           const unsigned char *address, int length)
{
    if (unlikely(length > NET_ADDR_MAX_LG)) {
        errno = EADDRNOTAVAIL;
        return NULL;
    }
    net_device_t *ndev = (net_device_t *)kalloc(sizeof(net_device_t *));
    strncpy(ndev->name, name, DEV_NAME_SZ);
    ndev->state = 0;
    // ndev->index = kNET.;
    ndev->ops = ops;

    ndev->addr_length = length;
    memcpy(ndev->hw_address, address, length);

    ETH_init(ndev);
    IPv4_init(ndev);
    return ndev;
}

void net_receive(net_device_t *dev, const unsigned char *buffer, int length)
{

}


sk_buffer_t *net_skb(socket_t *socket)
{
    sk_buffer_t *skb = (sk_buffer_t *)kalloc(sizeof(sk_buffer_t));
    skb->address = kmap(PAGE_SIZE, NULL, 0, VMA_FG_ANON | VMA_RESOLVE);
    skb->page = mmu_read((size_t)skb->address, false, false);
    skb->socket = socket;
    return skb;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

PACK(struct ETH_header {
    unsigned char target[6];
    unsigned char sender[6];
    unsigned short protocole;
});

PACK(struct ARP_header {
    unsigned short hardware_type;
    unsigned short protocole_type;
    unsigned char hardware_address_length;
    unsigned char protocole_address_length;
    unsigned short operation;
});

void ARP_who_has(net_device_t *ndev, const unsigned char *target,
                 const unsigned char *qry)
{
    sk_buffer_t *skb = net_skb(NULL);
    int lg = sizeof(struct ETH_header) + sizeof(struct ARP_header) + 20;
    struct ETH_header *eth_header = (struct ETH_header *)skb->address;
    struct ARP_header *arp_header = (struct ARP_header *)&skb->address[sizeof(
                                        struct ETH_header)];

    // Ethernet
    if (target == NULL) {
        memset(eth_header->target, 0xFF, 6);
    } else {
        memcpy(eth_header->target, target, 6);
    }
    memcpy(eth_header->sender, ndev->protocoles[NET_PROTO_ETHERNET]->address, 6);
    eth_header->protocole = 0x0608; // ETH_PROTOCOLE_ARP  __be16(0x0806)
    // __be16(x)    ((uint16_t)(((uint16_t)(X)&0xff)<<8)|(((uint16_t)(X)>>8)&0xff))

    // ARP
    arp_header->hardware_type = 0x100; // Use eternet
    arp_header->protocole_type = 0x8; // Use IPv4
    arp_header->hardware_address_length = 6;
    arp_header->protocole_address_length = 4;
    arp_header->operation = 0x100; // ARP Request 1 : ARP Reply 2
    memcpy(&skb->address[lg - 20], ndev->protocoles[NET_PROTO_ETHERNET]->address,
           6);
    memcpy(&skb->address[lg - 14], ndev->protocoles[NET_PROTO_IPv4]->address, 4);
    memset(&skb->address[lg - 10], 0, 6);
    memcpy(&skb->address[lg - 4], qry, 4);

    // int pk_lg = ALIGN_UP(lg, 64);
    // uint32_t crc = crc32(skb->address, pk_lg - 4);
    // memcpy(&skb->address[pk_lg - 4], &crc, 4);
    skb->length = lg;
    ndev->ops->send(ndev, skb);
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void ETH_init(net_device_t *ndev)
{
    if (ndev->addr_length != 6 || ndev->protocoles[NET_PROTO_ETHERNET]) {
        return;
    }

    net_layer_info_t *info = (net_layer_info_t *)kalloc(sizeof(net_layer_info_t));
    info->addr_length = 6;
    memcpy(info->address, ndev->hw_address, 6);
    memset(info->broadcast, 0xFF, 6);
    info->status = NET_ST_READY;
    ndev->protocoles[NET_PROTO_ETHERNET] = info;
}

void IPv4_init(net_device_t *ndev)
{
    if (ndev->protocoles[NET_PROTO_ETHERNET] == NULL ||
            ndev->protocoles[NET_PROTO_IPv4]) {
        return;
    }

    net_layer_info_t *info = (net_layer_info_t *)kalloc(sizeof(net_layer_info_t));
    info->addr_length = 4;
    info->address[0] = 192;
    info->address[1] = 178;
    info->address[2] = 0;
    info->address[3] = 1;
    memset(info->broadcast, 0xFF, 4);
    info->status = NET_ST_INIT;
    ndev->protocoles[NET_PROTO_IPv4] = info;

    asm("sti");
    ARP_who_has(ndev, NULL, info->address);
}
