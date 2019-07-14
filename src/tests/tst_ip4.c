
void ip4_connect(socket_t *socket, uint8_t *ip)
{
    ip4_cnx_t *cnx = NULL;
    // look on route cache table
    ip4_route_t *route = NULL;
    socket->packet = ip4_socket_packet;
    for ll_each(&ip4_cnxs, cnx, ip4_cnx_t, node) {
	if (ip4_check_cnx(cnx, ip))
	    break;
    }
    if (cnx != NULL) {
	// Send ARP request
	route = ip4_arp_whois(cnx->ifnet, ip, 3, 500);
	socket->route = route;
	return route ? 0 : -1;
    }

    // Look for a gateway
	socket->route = route;
	return route ? 0 : -1;
}

void ip4_config(ifnet_t *ifnet, uint8_t *addr, uint8_t *mask, uint8_t *gateway)
{
    ip4_cnx_t *cnx = NULL;
    for ll_each(&ip4_cnxs, cnx, ip4_cnx_t, node) {
	if (cnx->ifnet == ifnet)
	    break;
    }


    if (cnx == NULL) {
	cnx = kalloc(sizeof(ip4_cnx_t));
	cnx->ifnet =ifnet;
	ll_append(&ip4_cnxs, &cnx->node);
    }

    memcpy(cnx->address, addr, IP4_ALEN);
    memcpy(cnx->mask, mask, IP4_ALEN);
    memcpy(cnx->gateway, gateway, IP4_ALEN);
    
}

// ============

void tst_ip4_01()
{
    uint8_t ad0[] = { 192, 168, 10, 1 };
    uint8_t ad1[] = { 192, 168, 10, 6 };
    uint8_t adm[] = { 192, 168, 10, 10 };
    uint8_t adn[] = { 192, 168, 10, 20 };
    uint8_t mask[] = { 255, 255, 252, 0 };
    uint8_t zr[] = { 0, 0, 0, 0 };

    inet_t instances[3];
    lnet_t *ntx = lnet_switch(8);
    ifnet_t *if0 = lnet_endpoint(ntk, &instances[0]);
    ifnet_t *if1 = lnet_endpoint(ntk, &instances[1]);
    ifnet_t *if2 = lnet_endpoint(ntk, &instances[2]);

    ip4_config(if0, ad0, mask, zr);
    ip4_config(if1, ad1, mask, zr);
    dhcp_server(if0, adm, adn);

    // Cant send ping
    
    dhcp_discover(if2);

    // Can send ping to if1, received pong
    int ret = icmp_pong(if2, ad1, 500);
}

int main()
{
    ifnet_t *ifnet = kalloc();
    ifnet->type = ETH

    ip4_config(ifnet, addr, mask, gateway);

    socket_t *sock = net_socket();

    ip4_connect(sock, ip);

    net_socket_write(sock, "Hello\n", 6);

}
