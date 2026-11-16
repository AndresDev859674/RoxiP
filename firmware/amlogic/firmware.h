#ifndef AMLOGIC_MESON_FIRMWARE_H
#define AMLOGIC_MESON_FIRMWARE_H

/* --------------------------------------------------------------------------
 * Amlogic Meson SoC MMIO addresses (G12A / G12B / GXL / GXM family)
 * e.g. S905X2, S905Y2 (G12A)  |  S905X3, S905D3, A311D (G12B)
 *
 * NOTE: Exact offsets vary between Meson generations.  These are the
 * commonly-documented G12A/G12B addresses; adjust per board.
 * -------------------------------------------------------------------------- */

/* Always-On UART (AO UART) - serial console */
#define UART0_BASE  0xFF803000
#define UART0_DR    (*((volatile unsigned int*)(UART0_BASE + 0x00)))
#define UART0_FR    (*((volatile unsigned int*)(UART0_BASE + 0x18)))

/* HDMI TX controller (video output) */
#define HDMI_TX_BASE   0xFF200000
#define HDMI_TX_CTRL   (*((volatile unsigned int*)(HDMI_TX_BASE + 0x00)))
#define HDMI_TX_STATUS (*((volatile unsigned int*)(HDMI_TX_BASE + 0x04)))
#define HDMI_PHY_BASE  0xFF201000

/* HIU (clock & reset control) */
#define HHI_BASE  0xFF620000
#define HHI_CNTL  (*((volatile unsigned int*)(HHI_BASE + 0x00)))

/* Timer */
#define TIMER0_BASE  0xFFD30000
#define TIMER0_VALUE (*((volatile unsigned int*)(TIMER0_BASE + 0x04)))
#define TIMER0_CTRL  (*((volatile unsigned int*)(TIMER0_BASE + 0x08)))

/* eMMC / SD controller (dwmc - DesignWare Multimedia Card) */
#define MMC_BASE     0xFFD40000
#define MMC_CMD      (*((volatile unsigned int*)(MMC_BASE + 0x00)))
#define MMC_ARG      (*((volatile unsigned int*)(MMC_BASE + 0x04)))
#define MMC_RESP0    (*((volatile unsigned int*)(MMC_BASE + 0x08)))
#define MMC_STATUS   (*((volatile unsigned int*)(MMC_BASE + 0x0C)))
#define MMC_DATA     (*((volatile unsigned int*)(MMC_BASE + 0x10)))

/* Audio codec (Amlogic) */
#define AUDIO_BASE   0xFF804000
#define AUDIO_CTRL   (*((volatile unsigned int*)(AUDIO_BASE + 0x00)))
#define AUDIO_DATA   (*((volatile unsigned int*)(AUDIO_BASE + 0x04)))

/* Compatibility aliases used by drivers/core */
/* (Meson has no KMI controller -> KMI0_BASE undefined -> driver stubs) */

#endif /* AMLOGIC_MESON_FIRMWARE_H */
