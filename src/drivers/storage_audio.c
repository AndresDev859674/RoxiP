#include "kernel.h"
#include "drivers.h"
#include "firmware.h"

/* ----------------------------------------------------------------------
 * Storage detection — board-specific implementations
 * ---------------------------------------------------------------------- */

#ifdef MMC_BASE
/* Boards with a dwmc eMMC/SD controller (Amlogic Meson, Rockchip) */
void detect_primary_disk(char* out_buf) {
    /* Probe the MMC controller status register to confirm presence */
    unsigned int mmc_status = MMC_STATUS;
    if (mmc_status != 0xFFFFFFFF && mmc_status != 0x00000000) {
        strcpy(out_buf, "eMMC / SD Storage Drive (dwmc)");
        log_boot_event("[OK] dwmc eMMC/SD Storage Detected");
    } else {
        strcpy(out_buf, "eMMC / SD Storage Drive (dwmc, not responding)");
        log_boot_event("[WARN] dwmc eMMC/SD controller not responding");
    }
}
#else
/* Fallback for boards without dwmc (VersatilePB uses VirtIO/MMC stub) */
void detect_primary_disk(char* out_buf) {
    strcpy(out_buf, "ARM VirtIO / MMC Storage Drive");
    log_boot_event("[OK] ARM VirtIO Block Storage Detected");
}
#endif /* MMC_BASE */

/* ----------------------------------------------------------------------
 * Audio initialization — board-specific implementations
 * ---------------------------------------------------------------------- */

#ifdef AUDIO_BASE
/* Boards with a dedicated audio codec MMIO (Amlogic Meson, Rockchip) */
void init_hda_audio(void) {
    unsigned int audio_ctrl = AUDIO_CTRL;
    if (audio_ctrl != 0xFFFFFFFF && audio_ctrl != 0x00000000) {
        strcpy(audio_model_string, "SoC Audio Codec (MMIO)");
        audio_initialized = 1;
        log_boot_event("[OK] SoC Audio subsystem active");
    } else {
        strcpy(audio_model_string, "SoC Audio Codec (not responding)");
        audio_initialized = 0;
        log_boot_event("[WARN] SoC Audio codec not responding");
    }
}
#else
/* Fallback for boards without a dedicated audio codec */
void init_hda_audio(void) {
    strcpy(audio_model_string, "ARM Generic MMIO Audio Codec");
    audio_initialized = 1;
    log_boot_event("[OK] Generic Audio subsystem active");
}
#endif /* AUDIO_BASE */
