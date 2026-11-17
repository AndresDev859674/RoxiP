#ifndef RISCV_VIRT_FIRMWARE_H
#define RISCV_VIRT_FIRMWARE_H

/* --------------------------------------------------------------------------
 * RISC-V QEMU virt board MMIO addresses
 * Target: riscv64-unknown-elf (rv64gc, lp64 ABI)
 * QEMU: qemu-system-riscv64 -machine virt
 * -------------------------------------------------------------------------- */

/* 16550-compatible UART (serial console) */
#define UART0_BASE  0x10000000
#define UART_THR    (*((volatile unsigned char*)(UART0_BASE + 0x00))) /* write */
#define UART_RBR    (*((volatile unsigned char*)(UART0_BASE + 0x00))) /* read  */
#define UART_IER    (*((volatile unsigned char*)(UART0_BASE + 0x01)))
#define UART_IIR    (*((volatile unsigned char*)(UART0_BASE + 0x02))) /* read  */
#define UART_LCR    (*((volatile unsigned char*)(UART0_BASE + 0x03)))
#define UART_MCR    (*((volatile unsigned char*)(UART0_BASE + 0x04)))
#define UART_LSR    (*((volatile unsigned char*)(UART0_BASE + 0x05)))
#define UART_MSR    (*((volatile unsigned char*)(UART0_BASE + 0x06)))
#define UART_SCR    (*((volatile unsigned char*)(UART0_BASE + 0x07)))

/* CLINT (Core Local Interruptor) - timer */
#define CLINT_BASE  0x02000000
#define CLINT_MTIMECMP(hart) (*((volatile unsigned long*)(CLINT_BASE + 0x4000 + (hart) * 8)))
#define CLINT_MTIME     (*((volatile unsigned long*)(CLINT_BASE + 0xbff8)))

/* Compatibility aliases used by drivers/core */
/* No KMI controller on virt -> stubs in drivers.c */
/* No PL110 on virt -> stubs in display.c */

#endif /* RISCV_VIRT_FIRMWARE_H */
