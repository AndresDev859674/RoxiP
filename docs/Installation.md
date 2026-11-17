# Installation And a Quick Overview

To get started with RoxiP, there are two ways to compile it: using either Meson (with Ninja) or make.

## Quick Start

```bash
# 1. Install dependencies (Arch Linux example)
sudo pacman -S meson ninja arm-none-eabi-gcc riscv64-elf-gcc riscv64-elf-binutils \
  qemu-system-arm qemu-system-riscv

# 2. Configure for your board (ARM VersatilePB = easiest to test)
meson setup builddir --cross-file cross-arm-none-eabi.txt -Dboard=versatilepb

# 3. Build
meson compile -C builddir

# 4. Run in QEMU (shows serial boot logs)
meson compile -C builddir roxip-run   # press Ctrl+C to stop
```

Or with the legacy Makefile:

```bash
make                # builds for VersatilePB by default
make rockchip       # builds for Rockchip
make amlogic        # builds for Amlogic Meson
make riscv-virt     # builds for RISC-V virt
make run            # runs in QEMU
```

## Project Overview

| Aspect | Description |
|--------|-------------|
| **Storage** | Single file (`roxip.storage`) — no partitions. Managed by YAFS |
| **Display** | 80×25 VGA text buffer rendered into a framebuffer |
| **Architecture** | ARM (arm926, cortex-a7, cortex-a53) and RISC-V (rv64gc) |
| **Build systems** | Meson (primary) and Makefile (legacy, both maintained) |

## Documentation

| Document | Contents |
|----------|----------|
| `architecture/` | Kernel boot process, HAL contract, memory map |
| `drivers/` | UART, display, input, storage/audio drivers |
| `ui/` | Terminal, CLI, desktop, apps, demos |
| `cli/` | Shell commands and host CLI tool |
| `yafs/` | Single-file file system |
| `Installation.md` | This file — build, run, and add a new board |

## Supported Boards

| Board | CPU | Toolchain | QEMU Machine |
|-------|-----|-----------|--------------|
| `versatilepb` | arm926ej-s | arm-none-eabi-* | `versatilepb` |
| `rockchip` | cortex-a7 | arm-none-eabi-* | `versatilepb` (software model) |
| `amlogic` | cortex-a53 | arm-none-eabi-* | hardware only |
| `riscv-virt` | rv64gc | riscv64-elf-* | `virt` (needs OpenSBI) |

## License

RoxiP is released under the GPL. See `LICENSE` for details.
