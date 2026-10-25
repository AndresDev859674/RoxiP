#include "kernel.h"
#include "hal.h"

#define TIMER0_BASE  0x101E2000
#define TIMER0_VALUE (*((volatile unsigned int*)(TIMER0_BASE + 0x04)))
#define TIMER0_CTRL  (*((volatile unsigned int*)(TIMER0_BASE + 0x08)))

static inline void mmio_write32(unsigned int addr, unsigned int val) {
    asm volatile("str %0, [%1]" : : "r"(val), "r"(addr) : "memory");
}

void board_detect_cpu(char* out_buf) {
    strcpy(out_buf, "ARM926EJ-S (ARMv5TE 32-bit)");
}

void board_detect_gpu(char* out_buf) {
    strcpy(out_buf, "PL110 Color LCD Framebuffer Adapter");
}

void board_delay_ms(unsigned int ms) {
    TIMER0_CTRL = 0x82;
    unsigned int start = TIMER0_VALUE;
    unsigned int ticks_to_wait = ms * 1000;
    while ((start - TIMER0_VALUE) < ticks_to_wait) { asm volatile("nop"); }
}

void board_delay_cycles(volatile unsigned int count) {
    while (count--) { asm volatile("nop"); }
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

void board_init(void) {
    init_idt();
    log_boot_event("[OK] VersatilePB Board Hardware Initialized");
}