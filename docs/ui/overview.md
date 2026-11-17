# RoxiP UI System

The UI is built on top of an 80×25 VGA text buffer rendered into a framebuffer (640×480 on VersatilePB via PL110).

## Core (`src/ui/ui_core.c`)

- `term_lines[TERM_ROWS][TERM_COLS]` — terminal scrollback buffer (18 rows × 74 cols)
- `term_add_line()` — add a line to the terminal (with optional pipe capture)
- `draw_top_header()` — draws the status bar with "RoxiP Core (comcow)", Hub spinner, and animation
- `redraw_current_view()` — dispatches to the active mode's render function

## View Modes (`src/ui/ui_desktop.c`, `src/ui/ui_cli.c`, `src/ui/ui_apps.c`, `src/ui/ui_demos.c`)

| File | Modes | Description |
|------|-------|-------------|
| `ui_desktop.c` | `MODE_DESK`, `MODE_DESK_WINDOW` | Desktop with 3 tabs (System, Settings, Exit), themes, controls box |
| `ui_cli.c` | `MODE_PERF_CLI` | Performance terminal with command prompt |
| `ui_apps.c` | `MODE_ZTOP`, `MODE_VFM`, `MODE_MICRO_EDITOR` | System monitor, file manager, text editor |
| `ui_demos.c` | `MODE_FLOATCUBE`, `MODE_RGBDEMO` | 3D wireframe cube, RGB gradient demo |

## Graphics Primitives (`src/graphics.c`)

- `vga_draw_char_at()` / `vga_draw_string()` — draw to the VGA text buffer
- `vga_flush()` — copies changed cells from backbuffer to the framebuffer (renders 8×16 font glyphs to RGB565 pixels)
- `vga_clear()` / `vga_force_redraw()` — buffer management
- `draw_box()` — bordered box with title
- `draw_mouse_cursor()` — draws an 'X' at the mouse position
- `rgb_fill_rect()`, `rgb_draw_line_gradient()`, `rgb_draw_triangle_gradient()` — RGB framebuffer primitives
- `font8x8_basic[]` — 8×8 pixel font for 256 characters
- `vga_to_rgb565[]` — 16 VGA colors → RGB565 conversion table

## Themes

Three themes defined in `include/kernel.h`:

| Theme | Description |
|-------|-------------|
| `THEME_DEFAULT` | Industrial Grey (black text on dark grey) |
| `THEME_BLUE` | Classic Blue (black text on blue) |
| `THEME_MONOCHROME` | Monochrome (classic Mac OS dither pattern) |

Theme switching is done via the desktop Settings tab or keyboard input.

## The "comcow" Compositor

The `comcow_compositor_t` struct tracks display mode info (name, width, height, refresh rate, double buffering, latency, active mode ID). Despite the name, it is currently a text-mode compositor — all rendering goes through the VGA text buffer → framebuffer pipeline.
