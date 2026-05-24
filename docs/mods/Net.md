# Network Stack (Net)

**Source:** `src/net/` (core), `drivers/net/ip4/` (IPv4 and above)  
**Header:** `include/kernel/net.h`  
**Test harness:** `make cli-net` → `tests/net_*.sh`

---

## Purpose

The network subsystem provides a layered protocol stack from Ethernet
framing up to UDP sockets.  It is designed to be instantiated multiple
times as independent `netstack_t` objects, which allows the CLI test
harness to simulate several machines on a virtual network without
running multiple processes.

---

## Design overview

### Stack model

Everything is anchored to a `netstack_t`.  The kernel creates exactly
one stack at boot via `net_setup()`.  Each task stores a pointer
`task->net` that is currently always the same global stack.

```
netstack_t
  ├── list of ifnet_t   (registered network interfaces)
  ├── bbtree of nproto_t (registered protocols, keyed by AF)
  ├── rx_list / rx_sem  (received packet queue + semaphore)
  └── handlers          (event listener list)
```

### Network interface — `ifnet_t`

Each physical or virtual NIC is represented by an `ifnet_t`.  It holds
the hardware address (`hwaddr`), MTU, statistics counters, a pointer to
the driver operations (`net_ops_t`), and a reference back to the stack.
The driver calls `net_device()` to register, then calls `net_event()` to
signal link-up/link-down.

### Packet buffer — `skb_t`

A socket kernel buffer (`skb_t`) is a variable-length struct with a
fixed header followed by a byte array.  A `pen` cursor tracks the
current read position; `net_skb_reserve()` advances the write cursor and
returns a pointer to the newly reserved region.

**TX flow:**
1. Upper protocol calls `net_packet(ifnet)` → allocates `skb_t`.
2. Each protocol layer calls `net_skb_reserve()` to write its header.
3. `net_skb_send()` passes the packet to `ifnet->ops->send()`.

**RX flow:**
1. Driver ISR calls `net_skb_recv(ifnet, buf, len)` → allocates `skb_t`,
   copies data, enqueues to `stack->rx_list`, releases `rx_sem`.
2. `net_deamon()` (a kernel thread) wakes, dequeues, and calls
   `nproto_t::receive()` for the matching protocol.

### Protocol registry — `nproto_t`

Protocols are registered with `net_set_protocol(stack, AF_*, proto)`.
The `nproto_t` vtable provides:
- `receive(skb)` — process an incoming packet
- `socket(sock, method)` — initialise a new socket
- `bind` / `connect` / `send` / `recv` / `close` — socket operations
- `accept(sock, model, skb)` — create a client socket from a listening one
- `clear(stack, ifnet)` / `teardown(stack)` — cleanup

### Socket — `socket_t`

A socket is a logical endpoint.  It holds local and remote addresses
(`laddr`, `raddr`), a reference to the protocol, an incoming packet
queue (`lskb`), and a semaphore for blocking receive.

---

## Protocol stack

```
Application (syscalls)
     │
  socket_t   ←→   net_socket_send / net_socket_recv
     │
  nproto_t (UDP / TCP / ICMP)
     │
  ip4.c  →  ip4_header() / ip4_receive()
     │
  arp.c  →  ARP resolution cache
     │
  eth.c  →  eth_header() / eth_receive()
     │
  ifnet_t  →  net_ops_t::send() / net_skb_recv()
     │
  NIC driver (e.g. drivers/pc/e1000/)
```

The loopback device (`src/net/lo.c`) follows the same path but short-circuits
the hardware step.

---

## IPv4 module (`drivers/net/ip4/`)

| File | Responsibility |
|---|---|
| `ip4.c` | IP header write/read, protocol dispatch |
| `ip4_mod.c` | Module setup, IP address configuration |
| `ip4_sock.c` | Port allocation and socket lookup |
| `route.c` | Routing table (find interface + gateway for a destination IP) |
| `arp.c` | ARP request/reply, address-to-MAC resolution cache |
| `icmp.c` | ICMP echo request/reply (ping) |
| `dhcp.c` | DHCP client (discover → offer → request → ack) |
| `udp.c` | UDP socket implementation (working) |
| `tcp.c` | TCP header parsing and port allocation only — **no state machine** |
| `dns.c` | Stub — not implemented |

### ARP

ARP requests are sent via `arp_request()` and the response is awaited
using a `net_qry_t` (condition variable + timeout).  A small in-memory
cache maps IP addresses to MAC addresses.

### DHCP

The DHCP client sends DISCOVER, waits for OFFER, sends REQUEST, and
configures the interface on ACK.  Lease renewal and DECLINE paths have
open TODOs.

### UDP sockets

`udp_socket()` registers a new socket.  `udp_send()` writes the UDP
header, then calls `ip4_header()`.  `udp_recv()` awaits an incoming
packet via `net_socket_pull()`.

---

## Known issues and limitations

- **TCP**: No connection state machine.  Only header parsing exists.
- **DNS**: Not implemented.
- **NTP**: Not implemented.
- **ARP**: No poisoning protection; no unsolicited ARP detection.
- **DHCP**: Lease validation and DECLINE path incomplete.
- **Routing**: No routing table persistence; single-interface routing only.
- **IPv6**: Not implemented; no design planned.
