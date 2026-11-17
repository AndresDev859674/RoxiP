#include "kernel.h"
#include "drivers.h"
#include "firmware.h"

/* ----------------------------------------------------------------------
 * Amlogic Meson HDMI TX driver — DVI mode first
 *
 * DVI is simpler than HDMI: no InfoFrames, no audio, no EDID handling.
 * We just configure video timing, power up the PHY, and enable output.
 * ---------------------------------------------------------------------- */

#ifdef HDMI_TX_ENABLE

/* Video timing for 640x480 @ 60Hz (VESA DMT) */
#define DVI_640x480_H_ACTIVE   640
#define DVI_640x480_H_FRONT    16
#define DVI_640x480_H_SYNC     96
#define DVI_640x480_H_BACK     48
#define DVI_640x480_V_ACTIVE   480
#define DVI_640x480_V_FRONT    10
#define DVI_640x480_V_SYNC     2
#define DVI_640x480_V_BACK     33

static void hdmi_phy_power_on(void) {
    /* Power up the HDMI PHY */
    HDMI_PHY_POWER = HDMI_PHY_POWER_ON;
    HDMI_PHY_CTRL0 = 0x00000001;  /* PHY enable */
    HDMI_PHY_CTRL1 = 0x00000000;  /* Default drive strength */
}

static void hdmi_set_timing_640x480(void) {
    /* Horizontal timing */
    HDMI_TX_H_START = DVI_640x480_H_BACK;              /* back porch start */
    HDMI_TX_H_END   = DVI_640x480_H_BACK + DVI_640x480_H_ACTIVE;
    HDMI_TX_HSYNC_W = DVI_640x480_H_SYNC;

    /* Vertical timing */
    HDMI_TX_V_START = DVI_640x480_V_BACK;
    HDMI_TX_V_END   = DVI_640x480_V_BACK + DVI_640x480_V_ACTIVE;
    HDMI_TX_VSYNC_W = DVI_640x480_V_SYNC;
}

void init_hdmi_dvi(void) {
    /* 1. Enable HDMI clock via HIU */
    HHI_CNTL |= (1 << 0);  /* HDMI clock enable (board-specific bit) */

    /* 2. Power up HDMI PHY */
    hdmi_phy_power_on();

    /* 3. Configure video timing (640x480 @ 60Hz) */
    hdmi_set_timing_640x480();

    /* 4. Set DVI mode (no InfoFrames, no audio) */
    HDMI_TX_MODE = HDMI_MODE_DVI;

    /* 5. Enable HDMI TX output */
    HDMI_TX_ENABLE = 0x00000001;

    log_boot_event("[OK] Amlogic HDMI TX DVI mode enabled (640x480@60Hz)");
}

#endif /* HDMI_TX_ENABLE */

/* ----------------------------------------------------------------------
 * Stub for boards without HDMI TX
 * ---------------------------------------------------------------------- */

#ifndef HDMI_TX_ENABLE
void init_hdmi_dvi(void) {
    log_boot_event("[OK] Display initialized (no HDMI TX on this board)");
}
#endif
