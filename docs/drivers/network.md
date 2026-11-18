# Network Drivers (`src/drivers/network.h`, `src/drivers/ne2000.c`, `src/net/`)

## Overview

RoxiP includes an optional network stack for basic Ethernet/IP communication.
Network support is **disabled by default** and must be explicitly enabled at
build time due to the added complexity and potential security implications.

## Build Configuration

Networking is controlled by the `NETWORK_ENABLED` build flag:

### Make
```bash
# Build without networking (default)
make

# Build with networking enabled
make NETWORK_ENABLED=1

# Build a specific board with networking
make NETWORK_ENABLED=1 rockchip
make NETWORK_ENABLED=1 amlogic
make NETWORK_ENABLED=1 riscv-virt
```

### Meson
```bash
# Build without networking (default)
meson setup builddir --cross-file cross-arm-none-eabi.txt -Dboard=versatilepb

# Build with networking enabled
meson setup builddir --cross-file cross-arm-none-eabi.txt -Dboard=versatilepb -Dnetwork-enabled=true
```

## Network Stack Architecture

The network stack is layered:

| Layer | File | Description |
|-------|------|-------------|
| Network Device | `src/drivers/ne2000.c` | NE2000-compatible Ethernet driver |
| Network Core | `src/net/net.c` | Interface management, packet TX/RX |
| ARP | `src/net/arp.c` | Address Resolution Protocol |
| IP | `src/net/ip.c` | Internet Protocol |
| ICMP | `src/net/icmp.c` | Internet Control Message Protocol (ping) |
| UDP | `src/net/udp.c` | User Datagram Protocol |

## Supported Hardware

| Driver | Hardware | QEMU Emulation |
|--------|----------|----------------|
| NE2000 | NE2000-compatible Ethernet | `-device ne2000_pci,netdev=net0 -netdev user,id=net0` |

## Example: QEMU with Networking

```bash
# Build with networking
make NETWORK_ENABLED=1

# Run QEMU with NE2000 and user networking
qemu-system-arm -M versatilepb -cpu arm926 -m 256M \
  -kernel roxip.bin \
  -drive file=roxip.storage,if=pflash,format=raw \
  -vga none -serial stdio -audio driver=none -display none \
  -device ne2000_pci,netdev=net0 \
  -netdev user,id=net0
```

## Security Considerations

Network support is disabled by default for the following reasons:

1. **Attack surface**: Network code increases the potential attack surface
2. **Complexity**: Network stacks are complex and prone to implementation bugs
3. **Resource usage**: Network buffers and state consume limited RAM
4. **Target audience**: RoxiP is primarily a local/standalone OS

When enabling networking, ensure:
- The network is isolated (no direct internet exposure)
- Only trusted networks are used
- The NE2000 driver is treated as experimental

## API Reference

| Function | Description |
|----------|-------------|
| `net_init()` | Initialize network stack |
| `net_if_register()` | Register a network interface |
| `net_if_up()` / `net_if_down()` | Bring interface up/down |
| `net_rx()` / `net_tx()` | Receive/transmit packets |
| `arp_resolve()` / `arp_request()` | ARP address resolution |
| `ip_output()` | Send IP packet |
| `icmp_echo()` | Send ICMP echo request (ping) |
| `udp_output()` | Send UDP datagram |
