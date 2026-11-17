#include "kernel.h"
#include "hal.h"
#include "drivers.h"

system_mode_t current_mode = MODE_PERF_CLI;
desk_theme_t current_theme = THEME_DEFAULT;

char input_buffer[128];
int input_pos = 0;

char micro_filename[MAX_FILENAME_LEN];
char micro_buffer[MAX_FILE_CONTENT];
int micro_pos = 0;

char cpu_model_string[64] = "Detecting CPU...";
char gpu_model_string[64] = "Detecting GPU...";
char audio_model_string[64] = "Detecting Audio Hardware...";
char disk_model_string[64] = "Detecting Primary Disk...";
unsigned int timer_ticks = 0;

int audio_initialized = 0;
char boot_logs[MAX_BOOT_LOGS][74];
int boot_log_count = 0;

int bios_anim_frame = 0;
int mouse_x = 40;
int mouse_y = 12;
int dbg_line = 0;

static inline unsigned char make_color(unsigned char fg, unsigned char bg) {
    return fg | (bg << 4);
}

void log_boot_event(const char* msg) {
    if (boot_log_count < MAX_BOOT_LOGS) {
        int i = 0;
        while (msg[i] && i < 73) { boot_logs[boot_log_count][i] = msg[i]; i++; }
        boot_logs[boot_log_count][i] = '\0';
        boot_log_count++;
    }
}

void dbg_log(const char* msg) {
    uart_puts("[BOOT DBG] "); uart_puts(msg); uart_puts("\r\n");
    int offset = dbg_line * VGA_WIDTH;
    for (int i = 0; i < VGA_WIDTH; i++) vga_virtual_buffer[offset + i] = (unsigned short)' ' | (COLOR_BLACK << 8);
    for (int i = 0; msg[i] != '\0' && i < VGA_WIDTH; i++) vga_virtual_buffer[offset + i] = (unsigned short)msg[i] | (COLOR_YELLOW << 8);

    dbg_line++;
    if (dbg_line >= 25) dbg_line = 0;
}

static void run_bootloader(void) {
    int seconds_remaining = 10;
    int selected_option = 0;

    unsigned char logo_color  = make_color(COLOR_LIGHT_MAGENTA, COLOR_BLACK);
    unsigned char title_color = make_color(COLOR_YELLOW, COLOR_BLACK);
    unsigned char timer_color = make_color(COLOR_LIGHT_CYAN, COLOR_BLACK);

    while (seconds_remaining > 0) {
        vga_clear(make_color(COLOR_WHITE, COLOR_BLACK));
        for (int y = 0; y < VGA_HEIGHT; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                unsigned int hash = ((x * 37) ^ (y * 109) ^ (x * y * 17)) % 100;
                if (hash < 2) vga_draw_char_at('*', make_color(COLOR_WHITE, COLOR_BLACK), x, y);
                else if (hash < 6) vga_draw_char_at('.', make_color(COLOR_DARK_GREY, COLOR_BLACK), x, y);
            }
        }

        vga_draw_string("  /\\  ", logo_color, 37, 2);
        vga_draw_string(" /  \\ ", logo_color, 37, 3);
        vga_draw_string("/ /\\ \\", logo_color, 37, 4);
        vga_draw_string("\\ \\/ /", logo_color, 37, 5);
        vga_draw_string(" \\  / ", logo_color, 37, 6);
        vga_draw_string("  \\/  ", logo_color, 37, 7);
        
        vga_draw_string("=== RoxiP Boot loader v0.1 (Core) ===", title_color, 22, 9);

        unsigned char opt1_color = (selected_option == 0) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_WHITE, COLOR_BLACK);
        unsigned char opt2_color = (selected_option == 1) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_WHITE, COLOR_BLACK);

        vga_draw_string(selected_option == 0 ? " > [1] Boot RoxiP OS   " : "   [1] Boot RoxiP OS   ", opt1_color, 28, 12);
        vga_draw_string(selected_option == 1 ? " > [2] Reboot System   " : "   [2] Reboot System   ", opt2_color, 28, 14);

        char timer_str[80] = "Auto-booting in ";
        char sec_buf[16]; int_to_str(seconds_remaining, sec_buf);
        strcpy(timer_str + 16, sec_buf);
        strcpy(timer_str + 16 + strlen(sec_buf), "s... (Arrows=Select, ENTER=Confirm)");

        vga_draw_string(timer_str, timer_color, 16, 17);
        vga_flush();

        int key_interrupted = 0;
        for (int step = 0; step < 20; step++) {
            char c = get_keyboard_input();
            if (c) {
                if (c == KEY_UP) { selected_option = 0; key_interrupted = 1; break; }
                else if (c == KEY_DOWN) { selected_option = 1; key_interrupted = 1; break; }
                else if (c == '1') { selected_option = 0; return; }
                else if (c == '2') system_reboot();
                else if (c == '\n' || c == '\r') {
                    if (selected_option == 0) return;
                    else system_reboot();
                }
            }
            delay_ms(50);
        }
        if (!key_interrupted) seconds_remaining--;
    }
}

void kmain(void) {
    dbg_line = 0;
    
    /* Step 01: Initialize Board HAL & ARM Exception Vector Table first for Kernel Panic protection */
    dbg_log("[01/12] Initializing Board HAL & Exception Vectors...");
    board_init();

    dbg_log("[02/12] Calling init_heap()...");
    init_heap();

    dbg_log("[03/12] Calling init_env_vars()...");
    init_env_vars();

    dbg_log("[04/12] Querying CPU Architecture...");
    detect_cpu_brand(cpu_model_string);

    dbg_log("[05/12] Initializing PL110 LCD Framebuffer...");
    init_pl110_lcd();
    detect_gpu(gpu_model_string);

    dbg_log("[05b] Initializing HDMI/DVI Display...");
    init_hdmi_dvi();

    dbg_log("[06/12] Checking Audio Engine...");
    init_hda_audio();

    dbg_log("[07/12] Querying Storage Controller...");
    detect_primary_disk(disk_model_string);

    dbg_log("[08/12] Mounting File Systems...");
    init_filesystems();

    dbg_log("[09/12] Initializing comcow Compositor...");
    init_comcow();

    dbg_log("[10/12] Initializing UART Serial Input Driver...");
    log_boot_event("[OK] ARM PL011 UART Serial Input Active");

    dbg_log("[11/12] Initializing KMI Input Drivers...");
    init_kmi_keyboard();
    init_kmi_mouse();

    dbg_log("[12/12] Launching Interactive Bootloader...");
    run_bootloader();

    dbg_log("Launching CLI Shell View...");
    redraw_current_view();

    while (1) {
        timer_ticks++;
        poll_kmi_mouse();

        if (current_mode == MODE_PERF_CLI || current_mode == MODE_FLOATCUBE || current_mode == MODE_RGBDEMO) {
            redraw_current_view();
            delay_ms(16);
        }

        if (current_mode == MODE_FLOATCUBE) {
            redraw_current_view();
            delay_cycles(150000);
        }
        else if (current_mode == MODE_ZTOP && (timer_ticks % 1000 == 0)) {
            redraw_current_view();
        }

        char c = get_keyboard_input();
        if (c) process_char_input(c);
    }
}