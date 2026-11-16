#include "kernel.h"
#include "drivers.h"
#include "firmware.h"

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

