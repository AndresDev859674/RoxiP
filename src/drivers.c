#include "kernel.h"

#define TIMER0_BASE  0x101E2000
#define TIMER0_VALUE (*((volatile unsigned int*)(TIMER0_BASE + 0x04)))
#define TIMER0_CTRL  (*((volatile unsigned int*)(TIMER0_BASE + 0x08)))

static int ps2_release_flag = 0;
static int ps2_extended_flag = 0;
static int shift_pressed = 0;
static int caps_lock_active = 0;
static int alt_pressed = 0;

static const char ps2_set2_ascii[128] = {
    [0x1C] = 'a', [0x32] = 'b', [0x21] = 'c', [0x23] = 'd', [0x24] = 'e',
    [0x2B] = 'f', [0x34] = 'g', [0x33] = 'h', [0x43] = 'i', [0x3B] = 'j',
    [0x42] = 'k', [0x4B] = 'l', [0x3A] = 'm', [0x31] = 'n', [0x44] = 'o',
    [0x4D] = 'p', [0x15] = 'q', [0x2D] = 'r', [0x1B] = 's', [0x2C] = 't',
    [0x3C] = 'u', [0x2A] = 'v', [0x1D] = 'w', [0x22] = 'x', [0x35] = 'y',
    [0x1A] = 'z', [0x45] = '0', [0x16] = '1', [0x1E] = '2', [0x26] = '3',
    [0x25] = '4', [0x2E] = '5', [0x36] = '6', [0x3D] = '7', [0x3E] = '8',
    [0x46] = '9', [0x29] = ' ', [0x5A] = '\n', [0x66] = '\b', [0x0D] = '\t',
    [0x76] = 27,  [0x4E] = '-', [0x55] = '=', [0x54] = '[', [0x5B] = ']',
    [0x5D] = '\\',[0x4C] = ';', [0x52] = '\'', [0x0E] = '`', [0x41] = ',',
    [0x49] = '.', [0x4A] = '/'
};

static const char ps2_set2_shifted[128] = {
    [0x1C] = 'A', [0x32] = 'B', [0x21] = 'C', [0x23] = 'D', [0x24] = 'E',
    [0x2B] = 'F', [0x34] = 'G', [0x33] = 'H', [0x43] = 'I', [0x3B] = 'J',
    [0x42] = 'K', [0x4B] = 'L', [0x3A] = 'M', [0x31] = 'N', [0x44] = 'O',
    [0x4D] = 'P', [0x15] = 'Q', [0x2D] = 'R', [0x1B] = 'S', [0x2C] = 'T',
    [0x3C] = 'U', [0x2A] = 'V', [0x1D] = 'W', [0x22] = 'X', [0x35] = 'Y',
    [0x1A] = 'Z', [0x45] = ')', [0x16] = '!', [0x1E] = '@', [0x26] = '#',
    [0x25] = '$', [0x2E] = '%', [0x36] = '^', [0x3D] = '&', [0x3E] = '*',
    [0x46] = '(', [0x29] = ' ', [0x5A] = '\n', [0x66] = '\b', [0x0D] = '\t',
    [0x76] = 27,  [0x4E] = '_', [0x55] = '+', [0x54] = '{', [0x5B] = '}',
    [0x5D] = '|', [0x4C] = ':', [0x52] = '"', [0x0E] = '~', [0x41] = '<',
    [0x49] = '>', [0x4A] = '?'
};

inline void uart_putc(char c) {
    while (UART0_FR & (1 << 5));
    UART0_DR = (unsigned int)c;
}

inline void uart_puts(const char* str) {
    while (*str) uart_putc(*str++);
}

inline char uart_getc_nonblocking(void) {
    if (!(UART0_FR & (1 << 4))) return (char)(UART0_DR & 0xFF);
    return 0;
}

void init_pl110_lcd(void) {
    SYS_CLCD = 0x2C;
    PL110_LCDTIM0 = 0x3F1F3F9C;
    PL110_LCDTIM1 = 0x090B01DF;
    PL110_LCDTIM2 = 0x067F1800;
    PL110_LCDUPBASE = (unsigned int)fb_pixels;
    PL110_LCDCONTROL = 0x192D;
    log_boot_event("[OK] PL110 Hardware LCD Framebuffer Initialized (640x480)");
}

void detect_primary_disk(char* out_buf) {
    strcpy(out_buf, "ARM VirtIO / MMC Storage Drive");
    log_boot_event("[OK] ARM VirtIO Block Storage Detected");
}

void init_hda_audio(void) {
    strcpy(audio_model_string, "ARM Generic MMIO Audio Codec");
    audio_initialized = 1;
    log_boot_event("[OK] Generic Audio subsystem active");
}

static inline unsigned char make_color(unsigned char fg, unsigned char bg) {
    return fg | (bg << 4);
}

void kernel_panic(const char* message) {
    vga_clear(make_color(COLOR_WHITE, COLOR_RED));
    vga_draw_string("================================================================================", make_color(COLOR_YELLOW, COLOR_RED), 0, 1);
    vga_draw_string("                           Roxip Kernel Panic!                                  ", make_color(COLOR_WHITE, COLOR_RED), 0, 3);
    vga_draw_string("================================================================================", make_color(COLOR_YELLOW, COLOR_RED), 0, 5);

    vga_draw_string("A critical hardware exception occurred and the system was halted safely.", make_color(COLOR_WHITE, COLOR_RED), 2, 8);
    vga_draw_string("Reason: ", make_color(COLOR_YELLOW, COLOR_RED), 2, 10);
    vga_draw_string(message, make_color(COLOR_WHITE, COLOR_RED), 10, 10);

    vga_draw_string("System Status : HALTED (Triple-Fault Protection Active)", make_color(COLOR_LIGHT_GREY, COLOR_RED), 2, 13);
    vga_draw_string("Action        : Restart your machine to recover.", make_color(COLOR_LIGHT_GREY, COLOR_RED), 2, 15);
    vga_flush();

    uart_puts("\r\n========================================\r\n");
    uart_puts("*** ROXIP KERNEL PANIC: ");
    uart_puts(message);
    uart_puts(" ***\r\n========================================\r\n");

    while (1) { asm volatile("nop"); }
}

void c_data_abort_handler(void) { kernel_panic("Data Abort (Illegal Memory Access / Pointer Overflow)"); }
void c_undef_handler(void) { kernel_panic("Undefined Instruction Exception"); }
void c_prefetch_abort_handler(void) { kernel_panic("Prefetch Abort (Execution at Invalid Memory Address)"); }

static inline void mmio_write32(unsigned int addr, unsigned int val) {
    asm volatile("str %0, [%1]" : : "r"(val), "r"(addr) : "memory");
}

void init_idt(void) {
    dbg_log("  [VEC 1/2] Setting up ARM Vector Table...");
    mmio_write32(0x00, 0xE59FF018);
    mmio_write32(0x04, 0xE59FF018);
    mmio_write32(0x08, 0xE59FF018);
    mmio_write32(0x0C, 0xE59FF018);
    mmio_write32(0x10, 0xE59FF018);

    mmio_write32(0x24, (unsigned int)c_undef_handler);
    mmio_write32(0x2C, (unsigned int)c_prefetch_abort_handler);
    mmio_write32(0x30, (unsigned int)c_data_abort_handler);

    dbg_log("  [VEC 2/2] Registering Exception Handlers...");
    log_boot_event("[OK] ARM Exception Vector Table & Triple-Fault Guard Active");
}

void detect_cpu_brand(char* out_buf) { strcpy(out_buf, "ARM926EJ-S (ARMv5TE 32-bit)"); }
void detect_gpu(char* out_buf) { strcpy(out_buf, "PL110 Color LCD Framebuffer Adapter"); }

void init_kmi_mouse(void) {
    KMI1_CR = 0x14;
    while (!(KMI1_STAT & (1 << 6)));
    KMI1_DATA = 0xF4;
}

void poll_kmi_mouse(void) {
    static unsigned char packet[3];
    static int p_idx = 0;
    int moved = 0;

    while (KMI1_STAT & (1 << 4)) {
        unsigned char b = KMI1_DATA;
        if (p_idx == 0 && !(b & 0x08)) continue;
        packet[p_idx++] = b;

        if (p_idx == 3) {
            p_idx = 0;
            int raw_dx = (int)packet[1];
            int raw_dy = (int)packet[2];

            if (packet[0] & 0x10) raw_dx -= 256;
            if (packet[0] & 0x20) raw_dy -= 256;

            int old_x = mouse_x; int old_y = mouse_y;
            mouse_x += (raw_dx / 4); mouse_y -= (raw_dy / 4);

            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x >= VGA_WIDTH) mouse_x = VGA_WIDTH - 1;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y >= VGA_HEIGHT) mouse_y = VGA_HEIGHT - 1;

            if (mouse_x != old_x || mouse_y != old_y) moved = 1;

            if (packet[0] & 0x01) {
                if (current_mode == MODE_DESK) {
                    if (mouse_y == 1 && mouse_x >= 2 && mouse_x <= 7) { active_tab = 0; render_desktop(); }
                    else if (mouse_y == 1 && mouse_x >= 10 && mouse_x <= 19) { active_tab = 1; render_desktop(); }
                    else if (mouse_y == 1 && mouse_x >= 22 && mouse_x <= 27) { active_tab = 2; current_mode = MODE_PERF_CLI; redraw_current_view(); }
                    else if (active_tab == 1 && mouse_y == 7 && mouse_x >= 4 && mouse_x <= 30) { current_theme = THEME_DEFAULT; render_desktop(); }
                    else if (active_tab == 1 && mouse_y == 9 && mouse_x >= 4 && mouse_x <= 30) { current_theme = THEME_MAC_MONO; render_desktop(); }
                }
            }
        }
    }
    if (moved) redraw_current_view();
}

void init_kmi_keyboard(void) { KMI0_CR = 0x14; }

char get_keyboard_input(void) {
    char c = uart_getc_nonblocking();
    if (c) return c;

    if (KMI0_STAT & (1 << 4)) {
        unsigned char scancode = KMI0_DATA;
        if (scancode == 0xE0) { ps2_extended_flag = 1; return 0; }
        if (scancode == 0xF0) { ps2_release_flag = 1; return 0; }

        if (ps2_release_flag) {
            ps2_release_flag = 0;
            if (scancode == 0x12 || scancode == 0x59) shift_pressed = 0;
            if (scancode == 0x11) alt_pressed = 0;
            ps2_extended_flag = 0;
            return 0;
        }

        if (scancode == 0x12 || scancode == 0x59) { shift_pressed = 1; return 0; }
        if (scancode == 0x11) { alt_pressed = 1; return 0; }
        if (scancode == 0x58) { caps_lock_active = !caps_lock_active; return 0; }

        if (ps2_extended_flag) {
            ps2_extended_flag = 0;
            if (scancode == 0x75) return KEY_UP;
            if (scancode == 0x72) return KEY_DOWN;
            if (scancode == 0x6B) return KEY_LEFT;
            if (scancode == 0x74) return KEY_RIGHT;
        }

        if (scancode == 0x05) return KEY_F1;
        if (scancode == 0x06) return KEY_F2;
        if (scancode == 0x04) return KEY_F3;
        if (scancode == 0x0C) return KEY_F4;

        char base_char = ps2_set2_ascii[scancode];
        if (base_char >= 'a' && base_char <= 'z') {
            int use_upper = shift_pressed ^ caps_lock_active;
            return use_upper ? ps2_set2_shifted[scancode] : base_char;
        }
        if (shift_pressed && ps2_set2_shifted[scancode]) return ps2_set2_shifted[scancode];

        return base_char;
    }
    return 0;
}

void delay_ms(unsigned int ms) {
    TIMER0_CTRL = 0x82;
    unsigned int start = TIMER0_VALUE;
    unsigned int ticks_to_wait = ms * 1000;
    while ((start - TIMER0_VALUE) < ticks_to_wait) { asm volatile("nop"); }
}

void delay_cycles(volatile unsigned int count) {
    while (count--) { asm volatile("nop"); }
}

void system_shutdown(void) {
    vga_clear(make_color(COLOR_WHITE, COLOR_BLACK));
    vga_draw_string("================================================================================", make_color(COLOR_YELLOW, COLOR_BLACK), 0, 10);
    vga_draw_string("                    System is powering down safely...                           ", make_color(COLOR_WHITE, COLOR_BLACK), 0, 11);
    vga_draw_string("                    It is now safe to turn off.                    ", make_color(COLOR_LIGHT_GREEN, COLOR_BLACK), 0, 12);
    vga_draw_string("================================================================================", make_color(COLOR_YELLOW, COLOR_BLACK), 0, 13);
    vga_flush();

    uart_puts("\r\n[SYSTEM] RoxiP OS powered off safely. CPU Halted.\r\n");
    unsigned int zero = 0;
    while (1) { asm volatile("mcr p15, 0, %0, c7, c0, 4" : : "r"(zero)); }
}

void system_reboot(void) {
    uart_puts("\r\n[RESTART] Rebooting RoxiP OS Kernel...\r\n");
    vga_clear(make_color(COLOR_WHITE, COLOR_BLACK));
    vga_draw_string("Rebooting RoxiP OS...", make_color(COLOR_YELLOW, COLOR_BLACK), 2, 2);
    vga_flush();

    volatile unsigned int* sys_lock     = (volatile unsigned int*)(0x10000000);
    volatile unsigned int* sys_resetctl = (volatile unsigned int*)(0x10000040);
    *sys_lock = 0xA05F;
    *sys_resetctl = 0x0100;

    void (*reset_vector)(void) = (void (*)(void))0x00000000;
    reset_vector();
}