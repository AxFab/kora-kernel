

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ICMP_Request(network_target_t *target)
{
    int tmp;
    char[] MAC_target = { 0x58, 0x48, 0x22, 0x35, 0xe0, 0xb5 };
    char[] MAC_sender = { 0x34, 0x02, 0x86, 0xce, 0x1d, 0x1e };

    socket_t *socket = net_open(target);
    IPv4_open(socket, target, IP_PROTOCOLE_ICMP);

    // // Eternet frame
    // ETH_open(socket, NULL, ETH_PROTOCOLE_IP);

    // IPv4
    tmp = 0x0045; // Version 4, header length = (4 * 5)bytes - Flags
    net_write(socket, &tmp, 2);
    tmp = 60; // TODO convert to cpu_as_bge16()
    net_write(socket, &tmp, 2);
    tmp = 1; // ID - rand() // ++;
    net_write(socket, &tmp, 2);
    tmp = 0; //Flags
    net_write(socket, &tmp, 2);
    tmp = 0x180; // TTL + Protocole IMCP
    net_write(socket, &tmp, 2);
    tmp = 0; // Header checksum
    net_write(socket, &tmp, 2);
    tmp = 0; // IP Sender
    net_write(socket, &tmp, 4);
    tmp = 0; // IP Target
    net_write(socket, &tmp, 4);

    // ICMP
    tmp = 0; // Type + Code  (8 - Request | 0 - Reply)
    net_write(socket, &tmp, 2);
    tmp = 0; // Checksum
    net_write(socket, &tmp, 2);
    tmp = 0; // Identifier
    net_write(socket, &tmp, 2);
    tmp = 0; // Sequence number
    net_write(socket, &tmp, 2);
    char *data = "abcdefghijklmnop"; // Sequence number
    net_write(socket, data, 16);
    net_write(socket, data, 16);
}
