# RoxiP Philosophy

## Universal, Simple, Minimalist

RoxiP is built on three pillars:

### Universal

"Universal" means the code is architecturally prepared for **any scenario** — not just the hardware it runs on today. If a developer forks RoxiP to run it on a ESP32, a RISC-V microcontroller, or an entirely new SoC, the architecture should make that process straightforward and hassle-free.

This universality is achieved through:

* **Board abstraction via `firmware/<board>/`** — All SoC-specific MMIO addresses, exception vectors, reset/shutdown logic, and hardware detection live in per-board firmware directories. The kernel and drivers never hardcode a specific board's addresses; they include `firmware.h` which resolves to the active board via `-Ifirmware/$(BOARD)`.
* **HAL contract** — Every board implements the same interface declared in `include/hal.h` (`board_init()`, `init_idt()`, `system_shutdown()`, `system_reboot()`, etc.). New boards simply implement this contract.
* **Optional hardware guards** — Drivers use `#ifdef` to conditionally compile board-specific features (PL110 LCD, KMI keyboard/mouse, HDMI TX). Boards without that hardware get clean stubs instead of `#ifdef` spaghetti scattered through the kernel.
* **Single-toolchain builds** — Meson cross-files and Makefile toolchain variables make it trivial to swap in a different compiler (e.g., `ps2-gcc` for a PS2 port).

### Simple

RoxiP avoids unnecessary complexity:

* **No partitions** — All data lives in a single storage file (`roxip.storage`), managed by YAFS (Yet Another File System). There is no partition table, no MBR, no filesystem-on-disk complexity.
* **No dynamic discovery** — Hardware is configured statically at build time via board selection. No ACPI, no device tree, no PCI enumeration at runtime.
* **Minimal dependencies** — The kernel is freestanding (`-ffreestanding -nostdlib`). The only external toolchain requirement is a C cross-compiler and assembler.

### Minimalist

RoxiP is intentionally small:

* The kernel links at `0x10000` (ARM) or `0x80000000` (RISC-V) and fits in under 100 KB.
* The display is an 80×25 VGA text buffer rendered into a framebuffer — no GPU drivers, no windowing system, no compositing beyond text.
* The codebase targets under 15,000 lines of code. (if possible)

## Portability

Portability is a fundamental principle of RoxiP, exemplified by `roxip.storage` and its corresponding `.bin` file:

* A single file contains the entire file system. It can simply be copied onto any storage medium (SD card, USB flash drive, disk image) for RoxiP to boot from it.
* YAFS supports files of up to 2 MB each, with a limit of 16 files—sufficient capacity for a simple operating system and its utilities.
* The storage image is created once and can be reused across different boot sessions, emulators, and actual hardware.
* Since RoxiP uses a single `.bin` and `.elf` file, a bootloader is required to boot RoxiP on UEFI-based systems.

---

## Design Goals

| Goal | How it's achieved |
|------|-------------------|
| Easy to port to new SoCs | `firmware/<board>/` + HAL contract |
| Runs on crappy hardware | Minimal RAM, no MMU required, simple drivers |
| No partition tables | Single `roxip.storage` file + YAFS |
| Cross-platform build | Meson + Makefile, both maintained |
| Understandable codebase | <15K LOC, well-separated layers |
