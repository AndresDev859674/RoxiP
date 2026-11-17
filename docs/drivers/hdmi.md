# HDMI / DVI Display Driver (`src/drivers/hdmi.c`)

## Overview

HDMI TX support for Amlogic Meson SoCs. Phase 3.5 starts with **DVI mode** — simpler than full HDMI because it requires no InfoFrames, no audio, and no EDID handling. We just configure video timing, power up the PHY, and enable output.

## Supported Boards

| Board | HDMI Support |
|-------|-------------|
| Amlogic Meson (S905X2/X3, S905Y2, A311D, etc.) | Full DVI driver at `0xFF200000` |
| VersatilePB | Stub (no HDMI hardware) |
| Rockchip | Stub (no HDMI hardware) |
| RISC-V virt | Stub (no HDMI hardware) |

## Register Map (`firmware/amlogic/firmware.h`)

| Register | Address | Description |
|----------|---------|-------------|
| `HDMI_TX_CTRL` | `0xFF200000` | HDMI TX control |
| `HDMI_TX_STATUS` | `0xFF200004` | HDMI TX status |
| `HDMI_TX_H_START` | `0xFF200008` | Horizontal active start |
| `HDMI_TX_H_END` | `0xFF20000C` | Horizontal active end |
| `HDMI_TX_V_START` | `0xFF200010` | Vertical active start |
| `HDMI_TX_V_END` | `0xFF200014` | Vertical active end |
| `HDMI_TX_HSYNC_W` | `0xFF200018` | HSYNC width |
| `HDMI_TX_VSYNC_W` | `0xFF20001C` | VSYNC width |
| `HDMI_TX_MODE` | `0xFF200020` | `HDMI_MODE_DVI` or `HDMI_MODE_HDMI` |
| `HDMI_TX_ENABLE` | `0xFF200024` | Enable output |
| `HDMI_PHY_POWER` | `0xFF201008` | PHY power control |

## Boot Integration

`init_hdmi_dvi()` is called from `kmain()` at stage "[05b] Initializing HDMI/DVI Display...", right after PL110 initialization.

On boards without HDMI TX hardware, `init_hdmi_dvi()` is a stub that logs "[OK] Display initialized (no HDMI TX on this board)".

## Video Timing

Default mode: **640×480 @ 60Hz** (VESA DMT):

| Parameter | Value |
|-----------|-------|
| H active | 640 |
| H front porch | 16 |
| H sync | 96 |
| H back porch | 48 |
| V active | 480 |
| V front porch | 10 |
| V sync | 2 |
| V back porch | 33 |

## Future Work

* Full HDMI mode (InfoFrames, audio, EDID)
* 720p / 1080p timings
* VGA output (separate DAC)
