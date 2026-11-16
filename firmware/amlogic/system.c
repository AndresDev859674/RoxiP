#include "kernel.h"
#include "hal.h"
#include "drivers.h"
#include "firmware.h"

/* system.c is intentionally minimal for Amlogic Meson.
 * All board initialization, exception vectors, delays, shutdown/reboot,
 * and CPU/GPU detection live in board.c. */
