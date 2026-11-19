# Network Drivers (`src/drivers/network.h`, `src/drivers/ne2000.c`, `src/net/`)

## Overview

RoxiP includes an optional network stack for basic Ethernet/IP and Wi-Fi
communication. Network support is **disabled by default** and must be
explicitly enabled at build time due to the added complexity and potential
security implications.

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
| Network Core | `src/net/net.c` | Interface management, packet TX/RX, boot connect |
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

## Wi-Fi Configuration via YAFS (`@net` Nodes)

RoxiP stores Wi-Fi credentials as YAFS synthetic nodes in the storage file
(`roxip.storage`). These can be set using the `roxip-cli` host tool or by
reading them from the OS shell.

### Setting Wi-Fi Credentials

Using the host CLI tool:
```bash
./roxip-cli write roxip.storage @net:name "MyWiFiSSID"
./roxip-cli write roxip.storage @net:pass "MyPassword123"
```

Or from within RoxiP's shell (once the YAFS file system is mounted):
```
perf > echo @net:name "MyWiFiSSID"
perf > echo @net:pass "MyPassword123"
```

### Boot-Time Connection

During the boot process, RoxiP checks for `@net:name` and `@net:pass` in YAFS
storage. If both are present, it:

1. Displays a "Connecting to network..." screen with a progress indicator
2. Attempts to connect to the specified Wi-Fi network
3. On success, generates and stores `@net:id` (an obfuscated network identifier)
4. Proceeds to the interactive bootloader

During the connection process, the main operating system is not accessible.
A connecting screen is shown until the connection attempt completes.

### `@net` Synthetic Nodes

| Node | Description | Writable |
|------|-------------|----------|
| `@net:name` | Wi-Fi network name (SSID) | Yes |
| `@net:pass` | Wi-Fi password | Yes |
| `@net:id` | Network identifier (auto-generated on connect) | Read-only |

### Example Boot Sequence with Networking

```
[BOOT DBG] [08b] Initializing FAT32 Read-Only Driver...
[BOOT DBG] [08c] Initializing Network Connection...
[BOOT DBG] [OK] Network credentials found - attempting connection
[BOOT DBG] [OK] Connected to network
[BOOT DBG] [OK] Network ID stored: ROXIP-NET-123456789
[BOOT DBG] [12/12] Launching Interactive Bootloader...
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
- Wi-Fi credentials are stored securely
- The NE2000 driver is treated as experimental

## API Reference

| Function | Description |
|----------|-------------|
| `net_init()` | Initialize network stack |
| `net_if_register()` | Register a network interface |
| `net_if_up()` / `net_if_down()` | Bring interface up/down |
| `net_rx()` / `net_tx()` | Receive/transmit packets |
| `network_boot_connect()` | Connect to Wi-Fi during boot |
| `arp_resolve()` / `arp_request()` | ARP address resolution |
| `ip_output()` | Send IP packet |
| `icmp_echo()` | Send ICMP echo request (ping) |
| `udp_output()` | Send UDP datagram |

## `@net` Node API

| Node | Type | Description |
|------|------|-------------|
| `@net:name` | Read/Write | Wi-Fi SSID |
| `@net:pass` | Read/Write | Wi-Fi password |
| `@net:id` | Read-only | Network identifier (auto-generated) |
