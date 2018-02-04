

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

struct ARP_header {
    unsigned short hardware_type;
    unsigned short protocole_type;
    unsigned char hardware_address_length;
    unsigned char protocole_address_length;
    unsigned short operation;
};



void ARP_request(network_target *target)
{
    struct ARP_header head;
    // Request should be broadcast...
    socket_t *socket = net_open();
    ETH_open(socket, NULL, ETH_PROTOCOLE_ARP);

    head.hardware_type = 1; // Use eternet
    head.protocole_type = 0x800; // Use IPv4
    head.hardware_address_length = 6;
    head.protocole_address_length = true ? 4 : 16;
    head.operation = 1; // ARP Request 1 : ARP Reply 2
    net_write(socket, &head, sizeof(head));
    net_write(socket, socket->card->mac_address, 6);
    net_write(socket, socket->card->ipv4_address, 4);
    net_write(socket, ETH_broadcast, 6);
    net_write(socket, target->ipv4_address, 4);
}

void ARP_reply(network_target *target)
{
    struct ARP_header head;
    // Request should be broadcast...
    socket_t *socket = net_open();
    ETH_open(socket, target, ETH_PROTOCOLE_ARP);

    head.hardware_type = 1; // Use eternet
    head.protocole_type = 0x800; // Use IPv4
    head.hardware_address_length = 6;
    head.protocole_address_length = true ? 4 : 16;
    head.operation = 2; // ARP Request 1 : ARP Reply 2
    net_write(socket, &head, sizeof(head));
    net_write(socket, socket->card->mac_address, 6);
    net_write(socket, socket->card->ipv4_address, 4);
    net_write(socket, target->mac_address, 6);
    net_write(socket, target->ipv4_address, 4);
}

int ARP_read(socket_t *socket)
{
    struct ARP_header head;
    net_read(socket, &head, sizeof(head));

    // TODO -- check type of request / reply
    net_read(socket, NULL, 6);

}
