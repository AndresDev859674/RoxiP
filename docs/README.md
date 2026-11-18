# Getting Started with RoxiP

Welcome to RoxiP — a tiny, bare-metal operating system for ARM and RISC-V architectures.

## What is RoxiP?

RoxiP is a minimalist operating system that boots directly on hardware (or in an emulator) without a BIOS, bootloader services, or traditional OS services. It was originally created for low-cost ARM-based hardware (like the Gamestick M15) and has evolved into a universal OS for ARM and RISC-V.

Key characteristics:

* **Bare-metal** — Runs directly on hardware with no underlying OS.
* **Single-file storage** — All data lives in `roxip.storage`, managed by YAFS. No partitions.
* **Portable** — The same sources compile for VersatilePB, Rockchip, Amlogic Meson, and RISC-V virt.

## What You'll Need

| Tool | Purpose |
|------|---------|
| `meson` >= 0.60 | Build system |
| `ninja` | Build backend (used by Meson) |
| `arm-none-eabi-gcc` | ARM cross-compiler |
| `riscv64-elf-gcc` | RISC-V cross-compiler |
| `qemu-system-arm` | ARM VM for testing |
| `qemu-system-riscv64` | RISC-V VM for testing |

On Arch Linux:

```bash
sudo pacman -S meson ninja arm-none-eabi-gcc riscv64-elf-gcc riscv64-elf-binutils \
  qemu-system-arm qemu-system-riscv
```

## Your First Build

### Step 1: Choose a build system

RoxiP supports both **Meson** and **Make**. Use whichever you prefer:

* **Meson:** `meson setup builddir --cross-file cross-arm-none-eabi.txt -Dboard=versatilepb`
* **Make:** `make` (defaults to VersatilePB)

### Step 2: Configure and build

```bash
# Using Meson (recommended)
meson setup builddir --cross-file cross-arm-none-eabi.txt -Dboard=versatilepb
meson compile -C builddir

# Using Make
make
```

This produces `builddir/roxip.bin` (or `roxip.bin` with Make), the raw binary kernel.

### Step 3: Run in QEMU

```bash
# Meson
meson compile -C builddir roxip-run

# Make
make run
```

QEMU will launch and you'll see the boot log on your serial console:

```
[BOOT DBG] [01/12] Initializing Board HAL & Exception Vectors...
[BOOT DBG] [05b] Initializing HDMI/DVI Display...
[BOOT DBG] [12/12] Launching Interactive Bootloader...
```

Press **Ctrl+C** to stop QEMU.

## Available Boards

| Board | CPU | Build flag (Meson) | Build flag (Make) | QEMU |
|-------|-----|-------------------|-------------------|------|
| VersatilePB | arm926ej-s | `-Dboard=versatilepb` | (default) | `qemu-system-arm -M versatilepb` |
| Rockchip | cortex-a7 | `-Dboard=rockchip` | `make rockchip` | software model |
| Amlogic Meson | cortex-a53 | `-Dboard=amlogic` | `make amlogic` | hardware only |
| RISC-V virt | rv64gc | `-Dboard=riscv-virt` | `make riscv-virt` | `qemu-system-riscv64 -M virt` |

## Next Steps

* Read `docs/philosophy.md` to understand the design principles.
* Read `docs/architecture/boot.md` for the 12-stage boot process.
* Read `docs/drivers/overview.md` for driver details.
* Read `docs/yafs/overview.md` for the single-file storage system.
* Try adding your own board — see `docs/architecture/boot.md` ("Adding a New Board").

## License

RoxiP is released under the GPL. See `LICENSE` for details.
