#include "kernel.h"
#include "drivers.h"
#include "firmware.h"

/* ----------------------------------------------------------------------
 * UART driver — supports both PL011 (ARM boards) and 16550 (RISC-V virt)
 * ---------------------------------------------------------------------- */

#ifdef UART_THR
/* RISC-V virt: 16550-compatible UART at 0x10000000 */

inline void uart_putc(char c) {
    while (!(UART_LSR & 0x20)) { }  /* Wait for THRE (Transmit Holding Register Empty) */
    UART_THR = (unsigned char)c;
}

inline void uart_puts(const char* str) {
    while (*str) uart_putc(*str++);
}

inline char uart_getc_nonblocking(void) {
    if (!(UART_LSR & 0x01)) return 0;  /* DR (Data Ready) bit */
    return (char)(UART_RBR & 0xFF);
}

#else
/* ARM boards: PL011 UART at UART0_BASE */

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

#endif /* UART_THR */
