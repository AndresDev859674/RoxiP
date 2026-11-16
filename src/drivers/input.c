#include "kernel.h"
#include "drivers.h"
#include "firmware.h"

#ifdef KMI0_BASE
/* ----------------------------------------------------------------------
 * PS/2 scan-code tables (KMI protocol, VersatilePB only)
 * ---------------------------------------------------------------------- */
static const char ps2_set2_ascii[128] = {
    [0x1C] = 'a', [0x32] = 'b', [0x21] = 'c', [0x23] = 'd', [0x24] = 'e',
    [0x2B] = 'f', [0x34] = 'g', [0x33] = 'h', [0x43] = 'i', [0x3B] = 'j',
    [0x42] = 'k', [0x4B] = 'l', [0x3A] = 'm', [0x31] = 'n', [0x44] = 'o',
    [0x4D] = 'p', [0x15] = 'q', [0x2D] = 'r', [0x1B] = 's', [0x2C] = 't',
    [0x3C] = 'u', [0x2A] = 'v', [0x1D] = 'w', [0x22] = 'x', [0x35] = 'y',
    [0x1A] = 'z', [0x45] = '0', [0x16] = '1', [0x1E] = '2', [0x26] = '3',
    [0x25] = '4', [0x2E] = '5', [0x36] = '6', [0x3D] = '7', [0x3E] = '8',
    [0x46] = '9', [0x29] = ' ', [0x5A] = '\n', [0x66] = '\b', [0x0D] = '\t',
    [0x76] = 27,  [0x4E] = '-', [0x55] = '=', [0x54] = '[', [0x5B] = ']',
    [0x5D] = '\\',[0x4C] = ';', [0x52] = '\'', [0x0E] = '`', [0x41] = ',',
    [0x49] = '.', [0x4A] = '/'
};

static const char ps2_set2_shifted[128] = {
    [0x1C] = 'A', [0x32] = 'B', [0x21] = 'C', [0x23] = 'D', [0x24] = 'E',
    [0x2B] = 'F', [0x34] = 'G', [0x33] = 'H', [0x43] = 'I', [0x3B] = 'J',
    [0x42] = 'K', [0x4B] = 'L', [0x3A] = 'M', [0x31] = 'N', [0x44] = 'O',
    [0x4D] = 'P', [0x15] = 'Q', [0x2D] = 'R', [0x1B] = 'S', [0x2C] = 'T',
    [0x3C] = 'U', [0x2A] = 'V', [0x1D] = 'W', [0x22] = 'X', [0x35] = 'Y',
    [0x1A] = 'Z', [0x45] = ')', [0x16] = '!', [0x1E] = '@', [0x26] = '#',
    [0x25] = '$', [0x2E] = '%', [0x36] = '^', [0x3D] = '&', [0x3E] = '*',
    [0x46] = '(', [0x29] = ' ', [0x5A] = '\n', [0x66] = '\b', [0x0D] = '\t',
    [0x76] = 27,  [0x4E] = '_', [0x55] = '+', [0x54] = '{', [0x5B] = '}',
    [0x5D] = '|', [0x4C] = ':', [0x52] = '"', [0x0E] = '~', [0x41] = '<',
    [0x49] = '>', [0x4A] = '?'
};

static int ps2_release_flag = 0;
static int ps2_extended_flag = 0;
static int shift_pressed = 0;
static int caps_lock_active = 0;
static int alt_pressed = 0;

void init_kmi_mouse(void) {
    KMI1_CR = 0x14;
    while (!(KMI1_STAT & (1 << 6)));
    KMI1_DATA = 0xF4;
}

void poll_kmi_mouse(void) {
    static unsigned char packet[3];
    static int p_idx = 0;
    int moved = 0;

    while (KMI1_STAT & (1 << 4)) {
        unsigned char b = KMI1_DATA;
        if (p_idx == 0 && !(b & 0x08)) continue;
        packet[p_idx++] = b;

        if (p_idx == 3) {
            p_idx = 0;
            int raw_dx = (int)packet[1];
            int raw_dy = (int)packet[2];

            if (packet[0] & 0x10) raw_dx -= 256;
            if (packet[0] & 0x20) raw_dy -= 256;

            int old_x = mouse_x; int old_y = mouse_y;
            mouse_x += (raw_dx / 4); mouse_y -= (raw_dy / 4);

            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x >= VGA_WIDTH) mouse_x = VGA_WIDTH - 1;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y >= VGA_HEIGHT) mouse_y = VGA_HEIGHT - 1;

            if (mouse_x != old_x || mouse_y != old_y) moved = 1;

            if (packet[0] & 0x01) {
                if (current_mode == MODE_DESK) {
                    if (mouse_y == 1 && mouse_x >= 2 && mouse_x <= 7) { active_tab = 0; render_desktop(); }
                    else if (mouse_y == 1 && mouse_x >= 10 && mouse_x <= 19) { active_tab = 1; render_desktop(); }
                    else if (mouse_y == 1 && mouse_x >= 22 && mouse_x <= 27) { active_tab = 2; current_mode = MODE_PERF_CLI; redraw_current_view(); }
                    else if (active_tab == 1 && mouse_y == 7 && mouse_x >= 4 && mouse_x <= 30) { current_theme = THEME_DEFAULT; render_desktop(); }
                    else if (active_tab == 1 && mouse_y == 9 && mouse_x >= 4 && mouse_x <= 30) { current_theme = THEME_MAC_MONO; render_desktop(); }
                }
            }
        }
    }
    if (moved) redraw_current_view();
}

void init_kmi_keyboard(void) { KMI0_CR = 0x14; }
#else /* !KMI0_BASE */
/* Stubs for boards without a KMI controller (Rockchip, Amlogic, etc.) */
void init_kmi_mouse(void)    { (void)0; }
void poll_kmi_mouse(void)    { (void)0; }
void init_kmi_keyboard(void) { (void)0; }
#endif /* KMI0_BASE */

/* ----------------------------------------------------------------------
 * Keyboard input: UART first (works on every board), then KMI (if present)
 * ---------------------------------------------------------------------- */
char get_keyboard_input(void) {
    char c = uart_getc_nonblocking();
    if (c) return c;

#ifdef KMI0_BASE
    if (KMI0_STAT & (1 << 4)) {
        unsigned char scancode = KMI0_DATA;
        if (scancode == 0xE0) { ps2_extended_flag = 1; return 0; }
        if (scancode == 0xF0) { ps2_release_flag = 1; return 0; }

        if (ps2_release_flag) {
            ps2_release_flag = 0;
            if (scancode == 0x12 || scancode == 0x59) shift_pressed = 0;
            if (scancode == 0x11) alt_pressed = 0;
            ps2_extended_flag = 0;
            return 0;
        }

        if (scancode == 0x12 || scancode == 0x59) { shift_pressed = 1; return 0; }
        if (scancode == 0x11) { alt_pressed = 1; return 0; }
        if (scancode == 0x58) { caps_lock_active = !caps_lock_active; return 0; }

        if (ps2_extended_flag) {
            ps2_extended_flag = 0;
            if (scancode == 0x75) return KEY_UP;
            if (scancode == 0x72) return KEY_DOWN;
            if (scancode == 0x6B) return KEY_LEFT;
            if (scancode == 0x74) return KEY_RIGHT;
        }

        if (scancode == 0x05) return KEY_F1;
        if (scancode == 0x06) return KEY_F2;
        if (scancode == 0x04) return KEY_F3;
        if (scancode == 0x0C) return KEY_F4;

        char base_char = ps2_set2_ascii[scancode];
        if (base_char >= 'a' && base_char <= 'z') {
            int use_upper = shift_pressed ^ caps_lock_active;
            return use_upper ? ps2_set2_shifted[scancode] : base_char;
        }
        if (shift_pressed && ps2_set2_shifted[scancode]) return ps2_set2_shifted[scancode];

        return base_char;
    }
#endif /* KMI0_BASE */

    return 0;
}
