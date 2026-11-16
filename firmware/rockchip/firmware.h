#ifndef ROCKCHIP_FIRMWARE_H
#define ROCKCHIP_FIRMWARE_H

/* --------------------------------------------------------------------------
 * Rockchip SoC MMIO addresses (RK3288 / RK3328 class)
 * -------------------------------------------------------------------------- */

/* UART0 */
#define RK_UART0_BASE       0xFF130000
#define UART0_DR            (*((volatile unsigned int*)(RK_UART0_BASE + 0x00)))
#define UART0_FR            (*((volatile unsigned int*)(RK_UART0_BASE + 0x18)))

/* Timer 0 */
#define RK_TIMER0_BASE      0xFF430000
#define TIMER0_LOAD_COUNT   (*((volatile unsigned int*)(RK_TIMER0_BASE + 0x00)))
#define TIMER0_CUR_VALUE    (*((volatile unsigned int*)(RK_TIMER0_BASE + 0x04)))
#define TIMER0_CTRL         (*((volatile unsigned int*)(RK_TIMER0_BASE + 0x10)))

/* Clock & Reset Unit (CRU) */
#define RK_CRU_BASE         0xFF440000
#define RK_CRU_SOFTRST_CON  0x0300

/* eMMC / SD controller (dwmc) */
#define MMC_BASE     0xFF2C0000
#define MMC_CMD      (*((volatile unsigned int*)(MMC_BASE + 0x00)))
#define MMC_ARG      (*((volatile unsigned int*)(MMC_BASE + 0x04)))
#define MMC_RESP0    (*((volatile unsigned int*)(MMC_BASE + 0x08)))
#define MMC_STATUS   (*((volatile unsigned int*)(MMC_BASE + 0x0C)))
#define MMC_DATA     (*((volatile unsigned int*)(MMC_BASE + 0x10)))

/* Audio codec (Rockchip) */
#define AUDIO_BASE   0xFF1A0000
#define AUDIO_CTRL   (*((volatile unsigned int*)(AUDIO_BASE + 0x00)))
#define AUDIO_DATA   (*((volatile unsigned int*)(AUDIO_BASE + 0x04)))

/* Compatibility names used by drivers/core */
#define UART0_BASE  RK_UART0_BASE
#define TIMER0_BASE RK_TIMER0_BASE
#define TIMER0_VALUE TIMER0_CUR_VALUE

#endif /* ROCKCHIP_FIRMWARE_H */
