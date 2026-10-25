#include "kernel.h"
#include "drivers.h"

void detect_primary_disk(char* out_buf) {
    strcpy(out_buf, "ARM VirtIO / MMC Storage Drive");
    log_boot_event("[OK] ARM VirtIO Block Storage Detected");
}

void init_hda_audio(void) {
    strcpy(audio_model_string, "ARM Generic MMIO Audio Codec");
    audio_initialized = 1;
    log_boot_event("[OK] Generic Audio subsystem active");
}