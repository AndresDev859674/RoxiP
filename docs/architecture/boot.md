# RoxiP Architecture

## Boot Process (`kmain()` in `src/kernel.c`)

The kernel boots in 12 stages:

| Stage | Action | File |
|-------|--------|------|
| 01 | `board_init()` — exception vectors/traps, board log | `firmware/<board>/board.c` |
| 02 | `init_heap()` — dynamic memory arena after `__bss_end` | `src/memory.c` |
| 03 | `init_env_vars()` — environment variable table | `src/shell.c` |
| 04 | `detect_cpu_brand()` — board-specific CPU string | `firmware/<board>/board.c` |
| 05 | `init_pl110_lcd()` — PL110 framebuffer (ARM VersatilePB only) | `src/drivers/display.c` |
| 06 | `detect_gpu()` — board-specific GPU string | `firmware/<board>/board.c` |
| 07 | `init_hda_audio()` — audio codec detection | `src/drivers/storage_audio.c` |
| 08 | `detect_primary_disk()` — storage detection | `src/drivers/storage_audio.c` |
| 09 | `init_filesystems()` — mount YAFS from `roxip.storage` | `src/yafs.c` |
| 10 | `init_comcow()` — display compositor engine | `src/graphics.c` |
| 11 | `init_kmi_keyboard/mouse()` — PS/2 input (ARM VersatilePB only) | `src/drivers/input.c` |
| 12 | `run_bootloader()` — 10-second interactive boot menu | `src/kernel.c` |

After the bootloader, the main loop polls input, redraws the current view, and processes shell commands.

If any step fails catastrophically, `kernel_panic()` is called — it displays a red screen (ARM) or UART message (RISC-V) and halts.

## Memory Map

| Address | Region |
|---------|--------|
| `0x00000000` | ARM exception vectors (triple-fault guard) |
| `0x10000` | Kernel load address (ARM, via `linker.ld`) |
| `0x80000000` | Kernel load address (RISC-V, via `linker_riscv.ld`) |
| `__bss_end` → `__bss_end + HEAP_SIZE` | Dynamic heap arena |
| `0x34000000` | Storage flash base (`STORAGE_FLASH_BASE`) |

## HAL Contract

Each board in `firmware/<board>/` implements the interface declared in `include/hal.h`:

```c
void board_init(void);
void board_detect_cpu(char* out_buf);
void board_detect_gpu(char* out_buf);
void board_delay_ms(unsigned int ms);
void board_delay_cycles(volatile unsigned int count);
void system_shutdown(void);
void system_reboot(void);
void init_idt(void);          // exception vector table / trap setup
void kernel_panic(const char* msg);
```

Drivers include `firmware.h` which resolves to the active board's header via the
`-Ifirmware/$(BOARD)` include path. Board-specific MMIO (UART, timer, MMC, audio, HDMI) lives in
`firmware/<board>/firmware.h` — not in `kernel.h` — so the same driver sources compile for every board.

Optional hardware (PL110 LCD, KMI keyboard/mouse) is guarded with `#ifdef` and compiles to stubs on boards without them.

## System Modes

Defined in `include/kernel.h` as `system_mode_t`:

| Mode | Description |
|------|-------------|
| `MODE_PERF_CLI` | Performance terminal / CLI shell |
| `MODE_DESK` | Desktop with tabs and themes |
| `MODE_DESK_WINDOW` | Performance window overlay |
| `MODE_FLOATCUBE` | 3D wireframe cube demo |
| `MODE_MICRO_EDITOR` | Micro text editor |
| `MODE_VFM` | Visual file manager |
| `MODE_ZTOP` | System monitor (like top) |
| `MODE_RGBDEMO` | RGB gradient demo |

## Themes

Defined as `desk_theme_t`:

| Theme | Description |
|-------|-------------|
| `THEME_DEFAULT` | Industrial Grey |
| `THEME_BLUE` | Classic Blue |
| `THEME_MAC_MONO` | Monochrome (classic Mac OS style) |
