#include "ui.h"

static inline unsigned char make_color(unsigned char fg, unsigned char bg) {
    return fg | (bg << 4);
}

static void draw_line_3d(int x0, int y0, int x1, int y1, char ch, unsigned char color) {
    if (x0 < 0) x0 = 0;
    if (x0 >= VGA_WIDTH) x0 = VGA_WIDTH - 1;
    if (x1 < 0) x1 = 0;
    if (x1 >= VGA_WIDTH) x1 = VGA_WIDTH - 1;
    if (y0 < 0) y0 = 0;
    if (y0 >= VGA_HEIGHT) y0 = VGA_HEIGHT - 1;
    if (y1 < 0) y1 = 0;
    if (y1 >= VGA_HEIGHT) y1 = VGA_HEIGHT - 1;

    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    int max_steps = 100;
    while (max_steps-- > 0) {
        vga_draw_char_at(ch, color, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

void render_floatcube_frame(void) {
    static int angle_x = 0, angle_y = 0, angle_z = 0;
    static unsigned int frame_counter = 0;

    frame_counter++;
    angle_x = (angle_x + 1) % 36; angle_y = (angle_y + 2) % 36; angle_z = (angle_z + 1) % 36;

    vga_clear(make_color(COLOR_BLACK, COLOR_BLACK));
    vga_draw_string("=== RoxiP 3D Software Pipeline (ARM Fixed-Point) ===", make_color(COLOR_YELLOW, COLOR_BLACK), 14, 1);
    vga_draw_string("Press [ESC] or [Q] to return to perf Terminal", make_color(COLOR_DARK_GREY, COLOR_BLACK), 18, 2);

    int verts[8][3] = {
        {-18, -18, -18}, { 18, -18, -18}, { 18,  18, -18}, {-18,  18, -18},
        {-18, -18,  18}, { 18, -18,  18}, { 18,  18,  18}, {-18,  18,  18}
    };
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };

    int proj_x[8], proj_y[8];
    unsigned char rgb_colors[6] = { COLOR_LIGHT_RED, COLOR_LIGHT_GREEN, COLOR_YELLOW, COLOR_LIGHT_BLUE, COLOR_LIGHT_MAGENTA, COLOR_LIGHT_CYAN };

    for (int i = 0; i < 8; i++) {
        int x = verts[i][0]; int y = verts[i][1]; int z = verts[i][2];
        int x1 = (x * cos_table[angle_y] - z * sin_table[angle_y]) / 128;
        int z1 = (x * sin_table[angle_y] + z * cos_table[angle_y]) / 128;
        int y2 = (y * cos_table[angle_x] - z1 * sin_table[angle_x]) / 128;
        int z2 = (y * sin_table[angle_x] + z1 * cos_table[angle_x]) / 128;
        int x3 = (x1 * cos_table[angle_z] - y2 * sin_table[angle_z]) / 128;
        int y3 = (x1 * sin_table[angle_z] + y2 * cos_table[angle_z]) / 128;

        int fov = 90; int distance = z2 + 120;
        if (distance < 10) distance = 10;

        int px = 40 + (x3 * fov) / distance;
        int py = 13 + (y3 * (fov / 2)) / distance;

        if (px < 0) px = 0;
        if (px >= VGA_WIDTH) px = VGA_WIDTH - 1;
        if (py < 0) py = 0;
        if (py >= VGA_HEIGHT) py = VGA_HEIGHT - 1;

        proj_x[i] = px; proj_y[i] = py;
    }

    for (int e = 0; e < 12; e++) {
        int v0 = edges[e][0]; int v1 = edges[e][1];
        unsigned char edge_color = rgb_colors[(e + (frame_counter / 3)) % 6];
        draw_line_3d(proj_x[v0], proj_y[v0], proj_x[v1], proj_y[v1], '#', make_color(edge_color, COLOR_BLACK));
    }

    vga_draw_string("Stats:", make_color(COLOR_WHITE, COLOR_BLACK), 2, 21);
    vga_draw_string("Engine: ARM32 Fixed-Point 3D Pipeline (3-Axis)", make_color(COLOR_LIGHT_CYAN, COLOR_BLACK), 2, 22);
    vga_draw_string("Status: Triple-Fault Guard Active", make_color(COLOR_LIGHT_GREEN, COLOR_BLACK), 2, 23);
    vga_flush();
}

void render_rgbdemo(void) {
    vga_clear(make_color(COLOR_BLACK, COLOR_BLACK));
    vga_draw_string("=== RoxiP Framebuffer RGB Color Test ===", make_color(COLOR_WHITE, COLOR_BLACK), 20, 2);

    unsigned char colors[8] = {
        COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_CYAN,
        COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE, COLOR_LIGHT_GREY
    };

    for (int i = 0; i < 8; i++) {
        int start_x = 5 + (i * 9);
        for (int y = 6; y < 18; y++) {
            for (int x = start_x; x < start_x + 8; x++) {
                vga_draw_char_at('#', make_color(colors[i], COLOR_BLACK), x, y);
            }
        }
    }

    vga_draw_string("Press [ESC] or [Q] to return to perf Terminal", make_color(COLOR_DARK_GREY, COLOR_BLACK), 18, 22);
    vga_flush();
}