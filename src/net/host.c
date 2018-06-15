/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#include <kora/hmap.h>
#include <string.h>

typedef struct host host_t;

struct host {
	uint8_t mac[ETH_ALEN];
	uint8_t ip[IP4_ALEN];
	char *hostname;
	char *domain;
	int trust_mac;
	int trust_ip;
	int trust_fqdn;
	uint64_t updated;
};

HMP_map host_mac;
HMP_map host_ip;
HMP_map host_fqdn;

static void host_update(host_t *host)
{
}

void host_init()
{
	hmp_init(&host_mac, 8);
	hmp_init(&host_ip, 8);
	hmp_init(&host_fqdn, 8);
}

void host_register(uint8_t *mac, uint8_t *ip, const char *hostname, const char *domain, int trust)
{
	char bufm[18];
	char bufi[16];
	host_t *host = NULL;
	kprintf(KLOG_DBG, "HOST.%d: %s, %s, %s.%s\n", trust, net_ethstr(bufm, mac), net_ip4str(bufi, ip), hostname, domain);

	host = (host_t*)kalloc(sizeof(host_t));
	memcpy(host->mac, mac, ETH_ALEN);
	memcpy(host->ip, ip, IP4_ALEN);
	host->hostname = strdup(hostname);
	host->domain = strdup(domain);
	host->trust_mac = trust;
	host->trust_ip = trust;
	host->trust_fqdn = trust;

	hmp_put(&host_mac, mac, ETH_ALEN, host);
	hmp_put(&host_ip, ip, IP4_ALEN, host);

}

int host_mac_for_ip(uint8_t *mac, const uint8_t *ip, int trust)
{
	host_t *host = hmp_get(&host_ip, ip, IP4_ALEN);
	if (host == NULL || host->trust_mac < trust)
	    return -1;
	memcpy(mac, host->mac, ETH_ALEN);
	return 0;
}

