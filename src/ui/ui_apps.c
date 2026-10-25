#include "ui.h"

int vfm_selected_index = 0;

static inline unsigned char make_color(unsigned char fg, unsigned char bg) {
    return fg | (bg << 4);
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
        { 5, "root", 0, 48,  "SLEEP",   "yafs_engine" },
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
    vga_flush();
}

static void mini_snprintf_vfm(char* buf, const char* name, int size) {
    char sz_str[16];
    int_to_str(size, sz_str);
    int pos = 0;
    buf[pos++] = ' ';
    
    int n_len = strlen(name);
    for (int i = 0; i < 18; i++) {
        if (i < n_len) buf[pos++] = name[i];
        else buf[pos++] = ' ';
    }
    
    buf[pos++] = ' ';
    int s_len = strlen(sz_str);
    for (int i = 0; i < 6 - s_len; i++) buf[pos++] = ' ';
    for (int i = 0; i < s_len; i++) buf[pos++] = sz_str[i];
    
    buf[pos++] = 'B';
    buf[pos++] = ' ';
    buf[pos] = '\0';
}

void render_vfm(int selected_index) {
    vga_clear(make_color(COLOR_WHITE, COLOR_BLACK));

    vga_draw_string(" RoxiP File Manager v0.1 (vfm) - YAFS Explorer ", make_color(COLOR_BLACK, COLOR_CYAN), 0, 0);

    vga_draw_string("+--- YAFS Storage (pflash) ---+", make_color(COLOR_LIGHT_GREEN, COLOR_BLACK), 2, 2);
    int active_files = 0;
    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used) {
            char line[40];
            mini_snprintf_vfm(line, fs_table[i].name, fs_table[i].size);
            if (active_files == selected_index) {
                vga_draw_string(line, make_color(COLOR_BLACK, COLOR_LIGHT_GREEN), 3, 4 + active_files);
            } else {
                vga_draw_string(line, make_color(COLOR_WHITE, COLOR_BLACK), 3, 4 + active_files);
            }
            active_files++;
        }
    }

    vga_draw_string("+--- Synthetic Nodes ---+", make_color(COLOR_LIGHT_CYAN, COLOR_BLACK), 42, 2);
    vga_draw_string(" @cpu   (CPU Architecture) ", make_color(COLOR_DARK_GREY, COLOR_BLACK), 43, 4);
    vga_draw_string(" @ram   (Heap Memory Usage)", make_color(COLOR_DARK_GREY, COLOR_BLACK), 43, 5);
    vga_draw_string(" @gpu   (comcow Compositor)", make_color(COLOR_DARK_GREY, COLOR_BLACK), 43, 6);
    vga_draw_string(" @fb0   (Framebuffer Base) ", make_color(COLOR_DARK_GREY, COLOR_BLACK), 43, 7);
    vga_draw_string(" @yafs   (Storage Metadata) ", make_color(COLOR_DARK_GREY, COLOR_BLACK), 43, 8);

    vga_draw_string(" [UP/DOWN] Select  [ENTER] View (cat)  [ESC/Q] Exit back to perf ", make_color(COLOR_BLACK, COLOR_LIGHT_GREY), 0, 24);
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
    vga_draw_string(" Ctrl+O / Ctrl+S: Save  |  ESC / Ctrl+Q / Ctrl+X: Exit ", bot_color, 1, 24);

    unsigned char num_color = make_color(COLOR_LIGHT_GREY, COLOR_BLUE);

    int line_num = 1;
    int cur_x = 5, cur_y = 2;
    int in_comment = 0;
    int in_string = 0;

    char num_str[16];
    char formatted[16];
    int_to_str(line_num, num_str);
    int l_len = strlen(num_str);

    if (l_len == 1) { formatted[0] = ' '; formatted[1] = ' '; formatted[2] = num_str[0]; }
    else if (l_len == 2) { formatted[0] = ' '; formatted[1] = num_str[0]; formatted[2] = num_str[1]; }
    else { formatted[0] = num_str[0]; formatted[1] = num_str[1]; formatted[2] = num_str[2]; }
    formatted[3] = ' ';
    formatted[4] = '\0';
    vga_draw_string(formatted, num_color, 0, cur_y);

    for (int i = 0; i < micro_pos; i++) {
        char ch = micro_buffer[i];
        if (ch == '\n') {
            cur_y++;
            line_num++;
            cur_x = 5;
            in_comment = 0;
            in_string = 0;

            if (cur_y < 24) {
                int_to_str(line_num, num_str);
                l_len = strlen(num_str);
                if (l_len == 1) { formatted[0] = ' '; formatted[1] = ' '; formatted[2] = num_str[0]; }
                else if (l_len == 2) { formatted[0] = ' '; formatted[1] = num_str[0]; formatted[2] = num_str[1]; }
                else { formatted[0] = num_str[0]; formatted[1] = num_str[1]; formatted[2] = num_str[2]; }
                formatted[3] = ' ';
                formatted[4] = '\0';
                vga_draw_string(formatted, num_color, 0, cur_y);
            }
        } else {
            if (ch == '#') in_comment = 1;
            if (ch == '"') in_string = !in_string;

            unsigned char text_color = make_color(COLOR_WHITE, COLOR_BLUE);
            if (in_comment) {
                text_color = make_color(COLOR_LIGHT_GREEN, COLOR_BLUE);
            } else if (in_string || ch == '"') {
                text_color = make_color(COLOR_LIGHT_CYAN, COLOR_BLUE);
            } else if (ch == '=' || ch == '>' || ch == '|' || ch == '@') {
                text_color = make_color(COLOR_LIGHT_MAGENTA, COLOR_BLUE);
            } else {
                if ((i == 0 || micro_buffer[i - 1] == ' ' || micro_buffer[i - 1] == '\n') &&
                    (strncmp(&micro_buffer[i], "set", 3) == 0 ||
                     strncmp(&micro_buffer[i], "echo", 4) == 0 ||
                     strncmp(&micro_buffer[i], "fetch", 5) == 0 ||
                     strncmp(&micro_buffer[i], "clear", 5) == 0 ||
                     strncmp(&micro_buffer[i], "desk", 4) == 0 ||
                     strncmp(&micro_buffer[i], "stat", 4) == 0 ||
                     strncmp(&micro_buffer[i], "xxd", 3) == 0 ||
                     strncmp(&micro_buffer[i], "disasm", 6) == 0 ||
                     strncmp(&micro_buffer[i], "sh", 2) == 0)) {
                    text_color = make_color(COLOR_YELLOW, COLOR_BLUE);
                }
            }

            if (cur_y < 24 && cur_x < VGA_WIDTH - 1) {
                vga_draw_char_at(ch, text_color, cur_x, cur_y);
            }
            cur_x++;
            if (cur_x >= VGA_WIDTH - 2) { cur_y++; cur_x = 5; }
        }
    }

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