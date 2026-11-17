# RoxiP Drivers

RoxiP has four driver categories:

| Driver | File | Boards |
|--------|------|--------|
| UART | `src/drivers/uart.c` | All (PL011 on ARM, 16550 on RISC-V) |
| Display | `src/drivers/display.c` | VersatilePB (PL110), stub elsewhere |
| Input | `src/drivers/input.c` | VersatilePB (KMI PS/2), stub elsewhere |
| Storage & Audio | `src/drivers/storage_audio.c` | All (board-aware) |

## UART Driver (`src/drivers/uart.c`)

Provides `uart_putc()`, `uart_puts()`, and `uart_getc_nonblocking()`.

Two implementations are selected via `#ifdef UART_THR`:

**ARM boards (PL011 at `UART0_BASE`):**
- Uses PL011 registers: `UART0_DR`, `UART0_FR`
- `UART0_FR` bit 5 = TX FIFO full, bit 4 = RX FIFO empty

**RISC-V virt (16550-compatible at `0x10000000`):**
- Uses 16550 registers: `UART_THR`, `UART_RBR`, `UART_LSR`
- `UART_LSR` bit 0 = Data Ready, bit 5 = THRE (Transmit Holding Register Empty)

Board-specific UART init for RISC-V is in `firmware/riscv-virt/board.c` (`board_uart_init()`).

## Display Driver (`src/drivers/display.c`)

Provides `init_pl110_lcd()`.

**VersatilePB:** Programs the ARM PrimeCell PL110 Color LCD controller:
- `SYS_CLCD` enable
- `PL110_LCDTIM0`–`TIM3` — horizontal/vertical timing
- `PL110_LCDUPBASE` — framebuffer base address (`fb_pixels`)
- `PL110_LCDCONTROL` — enable LCD

**Other boards:** The PL110 registers are guarded with `#ifdef PL110_LCDCONTROL`. On boards without PL110 (Rockchip, Amlogic, RISC-V), `init_pl110_lcd()` logs a stub message and returns.

## Input Driver (`src/drivers/input.c`)

Provides `init_kmi_mouse()`, `poll_kmi_mouse()`, `init_kmi_keyboard()`, and `get_keyboard_input()`.

**VersatilePB (KMI PS/2):**
- `KMI0` = keyboard, `KMI1` = mouse at VersatilePB MMIO addresses
- PS/2 Set 2 scan codes translated to ASCII via lookup tables
- Extended keys (arrows, F1–F4) supported
- Mouse clicks on desktop tabs switch tabs/themes

**Other boards:** KMI functions are stubs (`#ifdef KMI0_BASE`). `get_keyboard_input()` falls back to UART only.

## Storage & Audio Driver (`src/drivers/storage_audio.c`)

Provides `detect_primary_disk()` and `init_hda_audio()`.

Board-aware via `#ifdef MMC_BASE` / `#ifdef AUDIO_BASE`:

| Board | Storage | Audio |
|-------|---------|-------|
| VersatilePB | VirtIO / MMC stub | Generic MMIO Audio stub |
| Rockchip | dwmc eMMC/SD (`0xFF2C0000`) | SoC Audio Codec (`0xFF1A0000`) |
| Amlogic Meson | dwmc eMMC/SD (`0xFFD40000`) | SoC Audio Codec (`0xFF804000`) |
| RISC-V virt | — | — |

Storage probes the MMC `STATUS` register; audio probes the `CTRL` register. If the register reads `0xFFFFFFFF` or `0x00000000`, the device is reported as "not responding".
