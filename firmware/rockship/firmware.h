#ifndef ROCKCHIP_FIRMWARE_H
#define ROCKCHIP_FIRMWARE_H

/* Base MMIO Addresses for Rockchip SoC Architecture */
#define RK_UART0_BASE       0xFF130000
#define RK_TIMER0_BASE      0xFF430000
#define RK_CRU_BASE         0xFF440000

/* Timer Register Offsets */
#define RK_TIMER_LOAD_COUNT 0x00
#define RK_TIMER_CUR_VALUE  0x04
#define RK_TIMER_CTRL       0x10

/* Clock & Reset Unit (CRU) Soft Reset Registers */
#define RK_CRU_SOFTRST_CON  0x0300

#endif /* ROCKCHIP_FIRMWARE_H */