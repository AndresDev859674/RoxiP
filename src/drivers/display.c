#include "kernel.h"
#include "drivers.h"

void init_pl110_lcd(void) {
    SYS_CLCD = 0x2C;
    PL110_LCDTIM0 = 0x3F1F3F9C;
    PL110_LCDTIM1 = 0x090B01DF;
    PL110_LCDTIM2 = 0x067F1800;
    PL110_LCDUPBASE = (unsigned int)fb_pixels;
    PL110_LCDCONTROL = 0x192D;
    log_boot_event("[OK] PL110 Hardware LCD Framebuffer Initialized (640x480)");
}