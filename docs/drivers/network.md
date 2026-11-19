# Network Drivers (`src/drivers/network.h`, `src/drivers/smc91c111.c`, `src/net/`)

## Overview

RoxiP includes an optional network stack for basic Ethernet/IP plus a
credential-driven boot-time connection flow. Network support is **disabled by
default** and must be explicitly enabled at build time due to the added
complexity and potential security implications.

The `@net:name`, `@net:pass` and `@net:id` YAFS nodes can always be written/read
(they live in `src/yafs.c`), but the automatic connection screen is compiled in
only with `NETWORK_ENABLED`.

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
| Network Device | `src/drivers/smc91c111.c` | SMC91C111 Ethernet (Versatile/PB onboard NIC) |
| Network Device | `src/drivers/ne2000.c` | NE2000-compatible Ethernet placeholder (other boards) |
| Network Core | `src/net/net.c` | Interface management, routing/polling, boot connect, `@net:id` generation |
| ARP | `src/net/arp.c` | Address Resolution Protocol |
| IP | `src/net/ip.c` | Internet Protocol (gateway-aware next-hop) |
| ICMP | `src/net/icmp.c` | Internet Control Message Protocol (ping) |
| UDP | `src/net/udp.c` | User Datagram Protocol (port handlers) |
| TFTP | `src/net/tftp.c` | Trivial File Transfer Protocol read client |

## Supported Hardware

| Driver | Hardware | QEMU Emulation |
|--------|----------|----------------|
| SMC91C111 | Onboard Ethernet at MMIO `0x10010000` (default `versatilepb` NIC) | `-nic user,model=smc91c111,tftp=/path/to/dir` |
| NE2000 | NE2000-compatible Ethernet | Placeholder only; other boards keep this path |

`NET_SMC91C111` is defined only when building `versatilepb` with networking
enabled. Other boards still call `ne2000_init_driver()`, whose register I/O is
a placeholder and does not move packets yet.

## Example: QEMU with Networking

```bash
# Build with networking
make NETWORK_ENABLED=1

# Run QEMU with the onboard SMC91C111 and user networking.
# The tftp= directory is exported by QEMU's built-in TFTP server.
mkdir -p /tmp/roxip-tftp && echo "hello from tftp" > /tmp/roxip-tftp/hello.txt

qemu-system-arm -M versatilepb -cpu arm926 -m 256M \
  -kernel roxip.bin \
  -drive file=roxip.storage,if=pflash,format=raw \
  -vga none -serial stdio -audio driver=none -display none \
  -nic user,model=smc91c111,tftp=/tmp/roxip-tftp
```

Use `-nic`, not a bare `-netdev user,id=net0`: the latter has no frontend on
`versatilepb` and the SMC91C111 is never instantiated (QEMU prints
`netdev net0 has no peer`). QEMU user-mode networking uses:

| Setting | Value |
|---------|-------|
| Guest IP | `10.0.2.15` |
| Netmask | `255.255.255.0` |
| Gateway / router | `10.0.2.2` |
| DNS (unused by TFTP) | `10.0.2.3` |

## Shell Commands

Networking commands are available in the `perf` shell when built with
`NETWORK_ENABLED`:

```
net                          Show interface/credential status
net up                       Bring eth0 up
ping <ip>                    Send one ICMP echo and wait up to 3 s for a reply
tftp <server-ip> <file> [local]
                             Download a file via TFTP and save it to YAFS
```

Example session:

```
perf > net up
perf > ping 10.0.2.2
perf > tftp 10.0.2.2 hello.txt downloaded.txt
perf > cat downloaded.txt
```

TFTP reads use RFC 1350 octet mode with 512-byte blocks. Downloads are buffered
in RAM and written to YAFS once complete, with a **32 KiB** cap and a 15-second
timeout. The local filename defaults to the remote file's basename. Writing
`@net:*` nodes via TFTP is rejected. Shell output is rendered to the framebuffer
terminal; serial output carries the boot log and high-level `[NET] ...`
diagnostics for headless verification.

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

`echo @net:name` with no value prints the current node. The classic redirect
form (`echo text > file`) still works for ordinary YAFS files. SSIDs are
limited to 32 bytes and passwords to 63 bytes.

### Boot-Time Connection

At boot stage `[11b]` (just before the interactive bootloader), RoxiP checks
for `@net:name` and `@net:pass` in YAFS storage. If both are present, it:

1. Displays a "Connecting to network..." screen with the SSID, phase
   (`Scanning`, `Authenticating`, `Associating`, `Bringing link up`), spinner
   and a progress bar
2. Brings the network interface up as the software link layer completes
3. On success, generates and stores `@net:id` (an obfuscated network identifier)
4. Shows a short "Connected" screen, then proceeds to the interactive bootloader

During the connection process the main OS is not accessible. If credentials are
missing/invalid, RoxiP logs the reason and boots offline instead of hanging.
Serial output reports every step with `[NET] ...` messages, so the flow can be
verified headlessly in QEMU.

> **Hardware note:** `versatilepb` uses a real SMC91C111 Ethernet driver
> (`src/drivers/smc91c111.c`). Other boards still have the NE2000 placeholder,
> which does not transfer packets. There is no 802.11 driver yet, so the
> association phase is handled by RoxiP's software link layer. The credential
> flow, connecting UI, interface bring-up and persistent `@net:id` are real; a
> future Wi-Fi driver only needs to replace the link step in
> `network_boot_connect()`.

### `@net` Synthetic Nodes

| Node | Description | Writable |
|------|-------------|----------|
| `@net:name` | Wi-Fi network name (SSID), max 32 bytes | Yes |
| `@net:pass` | Wi-Fi password, max 63 bytes | Yes |
| `@net:id` | Network identifier, written after a successful connection | No |

`@net:id` is derived with FNV-1a from the SSID and the interface MAC address,
for example `ROXIP-NET-F85D85DA`. It stays empty until RoxiP connects, and both
`roxip-cli write @net:id ...` and `echo @net:id "..."` are rejected. Reading
`@net:id` (typing `@net:id` at the shell, or `roxip-cli read ... @net:id`)
returns the stored value.

### Example Boot Sequence with Networking

```
[BOOT DBG] [08/12] Mounting File Systems...
[BOOT DBG] [YAFS] Persistent YAFS mounted from roxip.storage
[BOOT DBG] [08b] Initializing FAT32 Read-Only Driver...
[BOOT DBG] [08c] Initializing Network Stack...
[BOOT DBG] [11b] Initializing Network Connection...
[BOOT DBG] [NET] Credentials found - attempting connection
[BOOT DBG] [NET] Link established
[BOOT DBG] [NET] Connected to network
[BOOT DBG] [NET] Connected - ID: ROXIP-NET-F85D85DA
[BOOT DBG] [12/12] Launching Interactive Bootloader...
```

If no credentials are configured:

```
[BOOT DBG] [NET] No credentials configured - skipping connection
[BOOT DBG]      Network not connected - booting offline.
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
- The SMC91C111 driver is experimental and other boards still use the NE2000 placeholder

## API Reference

| Function | Description |
|----------|-------------|
| `net_init()` | Initialize network stack |
| `net_if_register()` | Register a network interface |
| `net_if_up()` / `net_if_down()` | Bring interface up/down |
| `net_rx()` / `net_tx()` | Receive/transmit packets |
| `network_boot_connect()` | Run the boot-time connection sequence |
| `network_print_status()` | Shell `net` command (SSID, hidden password, ID, link state) |
| `net_store_id()` | Persist the generated `@net:id` (kernel-internal) |
| `net_next_hop()` / `net_poll()` / `net_arp_wait()` | Routing, RX polling, blocking ARP |
| `arp_resolve()` / `arp_request()` | ARP address resolution |
| `ip_output()` | Send IP packet (gateway-aware) |
| `icmp_echo()` / `icmp_ping()` | Send ICMP echo / wait for echo reply |
| `udp_output()` / `udp_bind()` / `udp_unbind()` | Send UDP datagram / register port handlers |
| `net_ping()` | Shell `ping` helper |
| `tftp_get()` | Shell TFTP download helper |

## `@net` Node API

| Node | Type | Description |
|------|------|-------------|
| `@net:name` | Read/Write | Wi-Fi SSID (max 32 bytes) |
| `@net:pass` | Read/Write | Wi-Fi password (max 63 bytes) |
| `@net:id` | Read-only | Network identifier (auto-generated after connect) |

## Testing the Flow

```bash
make NETWORK_ENABLED=1
make cli

./roxip-cli create roxip.storage 64
./roxip-cli write roxip.storage @net:name "Example"
./roxip-cli write roxip.storage @net:pass "123456789"

mkdir -p /tmp/roxip-tftp && printf 'HELLO FROM ROXIP TFTP\n' > /tmp/roxip-tftp/hello.txt

qemu-system-arm -M versatilepb -cpu arm926 -m 256M \
  -kernel roxip.bin \
  -drive file=roxip.storage,if=pflash,format=raw \
  -vga none -serial stdio -audio driver=none -display none \
  -nic user,model=smc91c111,tftp=/tmp/roxip-tftp
```

At the shell prompt, run `net up`, `ping 10.0.2.2`, then
`tftp 10.0.2.2 hello.txt downloaded.txt`. After boot, the generated ID and the
downloaded file can be inspected from the host:

```bash
./roxip-cli read roxip.storage @net:id
./roxip-cli read roxip.storage downloaded.txt
```

The same flow works with Meson (`-Dnetwork-enabled=true -Dbuild-cli=true`) and
on `riscv-virt` (`make BOARD=riscv-virt NETWORK_ENABLED=1`, with
`qemu-system-riscv64 -M virt` and the `virtio-blk-device` storage setup from
`docs/README.md`).
