#include "kernel.h"
#include "firmware.h"

unsigned short fb_pixels[FB_WIDTH * FB_HEIGHT] __attribute__((aligned(16)));
unsigned short vga_virtual_buffer[VGA_WIDTH * VGA_HEIGHT];
unsigned short vga_backbuffer[VGA_WIDTH * VGA_HEIGHT];

comcow_compositor_t comcow;

const unsigned char font8x8_basic[256][8] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['!'] = {0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00},
    ['#'] = {0x24, 0x24, 0xff, 0x24, 0x24, 0xff, 0x24, 0x24},
    ['%'] = {0x62, 0x64, 0x08, 0x10, 0x26, 0x46, 0x00, 0x00},
    ['~'] = {0x00, 0x32, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['<'] = {0x18, 0x30, 0x60, 0x30, 0x18, 0x00, 0x00, 0x00},
    ['?'] = {0x3c, 0x66, 0x0c, 0x18, 0x18, 0x00, 0x18, 0x00},
    ['@'] = {0x3c, 0x66, 0x6e, 0x6e, 0x60, 0x3e, 0x00, 0x00},
    ['>'] = {0x60, 0x30, 0x18, 0x0c, 0x18, 0x30, 0x60, 0x00},
    ['='] = {0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00},
    ['-'] = {0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00},
    ['+'] = {0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00},
    ['|'] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
    ['/'] = {0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x00, 0x00},
    ['\\']= {0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x00, 0x00},
    [':'] = {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00},
    ['*'] = {0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00},
    ['.'] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00},
    [','] = {0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 0x00},
    ['_'] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00},
    ['['] = {0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00},
    [']'] = {0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00},
    ['('] = {0x0e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x0e, 0x00},
    [')'] = {0x70, 0x18, 0x18, 0x18, 0x18, 0x18, 0x70, 0x00},
    ['p'] = {0x00, 0x00, 0xdc, 0x66, 0x66, 0x7c, 0x60, 0xf0},
    ['e'] = {0x00, 0x00, 0x3c, 0x66, 0x7e, 0x60, 0x3c, 0x00},
    ['r'] = {0x00, 0x00, 0xdd, 0x66, 0x60, 0x60, 0xf0, 0x00},
    ['f'] = {0x1c, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x78, 0x00},
    ['a'] = {0x00, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00},
    ['b'] = {0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0xdc, 0x00},
    ['c'] = {0x00, 0x00, 0x3c, 0x66, 0x60, 0x66, 0x3c, 0x00},
    ['d'] = {0x06, 0x06, 0x3e, 0x66, 0x66, 0x66, 0x3e, 0x00},
    ['g'] = {0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x3c},
    ['h'] = {0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0xe6, 0x00},
    ['i'] = {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00},
    ['j'] = {0x0c, 0x00, 0x1c, 0x0c, 0x0c, 0x0c, 0xcc, 0x78},
    ['k'] = {0x60, 0x60, 0x66, 0x6c, 0x78, 0x6c, 0x66, 0x00},
    ['l'] = {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00},
    ['m'] = {0x00, 0x00, 0xec, 0xfe, 0xd6, 0xd6, 0xc6, 0x00},
    ['n'] = {0x00, 0x00, 0xdc, 0x66, 0x66, 0x66, 0x66, 0x00},
    ['o'] = {0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00},
    ['q'] = {0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x0f},
    ['s'] = {0x00, 0x00, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x00},
    ['t'] = {0x18, 0x18, 0x7e, 0x18, 0x18, 0x18, 0x0e, 0x00},
    ['u'] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00},
    ['v'] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00},
    ['w'] = {0x00, 0x00, 0xc6, 0xd6, 0xd6, 0xfe, 0x6c, 0x00},
    ['x'] = {0x00, 0x00, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x00},
    ['y'] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x3c},
    ['z'] = {0x00, 0x00, 0x7e, 0x0c, 0x18, 0x30, 0x7e, 0x00},
    ['A'] = {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x00},
    ['B'] = {0x7c, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x7c, 0x00},
    ['C'] = {0x3c, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3c, 0x00},
    ['D'] = {0x78, 0x6c, 0x66, 0x66, 0x66, 0x6c, 0x78, 0x00},
    ['E'] = {0x7e, 0x60, 0x60, 0x7c, 0x60, 0x60, 0x7e, 0x00},
    ['F'] = {0x7e, 0x60, 0x60, 0x7c, 0x60, 0x60, 0x60, 0x00},
    ['G'] = {0x3c, 0x66, 0x60, 0x6e, 0x66, 0x66, 0x3e, 0x00},
    ['H'] = {0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00},
    ['I'] = {0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00},
    ['J'] = {0x1e, 0x0c, 0x0c, 0x0c, 0x0c, 0xcc, 0x78, 0x00},
    ['K'] = {0x66, 0x6c, 0x78, 0x70, 0x78, 0x6c, 0x66, 0x00},
    ['L'] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00},
    ['M'] = {0x63, 0x77, 0x7f, 0x6b, 0x63, 0x63, 0x63, 0x00},
    ['N'] = {0x66, 0x76, 0x7e, 0x7e, 0x6e, 0x66, 0x66, 0x00},
    ['O'] = {0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00},
    ['P'] = {0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x60, 0x00},
    ['Q'] = {0x3c, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x0e, 0x00},
    ['R'] = {0x7c, 0x66, 0x66, 0x7c, 0x6c, 0x66, 0x63, 0x00},
    ['S'] = {0x3c, 0x66, 0x60, 0x3c, 0x06, 0x66, 0x3c, 0x00},
    ['T'] = {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    ['U'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00},
    ['V'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00},
    ['W'] = {0x63, 0x63, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00},
    ['X'] = {0x66, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x66, 0x00},
    ['Y'] = {0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00},
    ['Z'] = {0x7e, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x7e, 0x00},
    ['0'] = {0x3c, 0x66, 0x6f, 0x77, 0x66, 0x66, 0x3c, 0x00},
    ['1'] = {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00},
    ['2'] = {0x3c, 0x66, 0x06, 0x1c, 0x30, 0x60, 0x7e, 0x00},
    ['3'] = {0x3c, 0x66, 0x06, 0x1c, 0x06, 0x66, 0x3c, 0x00},
    ['4'] = {0x0c, 0x1c, 0x3c, 0x6c, 0xfe, 0x0c, 0x0c, 0x00},
    ['5'] = {0x7e, 0x60, 0x7c, 0x06, 0x06, 0x66, 0x3c, 0x00},
    ['6'] = {0x3c, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x3c, 0x00},
    ['7'] = {0x7e, 0x06, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x00},
    ['8'] = {0x3c, 0x66, 0x66, 0x3c, 0x66, 0x66, 0x3c, 0x00},
    ['9'] = {0x3c, 0x66, 0x66, 0x3e, 0x06, 0x0c, 0x38, 0x00},
    [176] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55},
    [219] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};

const int sin_table[36] = {
    0, 22, 43, 64, 82, 98, 110, 120, 126, 128, 126, 120, 110, 98, 82, 64, 43, 22,
    0, -22, -43, -64, -82, -98, -110, -120, -126, -128, -126, -120, -110, -98, -82, -64, -43, -22
};

const int cos_table[36] = {
    128, 126, 120, 110, 98, 82, 64, 43, 22, 0, -22, -43, -64, -82, -98, -110, -120, -126,
    -128, -126, -120, -110, -98, -82, -64, -43, -22, 0, 22, 43, 64, 82, 98, 110, 120, 126
};

static const unsigned short vga_to_rgb565[16] = {
    0x0000, 0x000F, 0x03E0, 0x03EF, 0x7800, 0x780F, 0x7BE0, 0xC618,
    0x7BEF, 0x381F, 0x07E0, 0x07FF, 0xF800, 0xF81F, 0xFFE0, 0xFFFF
};

static inline unsigned short make_vga_entry(char c, unsigned char color) {
    return (unsigned short)c | ((unsigned short)color << 8);
}

void init_comcow(void) {
    strcpy(comcow.mode_name, "640x480 RGB565 Hardware Framebuffer");
    comcow.width = 80; comcow.height = 25; comcow.refresh_rate = 60;
    comcow.double_buffering = 1; comcow.latency_ms = 1; comcow.active_mode_id = 1;
    log_boot_event("[OK] comcow Display Compositor Engine initialized");
}

void comcow_set_mode(int mode_id) {
    if (mode_id == 1) { strcpy(comcow.mode_name, "80x25 Standard Virtual Text"); comcow.active_mode_id = 1; }
    else if (mode_id == 2) { strcpy(comcow.mode_name, "80x25 High-Refresh Text"); comcow.active_mode_id = 2; }
    else if (mode_id == 3) { strcpy(comcow.mode_name, "80x50 High-Density Text"); comcow.active_mode_id = 3; }
    else if (mode_id == 4) { strcpy(comcow.mode_name, "320x200 Software Framebuffer"); comcow.active_mode_id = 4; }
}

void vga_flush(void) {
    for (int ty = 0; ty < VGA_HEIGHT; ty++) {
        for (int tx = 0; tx < VGA_WIDTH; tx++) {
            int entry_idx = ty * VGA_WIDTH + tx;
            unsigned short entry = vga_backbuffer[entry_idx];

            if (entry == vga_virtual_buffer[entry_idx]) continue;
            vga_virtual_buffer[entry_idx] = entry;

            unsigned char c = (unsigned char)(entry & 0xFF);
            unsigned char color_attr = (entry >> 8) & 0xFF;
            unsigned char bg_idx = (color_attr >> 4) & 0x0F;
            unsigned char fg_idx = color_attr & 0x0F;

            unsigned short bg_color = vga_to_rgb565[bg_idx];
            unsigned short fg_color = vga_to_rgb565[fg_idx];

            int start_x = tx * 8; int start_y = ty * 16 + 40;

            for (int py = 0; py < 16; py++) {
                unsigned char font_row = font8x8_basic[c][py / 2];
                int draw_y = start_y + py;
                if (draw_y >= FB_HEIGHT) continue;

                int row_offset = draw_y * FB_WIDTH;
                for (int px = 0; px < 8; px++) {
                    int draw_x = start_x + px;
                    if (draw_x < FB_WIDTH) {
                        int bit_set = (font_row & (1 << (7 - px))) != 0;
                        fb_pixels[row_offset + draw_x] = bit_set ? fg_color : bg_color;
                    }
                }
            }
        }
    }
#ifdef PL110_LCDCONTROL
    PL110_LCDCONTROL = 0x192D;
#endif
}

void vga_clear(unsigned char color) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_backbuffer[y * VGA_WIDTH + x] = make_vga_entry(' ', color);
        }
    }
}

void vga_force_redraw(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) vga_virtual_buffer[i] = 0xFFFF;
}

void vga_draw_char_at(char c, unsigned char color, int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga_backbuffer[y * VGA_WIDTH + x] = make_vga_entry(c, color);
    }
}

void vga_draw_string(const char* str, unsigned char color, int x, int y) {
    while (*str && x < VGA_WIDTH) {
        vga_draw_char_at(*str, color, x, y);
        x++; str++;
    }
}

void vga_draw_line(int x0, int y0, int x1, int y1, char c, unsigned char color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    int steps = 0;
    while (steps++ < 100) {
        vga_draw_char_at(c, color, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

/* VGA Wireframe Triangle */
void vga_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, char c, unsigned char color) {
    vga_draw_line(x0, y0, x1, y1, c, color);
    vga_draw_line(x1, y1, x2, y2, c, color);
    vga_draw_line(x2, y2, x0, y0, c, color);
}

/* VGA Circle Primitive (Bresenham Algorithm) */
void vga_draw_circle(int cx, int cy, int radius, char c, unsigned char color) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        vga_draw_char_at(c, color, cx + x, cy + y);
        vga_draw_char_at(c, color, cx + y, cy + x);
        vga_draw_char_at(c, color, cx - y, cy + x);
        vga_draw_char_at(c, color, cx - x, cy + y);
        vga_draw_char_at(c, color, cx - x, cy - y);
        vga_draw_char_at(c, color, cx - y, cy - x);
        vga_draw_char_at(c, color, cx + y, cy - x);
        vga_draw_char_at(c, color, cx + x, cy - y);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void draw_box(int start_x, int start_y, int width, int height, unsigned char border_color, unsigned char bg_color) {
    for (int y = start_y; y < start_y + height; y++) {
        for (int x = start_x; x < start_x + width; x++) vga_draw_char_at(' ', bg_color, x, y);
    }
    vga_draw_char_at('+', border_color, start_x, start_y);
    vga_draw_char_at('+', border_color, start_x + width - 1, start_y);
    vga_draw_char_at('+', border_color, start_x, start_y + height - 1);
    vga_draw_char_at('+', border_color, start_x + width - 1, start_y + height - 1);

    for (int x = start_x + 1; x < start_x + width - 1; x++) {
        vga_draw_char_at('-', border_color, x, start_y);
        vga_draw_char_at('-', border_color, x, start_y + height - 1);
    }
    for (int y = start_y + 1; y < start_y + height - 1; y++) {
        vga_draw_char_at('|', border_color, start_x, y);
        vga_draw_char_at('|', border_color, start_x + width - 1, y);
    }
}

static inline unsigned char make_color(unsigned char fg, unsigned char bg) {
    return fg | (bg << 4);
}

void draw_mouse_cursor(void) {
    if (mouse_x >= 0 && mouse_x < VGA_WIDTH && mouse_y >= 0 && mouse_y < VGA_HEIGHT) {
        unsigned char c = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_WHITE, COLOR_BLACK);
        vga_draw_char_at('X', c, mouse_x, mouse_y);
    }
}

static unsigned char rgb_to_vga_color(rgb_color_t color) {
    int r = color.r; int g = color.g; int b = color.b;
    if (r < 64 && g < 64 && b < 64) return COLOR_BLACK;
    if (r > 192 && g > 192 && b > 192) return COLOR_WHITE;

    int max_val = r;
    if (g > max_val) max_val = g;
    if (b > max_val) max_val = b;

    if (max_val == r && g < 128 && b < 128) return (r > 160) ? COLOR_LIGHT_RED : COLOR_RED;
    if (max_val == g && r < 128 && b < 128) return (g > 160) ? COLOR_LIGHT_GREEN : COLOR_GREEN;
    if (max_val == b && r < 128 && b < 128) return (b > 160) ? COLOR_LIGHT_BLUE : COLOR_BLUE;
    if (r > 128 && g > 128 && b < 128) return (r > 180) ? COLOR_YELLOW : COLOR_BROWN;
    if (r > 128 && b > 128 && g < 128) return (r > 180) ? COLOR_LIGHT_MAGENTA : COLOR_MAGENTA;
    if (g > 128 && b > 128 && r < 128) return (g > 180) ? COLOR_LIGHT_CYAN : COLOR_CYAN;

    return (max_val > 160) ? COLOR_LIGHT_GREY : COLOR_DARK_GREY;
}

rgb_framebuffer_t* create_rgb_framebuffer(int width, int height) {
    rgb_framebuffer_t* fb = (rgb_framebuffer_t*)malloc(sizeof(rgb_framebuffer_t));
    if (!fb) return 0;
    fb->width = width; fb->height = height;
    fb->pixels = (rgb_color_t*)malloc(width * height * sizeof(rgb_color_t));
    if (!fb->pixels) { free(fb); return 0; }
    return fb;
}

void free_rgb_framebuffer(rgb_framebuffer_t* fb) {
    if (!fb) return;
    if (fb->pixels) free(fb->pixels);
    free(fb);
}

static void rgb_set_pixel(rgb_framebuffer_t* fb, int x, int y, rgb_color_t color) {
    if (!fb || !fb->pixels) return;
    if (x >= 0 && x < fb->width && y >= 0 && y < fb->height) fb->pixels[y * fb->width + x] = color;
}

void rgb_fill_rect(rgb_framebuffer_t* fb, int rx, int ry, int rw, int rh, rgb_color_t color) {
    for (int y = ry; y < ry + rh; y++) {
        for (int x = rx; x < rx + rw; x++) rgb_set_pixel(fb, x, y, color);
    }
}

/* Fast Integer Color Interpolation (Fixed Point 16-bit) */
static inline rgb_color_t interpolate_rgb(rgb_color_t c1, rgb_color_t c2, int factor_256) {
    rgb_color_t out;
    out.r = (unsigned char)((c1.r * (256 - factor_256) + c2.r * factor_256) >> 8);
    out.g = (unsigned char)((c1.g * (256 - factor_256) + c2.g * factor_256) >> 8);
    out.b = (unsigned char)((c1.b * (256 - factor_256) + c2.b * factor_256) >> 8);
    return out;
}

/* Gradient Line Primitive for RGB Framebuffer */
void rgb_draw_line_gradient(rgb_framebuffer_t* fb, int x0, int y0, rgb_color_t c0, int x1, int y1, rgb_color_t c1) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    int total_steps = (dx > dy) ? dx : dy;
    if (total_steps == 0) {
        rgb_set_pixel(fb, x0, y0, c0);
        return;
    }

    int current_step = 0;
    while (1) {
        int factor = (current_step * 256) / total_steps;
        rgb_color_t c_curr = interpolate_rgb(c0, c1, factor);
        rgb_set_pixel(fb, x0, y0, c_curr);

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
        current_step++;
    }
}

/* RGB Gradient Triangle (Gouraud Shading via Scanline)  */
void rgb_draw_triangle_gradient(rgb_framebuffer_t* fb, 
                                int x0, int y0, rgb_color_t c0,
                                int x1, int y1, rgb_color_t c1,
                                int x2, int y2, rgb_color_t c2) {
    /* Sort vertices by Y ascending (y0 <= y1 <= y2) */
    if (y0 > y1) { int tx = x0; x0 = x1; x1 = tx; int ty = y0; y0 = y1; y1 = ty; rgb_color_t tc = c0; c0 = c1; c1 = tc; }
    if (y0 > y2) { int tx = x0; x0 = x2; x2 = tx; int ty = y0; y0 = y2; y2 = ty; rgb_color_t tc = c0; c0 = c2; c2 = tc; }
    if (y1 > y2) { int tx = x1; x1 = x2; x2 = tx; int ty = y1; y1 = y2; y2 = ty; rgb_color_t tc = c1; c1 = c2; c2 = tc; }

    int total_height = y2 - y0;
    if (total_height == 0) return;

    for (int i = 0; i <= total_height; i++) {
        int second_half = i > (y1 - y0) || y1 == y0;
        int segment_height = second_half ? (y2 - y1) : (y1 - y0);
        if (segment_height == 0) continue;

        int alpha = (i * 256) / total_height;
        int beta  = ((i - (second_half ? (y1 - y0) : 0)) * 256) / segment_height;

        int ax = x0 + (((x2 - x0) * alpha) >> 8);
        int bx = second_half ? (x1 + (((x2 - x1) * beta) >> 8)) : (x0 + (((x1 - x0) * beta) >> 8));

        rgb_color_t ca = interpolate_rgb(c0, c2, alpha);
        rgb_color_t cb = second_half ? interpolate_rgb(c1, c2, beta) : interpolate_rgb(c0, c1, beta);

        if (ax > bx) { int tx = ax; ax = bx; bx = tx; rgb_color_t tc = ca; ca = cb; cb = tc; }

        int scanline_width = bx - ax;
        for (int j = ax; j <= bx; j++) {
            int factor = (scanline_width == 0) ? 0 : (((j - ax) * 256) / scanline_width);
            rgb_color_t col = interpolate_rgb(ca, cb, factor);
            rgb_set_pixel(fb, j, y0 + i, col);
        }
    }
}

void rgb_render_to_vga(rgb_framebuffer_t* fb, int start_x, int start_y) {
    if (!fb || !fb->pixels) return;
    for (int y = 0; y < fb->height; y++) {
        for (int x = 0; x < fb->width; x++) {
            int target_x = start_x + x; int target_y = start_y + y;
            if (target_x < VGA_WIDTH && target_y < VGA_HEIGHT) {
                rgb_color_t col = fb->pixels[y * fb->width + x];
                unsigned char vga_col = rgb_to_vga_color(col);
                vga_draw_char_at(219, vga_col | (COLOR_BLACK << 4), target_x, target_y);
            }
        }
    }
}