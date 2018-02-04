#include <kernel/net.h>

#define ETH_PROTOCOLE_ARP  ENDIAN_FROM_BGE(0x0806)
#define ETH_PROTOCOLE_IPv4  ENDIAN_FROM_BGE(0x0806)
#define ETH_PROTOCOLE_IPv6  ENDIAN_FROM_BGE(0x0806)

int ARP_read(socket_t *socket);
int IPv4_read(socket_t *socket);
int IPv6_read(socket_t *socket);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static char *ETH_broadcast = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

int ETH_open(socket_t *socket, network_target_t *target, unsigned protocole)
{
    if (target) {
        net_write(socket, target->mac_address, 6);
    } else {
        net_write(socket, ETH_broadcast, 6);
    }
    net_write(socket, socket->card->mac_address, 6);
    net_write(socket, &protocole, 2);
    return 0;
}

int ETH_read(socket_t *socket)
{
    int protocole;
    char targeted[6];
    net_read(socket, targeted, 6);
    net_read(socket, socket->mac_address, 6);
    net_read(socket, &protocole, 2);
    switch (protocole) {
    case ETH_PROTOCOLE_ARP:
        return ARP_read(socket);
    case ETH_PROTOCOLE_IPv4:
        return IPv4_read(socket);
    case ETH_PROTOCOLE_IPv6:
        return IPv6_read(socket);
    default:
        return -1;
    }
}
