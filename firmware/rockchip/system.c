#include "kernel.h"
#include "hal.h"
#include "drivers.h"
#include "firmware.h"

/* system.c is intentionally minimal for Rockchip.
 * All board initialization, exception vectors, delays, shutdown/reboot,
 * and CPU/GPU detection live in board.c.  This file is kept for symmetry
 * with the VersatilePB layout (board.c + system.c per firmware dir). */
