#include "kernel.h"
#include "hal.h"
#include "drivers.h"
#include "firmware.h"

/* system.c is intentionally minimal for RISC-V virt.
 * All board initialization, trap handling, delays, shutdown/reboot,
 * and CPU/GPU detection live in board.c. */
