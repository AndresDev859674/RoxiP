#include "ui.h"

char term_lines[TERM_ROWS][TERM_COLS];
int term_row_count = 0;
int active_tab = 0;
int hub_hidden = 0;

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
    if (pipe_capture_active) {
        int p_len = strlen(pipe_buffer);
        int t_len = strlen(text);
        if (p_len + t_len + 2 < 4096) {
            if (p_len > 0) { pipe_buffer[p_len++] = '\n'; pipe_buffer[p_len] = '\0'; }
            strcpy(pipe_buffer + p_len, text);
        }
        return;
    }

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

void draw_top_header(void) {
    unsigned char bar_top = (current_theme == THEME_MONOCHROME) ? (COLOR_WHITE | (COLOR_BLACK << 4)) : (COLOR_BLACK | (COLOR_LIGHT_GREY << 4));
    for (int x = 0; x < VGA_WIDTH; x++) vga_draw_char_at(' ', bar_top, x, 0);
    vga_draw_string(" RoxiP Core (comcow)", bar_top, 0, 0);
    const char anim_chars[] = {'|', '/', '-', '\\'};
    char current_anim = anim_chars[(bios_anim_frame / 4) % 4];
    bios_anim_frame++;
    vga_draw_string("Hub", bar_top, 68, 0);
    vga_draw_char_at('[', bar_top, 74, 0);
    vga_draw_char_at(current_anim, (current_theme == THEME_MONOCHROME) ? (COLOR_BLACK | (COLOR_WHITE << 4)) : (COLOR_LIGHT_RED | (COLOR_LIGHT_GREY << 4)), 75, 0);
    vga_draw_char_at(']', bar_top, 76, 0);
}

void redraw_current_view(void) {
    switch (current_mode) {
        case MODE_PERF_CLI:      render_cli(); break;
        case MODE_DESK:          render_desktop(); break;
        case MODE_DESK_WINDOW:   render_perf_window(); break;
        case MODE_FLOATCUBE:     render_floatcube_frame(); break;
        case MODE_MICRO_EDITOR:  render_micro_editor(); break;
        case MODE_ZTOP:          render_ztop_frame(); break;
        case MODE_VFM:           render_vfm(vfm_selected_index); break;
        case MODE_RGBDEMO:       render_rgbdemo(); break;
        default:                 render_cli(); break;
    }
}