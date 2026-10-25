#include "ui.h"

static int shooting_star_active = 0;
static int shooting_star_x = 0;
static int shooting_star_y = 0;
static int shooting_star_frame = 0;

static inline unsigned char make_color(unsigned char fg, unsigned char bg) {
    return fg | (bg << 4);
}

void render_cli(void) {
    timer_ticks++;

    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            unsigned int seed = ((x * 37) ^ (y * 109) ^ (x * y * 17)) % 1000;
            unsigned int nebula_hash = ((x * 13 + y * 29) + (x * y * 7)) % 100;

            if (nebula_hash < 12) {
                unsigned char neb_col = (nebula_hash < 4) ? COLOR_MAGENTA : 
                                        (nebula_hash < 8 ? COLOR_BLUE : COLOR_CYAN);
                vga_draw_char_at('.', make_color(neb_col, COLOR_BLACK), x, y);
            } 
            else if (seed < 15) {
                int sparkle_phase = (seed + (timer_ticks / 4)) % 4;
                char sparkle_char = '.';
                unsigned char sparkle_col = COLOR_DARK_GREY;

                if (sparkle_phase == 0) { sparkle_char = '*'; sparkle_col = COLOR_WHITE; }
                else if (sparkle_phase == 1) { sparkle_char = '+'; sparkle_col = COLOR_LIGHT_CYAN; }
                else if (sparkle_phase == 2) { sparkle_char = '.'; sparkle_col = COLOR_LIGHT_MAGENTA; }

                vga_draw_char_at(sparkle_char, make_color(sparkle_col, COLOR_BLACK), x, y);
            } 
            else if (seed < 45) {
                vga_draw_char_at('.', make_color(COLOR_DARK_GREY, COLOR_BLACK), x, y);
            } 
            else {
                vga_draw_char_at(' ', make_color(COLOR_WHITE, COLOR_BLACK), x, y);
            }
        }
    }

    if (!shooting_star_active && (timer_ticks % 45 == 0)) {
        shooting_star_active = 1;
        shooting_star_x = 5 + (timer_ticks * 11) % 50;
        shooting_star_y = 1 + (timer_ticks * 7) % 10;
        shooting_star_frame = 0;
    }

    if (shooting_star_active) {
        int cur_x = shooting_star_x + (shooting_star_frame * 2);
        int cur_y = shooting_star_y + shooting_star_frame;

        if (cur_x < VGA_WIDTH - 2 && cur_y < VGA_HEIGHT - 1) {
            vga_draw_char_at('*', make_color(COLOR_WHITE, COLOR_BLACK), cur_x, cur_y);
            
            if (cur_x - 1 >= 0 && cur_y - 1 >= 0)
                vga_draw_char_at('\\', make_color(COLOR_YELLOW, COLOR_BLACK), cur_x - 1, cur_y - 1);
            if (cur_x - 2 >= 0 && cur_y - 2 >= 0)
                vga_draw_char_at('.', make_color(COLOR_LIGHT_RED, COLOR_BLACK), cur_x - 2, cur_y - 2);

            shooting_star_frame++;
            if (shooting_star_frame > 6) {
                shooting_star_active = 0;
            }
        } else {
            shooting_star_active = 0;
        }
    }

    vga_draw_string("RoxiP OS v0.1 - Monolithic Kernel (perf Terminal)", make_color(COLOR_WHITE, COLOR_BLACK), 0, 0);
    vga_draw_string("Type 'desk' to launch Desktop Hub or 'fetch' for system info.", make_color(COLOR_DARK_GREY, COLOR_BLACK), 0, 1);
    vga_draw_string("Welcome to RoxiP OS! A bare-metal OS based on .bin", make_color(COLOR_LIGHT_CYAN, COLOR_BLACK), 1, 2);

    for (int i = 0; i < term_row_count; i++) {
        vga_draw_string(term_lines[i], make_color(COLOR_LIGHT_GREEN, COLOR_BLACK), 0, 3 + i);
    }

    int prompt_y = 3 + term_row_count;
    vga_draw_string("perf > ", make_color(COLOR_YELLOW, COLOR_BLACK), 0, prompt_y);
    for (int i = 0; i < input_pos; i++) {
        vga_draw_char_at(input_buffer[i], make_color(COLOR_MAGENTA, COLOR_BLACK), 7 + i, prompt_y);
    }

    draw_mouse_cursor();
    vga_flush();
}