#ifndef VERSATILEPB_FIRMWARE_H
#define VERSATILEPB_FIRMWARE_H

/* --------------------------------------------------------------------------
 * VersatilePB (ARM926EJ-S / ARMv5TE) SoC MMIO addresses
 * -------------------------------------------------------------------------- */

/* UART0 (PL011) */
#define UART0_BASE 0x101f1000
#define UART0_DR   (*((volatile unsigned int*)(UART0_BASE + 0x00)))
#define UART0_FR   (*((volatile unsigned int*)(UART0_BASE + 0x18)))

/* KMI0 (keyboard) / KMI1 (mouse) */
#define KMI0_BASE 0x10006000
#define KMI0_CR   (*((volatile unsigned char*)(KMI0_BASE + 0x00)))
#define KMI0_STAT (*((volatile unsigned char*)(KMI0_BASE + 0x04)))
#define KMI0_DATA (*((volatile unsigned char*)(KMI0_BASE + 0x08)))

#define KMI1_BASE 0x10007000
#define KMI1_CR   (*((volatile unsigned char*)(KMI1_BASE + 0x00)))
#define KMI1_STAT (*((volatile unsigned char*)(KMI1_BASE + 0x04)))
#define KMI1_DATA (*((volatile unsigned char*)(KMI1_BASE + 0x08)))

/* System control (CLCD enable / reset) */
#define SYS_CLCD  (*((volatile unsigned int*)(0x1000001C)))

/* PL110 Color LCD controller */
#define PL110_LCD_BASE    0x10120000
#define PL110_LCDTIM0     (*((volatile unsigned int*)(PL110_LCD_BASE + 0x00)))
#define PL110_LCDTIM1     (*((volatile unsigned int*)(PL110_LCD_BASE + 0x04)))
#define PL110_LCDTIM2     (*((volatile unsigned int*)(PL110_LCD_BASE + 0x08)))
#define PL110_LCDTIM3     (*((volatile unsigned int*)(PL110_LCD_BASE + 0x0C)))
#define PL110_LCDUPBASE   (*((volatile unsigned int*)(PL110_LCD_BASE + 0x10)))
#define PL110_LCDLPBASE   (*((volatile unsigned int*)(PL110_LCD_BASE + 0x14)))
#define PL110_LCDCONTROL  (*((volatile unsigned int*)(PL110_LCD_BASE + 0x18)))

/* Timer 0 (SP804) */
#define TIMER0_BASE  0x101E2000
#define TIMER0_LOAD  (*((volatile unsigned int*)(TIMER0_BASE + 0x00)))
#define TIMER0_VALUE (*((volatile unsigned int*)(TIMER0_BASE + 0x04)))
#define TIMER0_CTRL  (*((volatile unsigned int*)(TIMER0_BASE + 0x08)))

/* System registers (VersatilePB system lock / reset control) */
#define SYS_LOCK      0x10000000
#define SYS_RESETCTL  0x10000040

#endif /* VERSATILEPB_FIRMWARE_H */
