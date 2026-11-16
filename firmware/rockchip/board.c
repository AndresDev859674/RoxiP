#include "kernel.h"
#include "hal.h"
#include "drivers.h"
#include "firmware.h"

static inline unsigned char make_color(unsigned char fg, unsigned char bg) {
    return fg | (bg << 4);
}

static inline void mmio_write32(unsigned int addr, unsigned int val) {
    asm volatile("str %0, [%1]" : : "r"(val), "r"(addr) : "memory");
}

static inline unsigned int mmio_read32(unsigned int addr) {
    unsigned int val;
    asm volatile("ldr %0, [%1]" : "=r"(val) : "r"(addr) : "memory");
    return val;
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

void board_detect_cpu(char* out_buf) {
    strcpy(out_buf, "Rockchip ARM Cortex-A Series (32-bit)");
}

void board_detect_gpu(char* out_buf) {
    strcpy(out_buf, "Mali Framebuffer / Direct MMIO Display");
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

/* HAL Compatibility Wrappers for kernel.c */
void detect_cpu_brand(char* out_buf) { board_detect_cpu(out_buf); }
void detect_gpu(char* out_buf) { board_detect_gpu(out_buf); }
void delay_ms(unsigned int ms) { board_delay_ms(ms); }
void delay_cycles(volatile unsigned int count) { board_delay_cycles(count); }

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
    uart_puts("\r\n[RESTART] Rebooting Rockchip Board...\r\n");
    vga_clear(make_color(COLOR_WHITE, COLOR_BLACK));
    vga_draw_string("Rebooting Rockchip SoC...", make_color(COLOR_YELLOW, COLOR_BLACK), 2, 2);
    vga_flush();

    /* Trigger Soft Reset via CRU register */
    volatile unsigned int* cru_softreset = (volatile unsigned int*)(RK_CRU_BASE + RK_CRU_SOFTRST_CON);
    *cru_softreset = 0xFFFF;

    void (*reset_vector)(void) = (void (*)(void))0x00000000;
    reset_vector();
}

void board_init(void) {
    init_idt();
    log_boot_event("[OK] Rockchip Hardware Platform Driver Active");
}
