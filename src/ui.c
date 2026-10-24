#include "kernel.h"

char term_lines[TERM_ROWS][TERM_COLS];
int term_row_count = 0;
int active_tab = 0;
int hub_hidden = 0;

static inline unsigned char make_color(unsigned char fg, unsigned char bg) {
    return fg | (bg << 4);
}

static void set_hardware_cursor(int x, int y) { (void)x; (void)y; }

static void term_add_line_single(const char* text) {
    if (term_row_count < TERM_ROWS) {
        int i = 0;
        while (text[i] && i < TERM_COLS - 1) { term_lines[term_row_count][i] = text[i]; i++; }
        term_lines[term_row_count][i] = '\0';
        term_row_count++;
    } else {
        for (int r = 0; r < TERM_ROWS - 1; r++) {
            for (int c = 0; c < TERM_COLS; c++) term_lines[r][c] = term_lines[r + 1][c];
        }
        int i = 0;
        while (text[i] && i < TERM_COLS - 1) { term_lines[TERM_ROWS - 1][i] = text[i]; i++; }
        term_lines[TERM_ROWS - 1][i] = '\0';
    }
}

void term_add_line(const char* text) {
    int len = strlen(text);
    int offset = 0;
    int max_chunk = TERM_COLS - 2;

    if (len == 0) { term_add_line_single(""); return; }

    while (offset < len) {
        char chunk[TERM_COLS]; int c_idx = 0;
        while (c_idx < max_chunk && text[offset] != '\0') chunk[c_idx++] = text[offset++];
        chunk[c_idx] = '\0';
        term_add_line_single(chunk);
    }
}

void term_clear_history(void) {
    for (int r = 0; r < TERM_ROWS; r++) term_lines[r][0] = '\0';
    term_row_count = 0;
}

static void draw_top_header(void) {
    unsigned char bar_top = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_BLACK, COLOR_LIGHT_GREY);
    for (int x = 0; x < VGA_WIDTH; x++) vga_draw_char_at(' ', bar_top, x, 0);
    vga_draw_string(" RoxiP Core (comcow)", bar_top, 0, 0);
    const char anim_chars[] = {'|', '/', '-', '\\'};
    char current_anim = anim_chars[(bios_anim_frame / 4) % 4];
    bios_anim_frame++;
    vga_draw_string("Hub", bar_top, 68, 0);
    vga_draw_char_at('[', bar_top, 74, 0);
    vga_draw_char_at(current_anim, (current_theme == THEME_MAC_MONO) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_LIGHT_RED, COLOR_LIGHT_GREY), 75, 0);
    vga_draw_char_at(']', bar_top, 76, 0);
}

void render_desktop(void) {
    if (hub_hidden) {
        vga_clear(make_color(COLOR_BLACK, COLOR_BLACK));
        draw_mouse_cursor();
        vga_flush();
        set_hardware_cursor(79, 24);
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
    set_hardware_cursor(79, 24);
}

void render_perf_window(void) {
    if (hub_hidden) {
        vga_clear(make_color(COLOR_BLACK, COLOR_BLACK));
        draw_mouse_cursor();
        vga_flush();
        set_hardware_cursor(79, 24);
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
    set_hardware_cursor(start_x + 9 + input_pos, prompt_y);
}

void render_cli(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            unsigned int hash = ((x * 37) ^ (y * 109) ^ (x * y * 17)) % 100;
            if (hash < 3) vga_draw_char_at('*', make_color(COLOR_WHITE, COLOR_BLACK), x, y);
            else if (hash < 8) vga_draw_char_at('.', make_color(COLOR_DARK_GREY, COLOR_BLACK), x, y);
            else if (hash < 12) vga_draw_char_at('+', make_color(COLOR_LIGHT_CYAN, COLOR_BLACK), x, y);
            else if (hash < 15) vga_draw_char_at('.', make_color(COLOR_MAGENTA, COLOR_BLACK), x, y);
            else vga_draw_char_at(' ', make_color(COLOR_WHITE, COLOR_BLACK), x, y);
        }
    }

    vga_draw_string("RoxiP OS v0.1 - Monolithic Kernel (perf Terminal)", make_color(COLOR_WHITE, COLOR_BLACK), 0, 0);
    vga_draw_string("Type 'desk' to launch Desktop Hub or 'fetch' for system info.", make_color(COLOR_DARK_GREY, COLOR_BLACK), 0, 1);
    vga_draw_string("Welcome to RoxiP OS! A bare-metal OS based on .bin", make_color(COLOR_LIGHT_CYAN, COLOR_BLACK), 1, 2);

    for (int i = 0; i < term_row_count; i++) vga_draw_string(term_lines[i], make_color(COLOR_LIGHT_GREEN, COLOR_BLACK), 0, 3 + i);

    int prompt_y = 3 + term_row_count;
    vga_draw_string("perf > ", make_color(COLOR_YELLOW, COLOR_BLACK), 0, prompt_y);
    for (int i = 0; i < input_pos; i++) vga_draw_char_at(input_buffer[i], make_color(COLOR_MAGENTA, COLOR_BLACK), 7 + i, prompt_y);

    draw_mouse_cursor();
    vga_flush();
    set_hardware_cursor(7 + input_pos, prompt_y);
}

void render_ztop_frame(void) {
    vga_clear(make_color(COLOR_LIGHT_GREY, COLOR_BLACK));
    unsigned char header_color = make_color(COLOR_BLACK, COLOR_CYAN);
    for (int x = 0; x < VGA_WIDTH; x++) vga_draw_char_at(' ', header_color, x, 0);
    vga_draw_string(" RoxiP ztop v0.1 - a minimal system monitor ", header_color, 0, 0);

    unsigned char bar_label = make_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    unsigned char bar_fill  = make_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    unsigned char bar_empty = make_color(COLOR_DARK_GREY, COLOR_BLACK);

    int cpu_percent = 4 + (timer_ticks % 7);
    vga_draw_string("CPU  [", bar_label, 2, 2);
    int cpu_bars = (cpu_percent * 25) / 100;
    for (int i = 0; i < 25; i++) {
        if (i < cpu_bars) vga_draw_char_at('|', bar_fill, 8 + i, 2);
        else vga_draw_char_at('.', bar_empty, 8 + i, 2);
    }
    vga_draw_char_at(']', bar_label, 33, 2);

    char cpu_str[16]; int_to_str(cpu_percent, cpu_str);
    vga_draw_string(cpu_str, make_color(COLOR_YELLOW, COLOR_BLACK), 35, 2);
    vga_draw_string("%", make_color(COLOR_YELLOW, COLOR_BLACK), 35 + strlen(cpu_str), 2);

    unsigned int used_kb = get_used_heap_bytes() / 1024;
    unsigned int total_kb = HEAP_SIZE / 1024;
    int mem_percent = (used_kb * 100) / total_kb;
    if (mem_percent == 0) mem_percent = 1;

    vga_draw_string("Mem  [", bar_label, 2, 3);
    int mem_bars = (mem_percent * 25) / 100;
    for (int i = 0; i < 25; i++) {
        if (i <= mem_bars) vga_draw_char_at('|', bar_fill, 8 + i, 3);
        else vga_draw_char_at('.', bar_empty, 8 + i, 3);
    }
    vga_draw_char_at(']', bar_label, 33, 3);

    char used_str[16], total_str[16];
    int_to_str(used_kb, used_str); int_to_str(total_kb, total_str);
    vga_draw_string(used_str, make_color(COLOR_YELLOW, COLOR_BLACK), 35, 3);
    vga_draw_string("KB/", make_color(COLOR_YELLOW, COLOR_BLACK), 35 + strlen(used_str), 3);
    vga_draw_string(total_str, make_color(COLOR_YELLOW, COLOR_BLACK), 38 + strlen(used_str), 3);
    vga_draw_string("KB", make_color(COLOR_YELLOW, COLOR_BLACK), 38 + strlen(used_str) + strlen(total_str), 3);

    vga_draw_string("Tasks: 6 total, 6 running", make_color(COLOR_WHITE, COLOR_BLACK), 45, 2);
    vga_draw_string("comcow: 60Hz PL110 Framebuffer", make_color(COLOR_WHITE, COLOR_BLACK), 45, 3);

    unsigned char th_color = make_color(COLOR_BLACK, COLOR_LIGHT_GREY);
    for (int x = 0; x < VGA_WIDTH; x++) vga_draw_char_at(' ', th_color, x, 5);
    vga_draw_string("  PID  USER       CPU%   MEM (KB)   STATE     COMMAND", th_color, 0, 5);

    process_info_t procs[6] = {
        { 1, "root", cpu_percent - 2, 128, "RUNNING", "arm_kernel" },
        { 2, "root", 1, 64,  "RUNNING", "comcow_wm" },
        { 3, "user", 2, 32,  "RUNNING", "perf_shell" },
        { 4, "root", 1, 16,  "RUNNING", "pl011_serial" },
        { 5, "root", 0, 48,  "SLEEP",   "vfs_engine" },
        { 6, "root", 0, 32,  "RUNNING", "arm_audio" }
    };

    for (int i = 0; i < 6; i++) {
        int y = 6 + i;
        unsigned char proc_color = (i % 2 == 0) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
        char pid_s[16], cpu_s[16], mem_s[16];
        int_to_str(procs[i].pid, pid_s); int_to_str(procs[i].cpu_usage, cpu_s); int_to_str(procs[i].mem_kb, mem_s);
        vga_draw_string(pid_s, proc_color, 4, y);
        vga_draw_string(procs[i].user, proc_color, 9, y);
        vga_draw_string(cpu_s, proc_color, 20, y);
        vga_draw_string("%", proc_color, 20 + strlen(cpu_s), y);
        vga_draw_string(mem_s, proc_color, 29, y);
        vga_draw_string(procs[i].state, proc_color, 40, y);
        vga_draw_string(procs[i].name, proc_color, 50, y);
    }

    unsigned char footer_color = make_color(COLOR_BLACK, COLOR_CYAN);
    for (int x = 0; x < VGA_WIDTH; x++) vga_draw_char_at(' ', footer_color, x, 24);
    vga_draw_string(" Press [Q] or [ESC] to exit ztop and return to perf Terminal ", footer_color, 1, 24);

    draw_mouse_cursor();
    set_hardware_cursor(79, 24);
    vga_flush();
}

void render_micro_editor(void) {
    vga_clear(make_color(COLOR_BLACK, COLOR_BLUE));
    unsigned char top_color = make_color(COLOR_BLACK, COLOR_LIGHT_GREY);
    for (int x = 0; x < VGA_WIDTH; x++) vga_draw_char_at(' ', top_color, x, 0);
    vga_draw_string(" micro - ", top_color, 1, 0);
    vga_draw_string(micro_filename[0] ? micro_filename : "Untitled", top_color, 10, 0);

    unsigned char bot_color = make_color(COLOR_BLACK, COLOR_CYAN);
    for (int x = 0; x < VGA_WIDTH; x++) vga_draw_char_at(' ', bot_color, x, 24);
    vga_draw_string(" Ctrl+S: Save  |  Ctrl+X: Exit ", bot_color, 1, 24);

    unsigned char text_color = make_color(COLOR_WHITE, COLOR_BLUE);
    int cur_x = 2, cur_y = 2;

    for (int i = 0; i < micro_pos; i++) {
        if (micro_buffer[i] == '\n') {
            cur_y++; cur_x = 2;
        } else {
            vga_draw_char_at(micro_buffer[i], text_color, cur_x, cur_y);
            cur_x++;
            if (cur_x >= VGA_WIDTH - 2) { cur_y++; cur_x = 2; }
        }
    }
    set_hardware_cursor(cur_x, cur_y);
    vga_flush();
}

void open_micro_editor(const char* filename) {
    strcpy(micro_filename, filename);
    micro_pos = 0; micro_buffer[0] = '\0';
    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, filename) == 0 && !fs_table[i].is_dir) {
            strcpy(micro_buffer, fs_table[i].content);
            micro_pos = strlen(micro_buffer);
            break;
        }
    }
    current_mode = MODE_MICRO_EDITOR;
    render_micro_editor();
}

static void draw_line_3d(int x0, int y0, int x1, int y1, char ch, unsigned char color) {
    if (x0 < 0) {
        x0 = 0;
    }
    if (x0 >= VGA_WIDTH) {
        x0 = VGA_WIDTH - 1;
    }
    if (x1 < 0) {
        x1 = 0;
    }
    if (x1 >= VGA_WIDTH) {
        x1 = VGA_WIDTH - 1;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (y0 >= VGA_HEIGHT) {
        y0 = VGA_HEIGHT - 1;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    if (y1 >= VGA_HEIGHT) {
        y1 = VGA_HEIGHT - 1;
    }

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

int validate_3d_pipeline(void) {
    if (cos_table[0] != 128 || sin_table[0] != 0 || sin_table[9] != 128) return 0;
    if (free_list == 0) return 0;
    return 1;
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

        if (px < 0) {
            px = 0;
        }
        if (px >= VGA_WIDTH) {
            px = VGA_WIDTH - 1;
        }
        if (py < 0) {
            py = 0;
        }
        if (py >= VGA_HEIGHT) {
            py = VGA_HEIGHT - 1;
        }

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

void redraw_current_view(void) {
    if (current_mode == MODE_PERF_CLI) render_cli();
    else if (current_mode == MODE_DESK) render_desktop();
    else if (current_mode == MODE_DESK_WINDOW) render_perf_window();
    else if (current_mode == MODE_FLOATCUBE) render_floatcube_frame();
    else if (current_mode == MODE_MICRO_EDITOR) render_micro_editor();
    else if (current_mode == MODE_ZTOP) render_ztop_frame();
}