#include "kernel.h"
#include "hal.h"
#include "drivers.h"
#include "firmware.h"

static inline unsigned char make_color(unsigned char fg, unsigned char bg) {
    return fg | (bg << 4);
}

/* ----------------------------------------------------------------------
 * RISC-V CSR helpers
 * ---------------------------------------------------------------------- */
static inline unsigned long csr_read(unsigned long csr) {
    unsigned long val;
    asm volatile("csrr %0, %1" : "=r"(val) : "i"(csr));
    return val;
}

static inline void csr_write(unsigned long csr, unsigned long val) {
    asm volatile("csrw %0, %1" : : "i"(csr), "r"(val));
}

static inline void csr_set(unsigned long csr, unsigned long val) {
    asm volatile("csrs %0, %1" : : "i"(csr), "r"(val));
}

/* CSR numbers */
#define CSR_MSTATUS 0x300
#define CSR_MTVEC   0x301
#define CSR_MSCRATCH 0x340
#define CSR_MEPC    0x341
#define CSR_MCAUSE  0x342

/* ----------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */
void board_uart_init(void);
char board_uart_getc_nonblocking(void);
void board_uart_putc(char c);
void board_uart_puts(const char* str);

/* ----------------------------------------------------------------------
 * Kernel panic (UART-only for RISC-V virt - no VGA)
 * ---------------------------------------------------------------------- */
void kernel_panic(const char* message) {
    uart_puts("\r\n========================================\r\n");
    uart_puts("*** ROXIP KERNEL PANIC (RISC-V) ***\r\n");
    uart_puts("Reason: ");
    uart_puts(message);
    uart_puts("\r\n========================================\r\n");
    uart_puts("System Status : HALTED\r\n");
    uart_puts("Action        : Restart your machine to recover.\r\n");

    while (1) { asm volatile("wfi"); }
}

/* ----------------------------------------------------------------------
 * Exception / trap handler entry (assembly)
 * ---------------------------------------------------------------------- */
extern void trap_handler_entry(void);

void c_trap_handler(unsigned long mcause, unsigned long mepc) {
    const char* reason = "Unknown";
    switch (mcause) {
        case 0:  reason = "Instruction address misaligned"; break;
        case 1:  reason = "Instruction access fault"; break;
        case 2:  reason = "Illegal instruction"; break;
        case 3:  reason = "Breakpoint"; break;
        case 4:  reason = "Load address misaligned"; break;
        case 5:  reason = "Load access fault"; break;
        case 6:  reason = "Store/AMO address misaligned"; break;
        case 7:  reason = "Store/AMO access fault"; break;
        case 8:  reason = "Environment call from U-mode"; break;
        case 9:  reason = "Environment call from S-mode"; break;
        case 11: reason = "Environment call from M-mode"; break;
        case 12: reason = "Instruction page fault"; break;
        case 13: reason = "Load page fault"; break;
        case 15: reason = "Store/AMO page fault"; break;
        default: break;
    }

    char hex_buf[12];
    uart_puts("\r\n[TRAP] mcause=");
    hex_to_str((unsigned int)mcause, hex_buf);
    uart_puts(hex_buf);
    uart_puts(" mepc=");
    hex_to_str((unsigned int)mepc, hex_buf);
    uart_puts(hex_buf);
    uart_puts(" reason=");
    uart_puts((char*)reason);
    uart_puts("\r\n");

    kernel_panic("RISC-V Trap/Exception");
}

/* ----------------------------------------------------------------------
 * Board init
 * ---------------------------------------------------------------------- */
void board_init(void) {
    /* Set machine trap vector to our assembly entry */
    extern void trap_handler_entry(void);
    csr_write(CSR_MTVEC, (unsigned long)trap_handler_entry);

    /* Enable machine-mode interrupts */
    csr_set(CSR_MSTATUS, 0x8);  /* MIE bit */

    /* Initialize UART (16550) */
    board_uart_init();

    log_boot_event("[OK] RISC-V Virt Hardware Platform Active");
}

/* ----------------------------------------------------------------------
 * CPU / GPU detection
 * ---------------------------------------------------------------------- */
void board_detect_cpu(char* out_buf) {
    strcpy(out_buf, "RISC-V rv64gc (QEMU virt)");
}

void board_detect_gpu(char* out_buf) {
    strcpy(out_buf, "None (serial console only)");
}

/* ----------------------------------------------------------------------
 * Delays
 * ---------------------------------------------------------------------- */
void board_delay_ms(unsigned int ms) {
    /* Busy-wait using mtime (CLINT timer) */
    unsigned long start = CLINT_MTIME;
    unsigned long ticks_to_wait = (unsigned long)ms * 10000; /* approx */
    while ((CLINT_MTIME - start) < ticks_to_wait) { asm volatile("nop"); }
}

void board_delay_cycles(volatile unsigned int count) {
    while (count--) { asm volatile("nop"); }
}

/* ----------------------------------------------------------------------
 * HAL Compatibility Wrappers for kernel.c
 * ---------------------------------------------------------------------- */
void detect_cpu_brand(char* out_buf) { board_detect_cpu(out_buf); }
void detect_gpu(char* out_buf) { board_detect_gpu(out_buf); }
void delay_ms(unsigned int ms) { board_delay_ms(ms); }
void delay_cycles(volatile unsigned int count) { board_delay_cycles(count); }

/* ----------------------------------------------------------------------
 * System control
 * ---------------------------------------------------------------------- */
void system_shutdown(void) {
    uart_puts("\r\n[SYSTEM] RoxiP (RISC-V) powered off safely. CPU Halted.\r\n");
    while (1) { asm volatile("wfi"); }
}

void system_reboot(void) {
    uart_puts("\r\n[RESTART] Rebooting RoxiP (RISC-V)...\r\n");
    /* QEMU virt doesn't have a reboot register; just jump to reset */
    void (*reset_vector)(void) = (void (*)(void))0x1000;
    reset_vector();
}

/* ----------------------------------------------------------------------
 * UART 16550 driver (board-specific, for RISC-V virt)
 * ---------------------------------------------------------------------- */

static inline void uart_lcr_set_dlab(int set) {
    unsigned char lcr = UART_LCR;
    if (set) UART_LCR = lcr | 0x80;
    else     UART_LCR = lcr & ~0x80;
}

void board_uart_init(void) {
    /* 115200 baud @ 1.8432 MHz clock: divisor = 1 */
    uart_lcr_set_dlab(1);
    UART_THR = 0x00;  /* DLL low byte = 0x00 (divisor = 1) */
    UART_IER = 0x01;  /* DLM high byte = 0x01 (divisor = 256 for 115200 @ 1.8432MHz? Actually let's just use 8N1) */
    uart_lcr_set_dlab(0);
    UART_LCR = 0x03;  /* 8 data bits, no parity, 1 stop bit */
    UART_IER = 0x00;  /* No interrupts (polling) */
    log_boot_event("[OK] RISC-V Virt UART (16550) Initialized");
}

char board_uart_getc_nonblocking(void) {
    if (!(UART_LSR & 0x01)) return 0;  /* DR bit */
    return (char)(UART_RBR & 0xFF);
}

void board_uart_putc(char c) {
    while (!(UART_LSR & 0x20)) { }  /* Wait for THRE */
    UART_THR = (unsigned char)c;
}

void board_uart_puts(const char* str) {
    while (*str) board_uart_putc(*str++);
}
