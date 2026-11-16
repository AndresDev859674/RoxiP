# RoxiP — Installation & How It Works

RoxiP is a tiny bare-metal operating system for ARM and RISC-V architectures. It was originally created for
crappy ARM-based hardware (like the Gamestick M15) but has evolved into a universal OS for ARM and RISC-V.

Key design choices:

* **No partitions.** All data lives in a single storage file (`roxip.storage`), which is copied to
  `roxip.img` for QEMU. A lightweight custom file system called **YAFS** (Yet Another File System) manages
  files inside that single file.
* **Bare-metal.** No BIOS, no bootloader services beyond loading the `.bin` into RAM at `0x10000`.
* **Portable.** The same sources compile for multiple boards (VersatilePB, Rockchip, Amlogic Meson) by
  selecting the board at build time.

---

## 1. Requirements

| Tool | Purpose |
|------|---------|
| `meson` >= 0.60 | Build system (replaces the legacy `Makefile`) |
| `ninja` | Backend used by Meson |
| `arm-none-eabi-gcc` / `arm-none-eabi-as` / `arm-none-eabi-objcopy` | ARM bare-metal cross-toolchain |
| `qemu-system-arm` | Emulate VersatilePB for testing |

On Arch Linux:

```bash
sudo pacman -S meson ninja arm-none-eabi-gcc qemu-system-arm
```

---

## 2. Build System (Meson)

The legacy `Makefile` has been replaced by **Meson + Ninja**.

### 2.1 Cross-compilation file

Create or use `cross-arm-none-eabi.txt` (already in the repo root). It points Meson at the ARM bare-metal
toolchain.

### 2.2 Configure

```bash
meson setup builddir \
  --cross-file cross-arm-none-eabi.txt \
  -Dboard=versatilepb        # or: rockchip, amlogic
```

Available boards:

| `-Dboard=` | CPU | Description |
|------------|-----|-------------|
| `versatilepb` | arm926ej-s | QEMU VersatilePB (default, easiest to test) |
| `rockchip` | cortex-a7 | Rockchip RK3288/RK3328-class SoCs |
| `amlogic` | cortex-a53 | Amlogic Meson G12A/G12B (S905X2/X3, S905Y2, A311D…) |

### 2.3 Build

```bash
meson compile -C builddir
```

This produces:

* `builddir/roxip.elf` — ELF kernel image
* `builddir/roxip.bin` — raw binary (boots from `0x10000`)

### 2.4 Build options

```bash
meson setup builddir --cross-file cross-arm-none-eabi.txt \
  -Dboard=amlogic \
  -Droxip-debug=true        # -O0 -g -DDEBUG
  -Dbuild-cli=true          # build host CLI tool (roxip-cli)
  -Dgdb-port=7777           # GDB stub port
```

---

## 3. Running in QEMU

### `meson compile -C builddir roxip-run`

This is the Meson equivalent of `make run`. It builds `roxip.bin`, creates a 64MB `roxip.storage`
image, and launches QEMU with serial output so you see the boot logs on your terminal:

```
[BOOT DBG] [01/12] Initializing Board HAL & Exception Vectors...
[BOOT DBG]   [VEC 1/2] Setting up ARM Vector Table...
...
[BOOT DBG] [12/12] Launching Interactive Bootloader...
```

Press **Ctrl+C** to stop QEMU.

### Debug mode

If you want GDB instead of direct serial output, run QEMU manually:

```bash
qemu-system-arm -M versatilepb -cpu arm926 -m 256M \
  -kernel builddir/roxip.bin \
  -drive file=builddir/roxip.storage,if=pflash,format=raw \
  -vga none -serial stdio -audio driver=none \
  -gdb tcp::7777 -S
```

Then connect:

```bash
arm-none-eabi-gdb roxip.elf -ex 'target remote localhost:7777'
```

---

## 4. Architecture Overview

```
roxip/
├── startup.s              # ARM entry: init stack, clear BSS, jump to kmain()
├── linker.ld              # Linker script — kernel linked at 0x10000
├── Makefile               # Legacy makefile (kept for reference)
├── meson.build            # Meson build definition
├── meson_options.txt      # Meson options
├── cross-arm-none-eabi.txt # Cross-compilation file
├── firmware/              # Board-specific SoC support (MMIO, init, panic, vectors)
│   ├── versatilepb/       # ARM VersatilePB (QEMU)
│   ├── rockchip/          # Rockchip RK3288/RK3328
│   └── amlogic/           # Amlogic Meson G12A/G12B
├── src/
│   ├── kernel.c           # kmain() — boot sequence, main loop, mode switching
│   ├── memory.c           # Heap allocator, libc-like string/mem functions
│   ├── graphics.c         # Framebuffer, VGA text rendering, fonts, RGB primitives
│   ├── shell.c            # Command shell, env vars, disk info
│   ├── yafs.c             # YAFS file system (single storage file)
│   ├── drivers/
│   │   ├── uart.c         # PL011 / AO UART serial
│   │   ├── display.c      # PL110 LCD (VersatilePB only)
│   │   ├── input.c        # KMI PS/2 keyboard+mouse (VersatilePB only)
│   │   └── storage_audio.c# Disk + audio detection (board-aware)
│   ├── ui/
│   │   ├── ui_core.c      # Terminal, view redraw, mode dispatch
│   │   ├── ui_cli.c       # CLI rendering
│   │   ├── ui_desktop.c   # Desktop with tabs/themes
│   │   ├── ui_apps.c      # ztop, vfm, micro editor
│   │   └── ui_demos.c     # floatcube, rgbdemo
│   └── cli/
│       └── roxip-cli.c    # Host tool to create storage images
├── include/
│   ├── kernel.h           # Types, constants, shared globals, prototypes
│   ├── hal.h              # Hardware abstraction layer interface
│   ├── drivers.h          # Driver prototypes
│   └── ui.h               # UI prototypes
├── x86/                   # Legacy x86 MBR bootloader (unused by ARM build)
├── docs/                  # This documentation
└── assets/                # Images, icons
```

### 4.1 Boot process (`kmain()` in `src/kernel.c`)

```
01. board_init()           — exception vectors, triple-fault guard, board log
02. init_heap()            — dynamic memory arena after __bss_end
03. init_env_vars()        — environment variable table
04. detect_cpu_brand()     — board-specific CPU string
05. init_pl110_lcd()       — PL110 framebuffer (VersatilePB only)
06. detect_gpu()           — board-specific GPU string
07. init_hda_audio()       — audio codec detection (board-aware)
08. detect_primary_disk()  — storage detection (board-aware)
09. init_filesystems()     — mount YAFS from roxip.storage
10. init_comcow()          — display compositor engine
11. init_kmi_keyboard/mouse() — PS/2 input (VersatilePB only)
12. run_bootloader()       — 10-second interactive boot menu
13. Main loop              — poll input, redraw view, process shell commands
```

If any step fails catastrophically, `kernel_panic()` is called — it displays a red screen and halts.

### 4.2 Firmware / HAL contract

Each board in `firmware/<board>/` implements the same interface declared in `include/hal.h`:

```c
void board_init(void);
void board_detect_cpu(char* out_buf);
void board_detect_gpu(char* out_buf);
void board_delay_ms(unsigned int ms);
void board_delay_cycles(volatile unsigned int count);
void system_shutdown(void);
void system_reboot(void);
void init_idt(void);          // ARM exception vector table
void kernel_panic(const char* msg);
```

Drivers (`src/drivers/*.c`) include `firmware.h` which resolves to the active board's header via the
`-Ifirmware/$(BOARD)` include path. Board-specific MMIO (UART, timer, MMC, audio, HDMI) lives there —
not in `kernel.h` — so the same driver sources compile for every board. Optional hardware (PL110 LCD,
KMI keyboard/mouse) is guarded with `#ifdef`.

---

## 5. Storage: YAFS

RoxiP does **not** use partitions. Instead:

1. A single file (`roxip.storage`, typically 64 MB) holds all file-system data.
2. For QEMU, this file is copied to `roxip.img`.
3. **YAFS** (Yet Another File System) is a minimal custom file system that stores files inside
   `roxip.storage`.

Create a storage image:

```bash
# Via the host CLI tool (if built)
./roxip-cli create roxip.storage 64

# Or manually
truncate -s 64M roxip.storage
```

YAFS supports:
* `fs_read_file`, `fs_write_file`, `fs_write_file_binary`
* `fs_mkdir`
* TAR archive extraction (`execute_tar`)

---

## 6. Display

* **VersatilePB:** PL110 Color LCD at 640×480. The OS renders an 80×25 VGA text buffer into the
  framebuffer (software-rendered characters).
* **Rockchip / Amlogic:** No PL110 — `init_pl110_lcd()` is a no-op stub. Display output is currently
  serial-only on these boards.
* **DVI/HDMI:** Planned (Phase 3). Amlogic HDMI TX MMIO addresses are defined in
  `firmware/amlogic/firmware.h`; the driver init sequence is not yet implemented.

---

## 7. Adding a New Board

1. Create `firmware/<board>/firmware.h` with that SoC's MMIO addresses (UART, timer, MMC, audio,
   display, reset).
2. Create `firmware/<board>/board.c` implementing the HAL interface from `include/hal.h`
   (`board_init`, `init_idt`, `kernel_panic`, `system_shutdown`, `system_reboot`, delays,
   CPU/GPU detection).
3. Create `firmware/<board>/system.c` (can be minimal — see `firmware/rockchip/system.c`).
4. Add a Meson option choice and CPU mapping in `meson.build` / `meson_options.txt`.
5. Build: `meson setup builddir --cross-file cross-arm-none-eabi.txt -Dboard=<board>`

---

## 8. Directory Summary

| Path | Contents |
|------|----------|
| `firmware/<board>/` | Board-specific SoC support (MMIO, init, panic, vectors) |
| `src/drivers/` | UART, display, input, storage/audio drivers |
| `src/ui/` | Terminal, CLI, desktop, apps, demos |
| `src/kernel.c` | `kmain()` — boot sequence and main loop |
| `src/memory.c` | Heap allocator + libc-like helpers |
| `src/yafs.c` | Single-file file system |
| `src/graphics.c` | Framebuffer, fonts, text/RGB rendering |
| `include/*.h` | Shared headers (types, constants, prototypes) |
| `startup.s` | ARM entry point |
| `linker.ld` | Linker script (kernel at 0x10000) |
| `x86/` | Legacy x86 MBR bootloader |
| `docs/` | This documentation |

---

## 9. License

RoxiP is released under the GPL. See `LICENSE` for details.
