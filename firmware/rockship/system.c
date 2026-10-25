#include "kernel.h"
#include "hal.h"
#include "drivers.h"
#include "firmware.h"

static inline unsigned char make_color(unsigned char fg, unsigned char bg) {
    return fg | (bg << 4);
}

void board_detect_cpu(char* out_buf) {
    strcpy(out_buf, "Rockchip ARM Cortex-A Series (32-bit)");
}

void board_detect_gpu(char* out_buf) {
    strcpy(out_buf, "Mali Framebuffer / Direct MMIO Display");
}

void board_delay_ms(unsigned int ms) {
    /* Busy-wait calibration loop for Rockchip clock speed */
    volatile unsigned int count = ms * 20000;
    while (count--) {
        asm volatile("nop");
    }
}

void board_delay_cycles(volatile unsigned int count) {
    while (count--) {
        asm volatile("nop");
    }
}

/* HAL Compatibility Wrappers for kernel.c */
void detect_cpu_brand(char* out_buf) { board_detect_cpu(out_buf); }
void detect_gpu(char* out_buf) { board_detect_gpu(out_buf); }
void delay_ms(unsigned int ms) { board_delay_ms(ms); }
void delay_cycles(volatile unsigned int count) { board_delay_cycles(count); }

void system_shutdown(void) {
    vga_clear(make_color(COLOR_WHITE, COLOR_BLACK));
    vga_draw_string("================================================================================", make_color(COLOR_YELLOW, COLOR_BLACK), 0, 10);
    vga_draw_string("              Rockchip Platform powering down safely...                          ", make_color(COLOR_WHITE, COLOR_BLACK), 0, 11);
    vga_draw_string("                    It is now safe to turn off.                    ", make_color(COLOR_LIGHT_GREEN, COLOR_BLACK), 0, 12);
    vga_draw_string("================================================================================", make_color(COLOR_YELLOW, COLOR_BLACK), 0, 13);
    vga_flush();

    uart_puts("\r\n[SYSTEM] Rockchip SoC powered off safely. CPU Halted.\r\n");
    while (1) { asm volatile("wfe"); }
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
    log_boot_event("[OK] Rockchip Hardware Platform Driver Active");
}