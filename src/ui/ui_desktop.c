#include "ui.h"

static inline unsigned char make_color(unsigned char fg, unsigned char bg) {
    return fg | (bg << 4);
}

void render_desktop(void) {
    if (hub_hidden) {
        vga_clear(make_color(COLOR_BLACK, COLOR_BLACK));
        draw_mouse_cursor();
        vga_flush();
        return;
    }
    if (current_theme == THEME_MAC_MONO) {
        unsigned char mac_dither = make_color(COLOR_DARK_GREY, COLOR_WHITE);
        for (int y = 0; y < VGA_HEIGHT; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) vga_draw_char_at((char)MAC_PATTERN, mac_dither, x, y);
        }
    } else {
        vga_clear(make_color(COLOR_WHITE, COLOR_BLUE));
    }

    draw_top_header();
    const char* tabs[3] = {" Main ", " Settings ", " Exit "};
    int tab_x[3] = {2, 10, 22};
    for (int i = 0; i < 3; i++) {
        unsigned char tab_color = (current_theme == THEME_MAC_MONO) ? ((i == active_tab) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_BLACK, COLOR_WHITE)) : ((i == active_tab) ? make_color(COLOR_WHITE, COLOR_BLUE) : make_color(COLOR_BLACK, COLOR_LIGHT_GREY));
        vga_draw_string(tabs[i], tab_color, tab_x[i], 1);
    }

    unsigned char box_border = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_LIGHT_CYAN, COLOR_BLUE);
    unsigned char box_bg     = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_WHITE, COLOR_BLUE);
    draw_box(2, 3, 50, 15, box_border, box_bg);

    if (active_tab == 0) {
        vga_draw_string(" System Information ", (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_YELLOW, COLOR_BLUE), 5, 3);
        vga_draw_string("CPU Model      : ", box_bg, 4, 5); vga_draw_string(cpu_model_string, box_bg, 21, 5);
        vga_draw_string("Primary Disk   : ", box_bg, 4, 7); vga_draw_string(disk_model_string, box_bg, 21, 7);
        vga_draw_string("Compositor     : comcow Engine", box_bg, 4, 9);
        vga_draw_string("System Date    : [10/24/2026]", box_bg, 4, 10);
        vga_draw_string("File System    : VFS (FAT32 / VFAT supports)", box_bg, 4, 12);
        vga_draw_string("Console Shell  : perf Terminal (perf >)", box_bg, 4, 13);
    } else if (active_tab == 1) {
        vga_draw_string(" Settings & Themes ", (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_YELLOW, COLOR_BLUE), 5, 3);
        vga_draw_string("Select Desktop Theme:", box_bg, 4, 5);
        vga_draw_string(current_theme == THEME_DEFAULT ? "[X] 1. Default Blue Classic" : "[ ] 1. Default Blue Classic", box_bg, 6, 7);
        vga_draw_string(current_theme == THEME_MAC_MONO ? "[X] 2. Monochrome Theme" : "[ ] 2. Monochrome Theme", box_bg, 6, 9);
        vga_draw_string("Press [1] or [2] to apply theme.", box_bg, 4, 12);
    } else {
        vga_draw_string(" Exit RoxiP Desk ", (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_YELLOW, COLOR_BLUE), 5, 3);
        vga_draw_string("Press [ENTER] to exit Desk mode and return", box_bg, 4, 6);
        vga_draw_string("to perf Terminal / CLI shell environment.", box_bg, 4, 7);
    }

    draw_box(53, 3, 25, 15, box_border, box_bg);
    vga_draw_string(" Controls ", (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_YELLOW, COLOR_BLUE), 55, 3);
    vga_draw_string("Tab / Arrows:", box_bg, 55, 5); vga_draw_string("Switch Tabs", box_bg, 55, 6);
    vga_draw_string("ENTER on Exit:", box_bg, 55, 8); vga_draw_string("Close Desk mode", box_bg, 55, 9);
    vga_draw_string("Right Ctrl: Toggle", box_bg, 55, 11); vga_draw_string("empty black screen", box_bg, 55, 12);

    unsigned char bar_bottom = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_BLACK, COLOR_CYAN);
    for (int x = 0; x < VGA_WIDTH; x++) vga_draw_char_at(' ', bar_bottom, x, VGA_HEIGHT - 1);
    vga_draw_string(" Tab/Arrows: Nav | Enter: Action | R-Ctrl: Hide | Alt/F1: perf Terminal ", bar_bottom, 1, VGA_HEIGHT - 1);

    draw_mouse_cursor();
    vga_flush();
}

void render_perf_window(void) {
    if (hub_hidden) {
        vga_clear(make_color(COLOR_BLACK, COLOR_BLACK));
        draw_mouse_cursor();
        vga_flush();
        return;
    }
    draw_top_header();

    unsigned char bg_blue = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_WHITE, COLOR_BLUE);
    for (int y = 1; y < 24; y++) {
        vga_draw_char_at(' ', bg_blue, 0, y);
        vga_draw_char_at(' ', bg_blue, 79, y);
    }

    unsigned char bar_bottom = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_BLACK, COLOR_CYAN);
    for (int x = 0; x < VGA_WIDTH; x++) vga_draw_char_at(' ', bar_bottom, x, VGA_HEIGHT - 1);
    vga_draw_string(" Alt/F1: Back to Hub  |  R-Ctrl: Hide Hub  |  Type 'help' for commands ", bar_bottom, 1, VGA_HEIGHT - 1);

    int start_x = 1, start_y = 1, width = 78, height = 23;
    unsigned char win_bg = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
    unsigned char border_color = make_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    unsigned char prompt_color = make_color(COLOR_LIGHT_GREEN, COLOR_BLACK);

    for (int y = start_y; y < start_y + height; y++) {
        for (int x = start_x; x < start_x + width; x++) vga_draw_char_at(' ', win_bg, x, y);
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

    vga_draw_string("[ perf Terminal ]", make_color(COLOR_YELLOW, COLOR_BLACK), start_x + 30, start_y);
    for (int i = 0; i < term_row_count; i++) vga_draw_string(term_lines[i], win_bg, start_x + 2, start_y + 1 + i);

    int prompt_y = start_y + 1 + term_row_count;
    vga_draw_string("perf > ", prompt_color, start_x + 2, prompt_y);
    for (int i = 0; i < input_pos; i++) vga_draw_char_at(input_buffer[i], win_bg, start_x + 9 + i, prompt_y);

    draw_mouse_cursor();
    vga_flush();
}