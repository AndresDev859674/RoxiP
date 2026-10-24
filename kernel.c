/*
 * RoxiP Kernel Core, A bare-metal operating-system (i hate my own code)
*/

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* VersatilePB MMIO Base Addresses */
#define UART0_BASE 0x101f1000
#define UART0_DR   (*((volatile unsigned int*)(UART0_BASE + 0x00)))
#define UART0_FR   (*((volatile unsigned int*)(UART0_BASE + 0x18)))

#define KMI1_BASE 0x10007000
#define KMI1_CR   (*((volatile unsigned char*)(KMI1_BASE + 0x00)))
#define KMI1_STAT (*((volatile unsigned char*)(KMI1_BASE + 0x04)))
#define KMI1_DATA (*((volatile unsigned char*)(KMI1_BASE + 0x08)))

/* VersatilePB Syscon Register (LCD Controller Unlock) */
#define SYS_CLCD         (*((volatile unsigned int*)(0x1000001C)))

/* PL050 KMI Keyboard MMIO Base for VersatilePB */
#define KMI0_BASE 0x10006000
#define KMI0_CR   (*((volatile unsigned char*)(KMI0_BASE + 0x00)))
#define KMI0_STAT (*((volatile unsigned char*)(KMI0_BASE + 0x04)))
#define KMI0_DATA (*((volatile unsigned char*)(KMI0_BASE + 0x08)))

/* Correct PL110 MMIO Offsets */
#define PL110_LCDTIM0    (*((volatile unsigned int*)(0x10120000)))
#define PL110_LCDTIM1    (*((volatile unsigned int*)(0x10120004)))
#define PL110_LCDTIM2    (*((volatile unsigned int*)(0x10120008)))
#define PL110_LCDTIM3    (*((volatile unsigned int*)(0x1012000C)))
#define PL110_LCDUPBASE  (*((volatile unsigned int*)(0x10120010)))
#define PL110_LCDLPBASE  (*((volatile unsigned int*)(0x10120014)))
#define PL110_LCDCONTROL (*((volatile unsigned int*)(0x10120018))) /* FIX: 0x18 en vez de 0x1C */

#define FB_WIDTH  640
#define FB_HEIGHT 480

/* VGA Color Palette Constants */
#define COLOR_BLACK         0x0
#define COLOR_BLUE          0x1
#define COLOR_GREEN         0x2
#define COLOR_CYAN          0x3
#define COLOR_RED           0x4
#define COLOR_MAGENTA       0x5
#define COLOR_BROWN         0x6
#define COLOR_LIGHT_GREY    0x7
#define COLOR_DARK_GREY     0x8
#define COLOR_LIGHT_BLUE    0x9
#define COLOR_LIGHT_GREEN   0xA
#define COLOR_LIGHT_CYAN    0xB
#define COLOR_LIGHT_RED     0xC
#define COLOR_LIGHT_MAGENTA 0xD
#define COLOR_YELLOW        0xE
#define COLOR_WHITE         0xF

/* CP437 Box & Texture Characters */
#define BOX_SINGLE_TL 218
#define BOX_SINGLE_TR 191
#define BOX_SINGLE_BL 192
#define BOX_SINGLE_BR 217
#define BOX_SINGLE_H  196
#define BOX_SINGLE_V  179

#define BOX_DOUBLE_TL 201
#define BOX_DOUBLE_TR 187
#define BOX_DOUBLE_BL 200
#define BOX_DOUBLE_BR 188
#define BOX_DOUBLE_H  205
#define BOX_DOUBLE_V  186
#define CURSOR_CHAR   219
#define MAC_PATTERN   176

#define TERM_ROWS 18
#define TERM_COLS 74
#define MAX_BOOT_LOGS 12
#define MAX_VFS_FILES 16
#define MAX_FILENAME_LEN 64
#define MAX_FILE_CONTENT 65536
#define MAX_ENV_VARS     16
#define MAX_VFS_FILES    16
#define MAX_ENV_KEY 32
#define MAX_ENV_VAL 64
#define MAX_CMD_HIST 16

/* Dynamic Heap Memory Arena mapped at RAM Safe Region */
#define HEAP_SIZE (512 * 1024) /* 512 KB Heap Arena */

/* Global Current Working Directory state */
static char current_dir[MAX_FILENAME_LEN] = "/";

typedef enum {
    MODE_PERF_CLI,
    MODE_DESK,
    MODE_DESK_WINDOW,
    MODE_FLOATCUBE,
    MODE_MICRO_EDITOR,
    MODE_ZTOP
} system_mode_t;

typedef enum {
    THEME_DEFAULT = 0,
    THEME_MAC_MONO = 1
} desk_theme_t;

/* comcow Display Compositor Structure */
typedef struct {
    char mode_name[32];
    int width;
    int height;
    int refresh_rate;
    int double_buffering;
    int latency_ms;
    int active_mode_id;
} comcow_compositor_t;

/* Memory Allocator Structures */
typedef struct block_header {
    unsigned int size;
    int is_free;
    struct block_header* next;
} block_header_t;

/* Software RGB Graphics Structures */
typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} rgb_color_t;

typedef struct {
    int width;
    int height;
    rgb_color_t* pixels;
} rgb_framebuffer_t;

/* Process Structure for ztop */
typedef struct {
    int pid;
    const char* user;
    int cpu_usage;
    unsigned int mem_kb;
    const char* state;
    const char* name;
} process_info_t;

/* Internal File Systems Data */
typedef struct {
    char name[MAX_FILENAME_LEN];
    char content[MAX_FILE_CONTENT];
    int size;
    int used;
    int is_vfat;
    int is_dir;
} file_record_t;

/* Environment Variables Structure */
typedef struct {
    char key[MAX_ENV_KEY];
    char value[MAX_ENV_VAL];
    int used;
} env_var_t;

/* ARM Vector Table Structure */
struct arm_vector_table {
    unsigned int reset;
    unsigned int undef_instruction;
    unsigned int software_interrupt;
    unsigned int prefetch_abort;
    unsigned int data_abort;
    unsigned int reserved;
    unsigned int irq;
    unsigned int fiq;
};

extern void dummy_isr_wrapper(void);
extern char __bss_end[];

/* Hardware Framebuffer mapped to PL110 LCD */
static unsigned short fb_pixels[FB_WIDTH * FB_HEIGHT] __attribute__((aligned(16)));

/* Virtual Text Buffer Memory pointers */
static unsigned short vga_virtual_buffer[VGA_WIDTH * VGA_HEIGHT];
static unsigned short vga_backbuffer[VGA_WIDTH * VGA_HEIGHT];

/* comcow Compositor Instance */
static comcow_compositor_t comcow;

static system_mode_t current_mode = MODE_PERF_CLI;
static desk_theme_t current_theme = THEME_DEFAULT;

static char input_buffer[128];
static int input_pos = 0;

/* Special Key Identifiers */
#define KEY_F1     0x81
#define KEY_F2     0x82
#define KEY_F3     0x83
#define KEY_F4     0x84
#define KEY_UP     0x88
#define KEY_DOWN   0x89
#define KEY_LEFT   0x8A
#define KEY_RIGHT  0x8B

/* Command History Storage */
static char cmd_history[MAX_CMD_HIST][128];
static int hist_count = 0;

/* Micro Text Editor State */
static char micro_filename[MAX_FILENAME_LEN];
static char micro_buffer[MAX_FILE_CONTENT];
static int micro_pos = 0;

/* Environment Variables Storage */
static env_var_t env_table[MAX_ENV_VARS];

/* Hardware Specs & Metrics */
static char cpu_model_string[64] = "Detecting CPU...";
static char gpu_model_string[64] = "Detecting GPU...";
static char audio_model_string[64] = "Detecting Audio Hardware...";
static char disk_model_string[64] = "Detecting Primary Disk...";
static unsigned int timer_ticks = 0;

/* Audio Global State */
static unsigned int audio_mmio_base __attribute__((unused)) = 0;
static int audio_initialized = 0;

/* System Logs Buffer */
static char boot_logs[MAX_BOOT_LOGS][74];
static int boot_log_count = 0;

/* UI State */
static int active_tab = 0;
static int hub_hidden = 0;
static int is_extended_scancode __attribute__((unused)) = 0;
static int bios_anim_frame = 0;

/* Mouse State */
static int mouse_x = 40;
static int mouse_y = 12;

/* Shared Terminal History */
static char term_lines[TERM_ROWS][TERM_COLS];
static int term_row_count = 0;

/* Storage Array */
static file_record_t fs_table[MAX_VFS_FILES];
static int fs_file_count = 0;
static int dbg_line = 0;

static int ps2_release_flag = 0;
static int ps2_extended_flag = 0;
static int shift_pressed = 0;
static int caps_lock_active = 0;
static int alt_pressed = 0;

/* Fixed-Point Trigonometry Lookup Table for FloatCube */
static const int sin_table[36] = {
    0, 22, 43, 64, 82, 98, 110, 120, 126, 128, 126, 120, 110, 98, 82, 64, 43, 22,
    0, -22, -43, -64, -82, -98, -110, -120, -126, -128, -126, -120, -110, -98, -82, -64, -43, -22
};

static const int cos_table[36] = {
    128, 126, 120, 110, 98, 82, 64, 43, 22, 0, -22, -43, -64, -82, -98, -110, -120, -126,
    -128, -126, -120, -110, -98, -82, -64, -43, -22, 0, 22, 43, 64, 82, 98, 110, 120, 126
};

/* VGA Palette to RGB565 Table */
static const unsigned short vga_to_rgb565[16] = {
    0x0000, 0x000F, 0x03E0, 0x03EF, 0x7800, 0x780F, 0x7BE0, 0xC618,
    0x7BEF, 0x381F, 0x07E0, 0x07FF, 0xF800, 0xF81F, 0xFFE0, 0xFFFF
};

/* ARM Hardware I/O Helpers */
static inline void uart_putc(char c) {
    while (UART0_FR & (1 << 5));
    UART0_DR = (unsigned int)c;
}

static inline void uart_puts(const char* str) {
    while (*str) {
        uart_putc(*str++);
    }
}

static inline char uart_getc_nonblocking(void) {
    if (!(UART0_FR & (1 << 4))) {
        return (char)(UART0_DR & 0xFF);
    }
    return 0;
}

/* Logging System Declaration */
static void log_boot_event(const char* msg);

/* UI Forward Declarations */
static void render_desktop(void);
static void redraw_current_view(void);

/* Memory Management Utilities */
void* memset(void* dest, int c, unsigned int n) {
    unsigned char* p = (unsigned char*)dest;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, unsigned int n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

/* String Utilities */
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int strncmp(const char* s1, const char* s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

static void strcpy(char* dest, const char* src) {
    int i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void int_to_str(int num, char* str) {
    int i = 0;
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    char temp[16];
    int p = 0;
    while (num > 0) {
        temp[p++] = (num % 10) + '0';
        num /= 10;
    }
    while (p > 0) {
        str[i++] = temp[--p];
    }
    str[i] = '\0';
}

static void log_boot_event(const char* msg);
static int fs_write_file(const char* filename, const char* content);
static void vfs_sync_to_storage(void);
static int vfs_load_from_storage(void);

static void save_history_to_vfs(void) {
    char blob[MAX_FILE_CONTENT];
    blob[0] = '\0';
    int pos = 0;

    for (int i = 0; i < hist_count && pos < MAX_FILE_CONTENT - 64; i++) {
        int len = strlen(cmd_history[i]);
        for (int j = 0; j < len; j++) {
            blob[pos++] = cmd_history[i][j];
        }
        blob[pos++] = '\n';
    }
    blob[pos] = '\0';
    fs_write_file("history.txt", blob);
}

static void add_to_history(const char* cmd) {
    if (cmd[0] == '\0') return;
    if (hist_count > 0 && strcmp(cmd_history[hist_count - 1], cmd) == 0) return;

    if (hist_count < MAX_CMD_HIST) {
        strcpy(cmd_history[hist_count++], cmd);
    } else {
        for (int i = 0; i < MAX_CMD_HIST - 1; i++) {
            strcpy(cmd_history[i], cmd_history[i + 1]);
        }
        strcpy(cmd_history[MAX_CMD_HIST - 1], cmd);
    }
    save_history_to_vfs();
}

static block_header_t* free_list = 0;

static void dbg_log(const char* msg) {
    uart_puts("[BOOT DBG] ");
    uart_puts(msg);
    uart_puts("\r\n");

    int offset = dbg_line * VGA_WIDTH;
    for (int i = 0; i < VGA_WIDTH; i++) {
        vga_virtual_buffer[offset + i] = (unsigned short)' ' | (COLOR_BLACK << 8);
    }
    for (int i = 0; msg[i] != '\0' && i < VGA_WIDTH; i++) {
        vga_virtual_buffer[offset + i] = (unsigned short)msg[i] | (COLOR_YELLOW << 8);
    }

    dbg_line++;
    if (dbg_line >= 25) dbg_line = 0;
}

/* PL110 LCD Controller Hardware Initialization for QEMU Display */
static void init_pl110_lcd(void) {
    SYS_CLCD = 0x2C;                           /* Unlock TFT mode */
    PL110_LCDTIM0 = 0x3F1F3F9C;                /* Horizontal timings */
    PL110_LCDTIM1 = 0x090B01DF;                /* Vertical timings */
    PL110_LCDTIM2 = 0x067F1800;                /* Clock divider */
    PL110_LCDUPBASE = (unsigned int)fb_pixels; /* Point to RAM buffer */

    /* Bit 0 (EN) | Bits 1-3 (565/555 bpp) | Bit 5 (TFT) | Bit 11 (PWR) */
    PL110_LCDCONTROL = 0x192D;                 /* Escribe en 0x10120018 */

    log_boot_event("[OK] PL110 Hardware LCD Framebuffer Initialized (640x480)");
}

static void init_heap(void) {
    dbg_log("    [HEAP 1/3] Initializing Memory Arena...");
    dbg_log("    [HEAP 2/3] Aligning heap after __bss_end...");

    unsigned int heap_start = ((unsigned int)__bss_end + 15) & ~15;

    if (heap_start < 0x00100000) {
        heap_start = 0x00100000 + 0x20000;
    }

    unsigned int safe_size = HEAP_SIZE;

    dbg_log("    [HEAP 3/3] Writing Free List Header...");
    free_list = (block_header_t*)heap_start;
    free_list->size = safe_size - sizeof(block_header_t);
    free_list->is_free = 1;
    free_list->next = 0;

    log_boot_event("[OK] Dynamic Heap allocated safely in ARM RAM");
}

static void detect_primary_disk(char* out_buf) {
    strcpy(out_buf, "ARM VirtIO / MMC Storage Drive");
    log_boot_event("[OK] ARM VirtIO Block Storage Detected");
}

static unsigned int get_used_heap_bytes(void) {
    unsigned int used = 0;
    block_header_t* curr = free_list;
    while (curr) {
        if (!curr->is_free) {
            used += curr->size + sizeof(block_header_t);
        }
        curr = curr->next;
    }
    return used;
}

void* malloc(unsigned int size) {
    if (size == 0) return 0;
    size = (size + 3) & ~3;

    block_header_t* curr = free_list;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            if (curr->size >= size + sizeof(block_header_t) + 16) {
                block_header_t* next_block = (block_header_t*)((unsigned char*)curr + sizeof(block_header_t) + size);
                next_block->size = curr->size - size - sizeof(block_header_t);
                next_block->is_free = 1;
                next_block->next = curr->next;

                curr->size = size;
                curr->next = next_block;
            }
            curr->is_free = 0;
            return (void*)((unsigned char*)curr + sizeof(block_header_t));
        }
        curr = curr->next;
    }
    return 0;
}

void free(void* ptr) {
    if (!ptr) return;
    block_header_t* block = (block_header_t*)((unsigned char*)ptr - sizeof(block_header_t));
    block->is_free = 1;

    block_header_t* curr = free_list;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += sizeof(block_header_t) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

static void init_hda_audio(void) {
    strcpy(audio_model_string, "ARM Generic MMIO Audio Codec");
    audio_initialized = 1;
    log_boot_event("[OK] Generic Audio subsystem active");
}

static unsigned char rgb_to_vga_color(rgb_color_t color) {
    int r = color.r;
    int g = color.g;
    int b = color.b;

    if (r < 64 && g < 64 && b < 64) return COLOR_BLACK;
    if (r > 192 && g > 192 && b > 192) return COLOR_WHITE;

    int max_val = r;
    if (g > max_val) {
        max_val = g;
    }
    if (b > max_val) {
        max_val = b;
    }

    if (max_val == r && g < 128 && b < 128) return (r > 160) ? COLOR_LIGHT_RED : COLOR_RED;
    if (max_val == g && r < 128 && b < 128) return (g > 160) ? COLOR_LIGHT_GREEN : COLOR_GREEN;
    if (max_val == b && r < 128 && b < 128) return (b > 160) ? COLOR_LIGHT_BLUE : COLOR_BLUE;
    if (r > 128 && g > 128 && b < 128) return (r > 180) ? COLOR_YELLOW : COLOR_BROWN;
    if (r > 128 && b > 128 && g < 128) return (r > 180) ? COLOR_LIGHT_MAGENTA : COLOR_MAGENTA;
    if (g > 128 && b > 128 && r < 128) return (g > 180) ? COLOR_LIGHT_CYAN : COLOR_CYAN;

    return (max_val > 160) ? COLOR_LIGHT_GREY : COLOR_DARK_GREY;
}

static rgb_framebuffer_t* create_rgb_framebuffer(int width, int height) {
    rgb_framebuffer_t* fb = (rgb_framebuffer_t*)malloc(sizeof(rgb_framebuffer_t));
    if (!fb) return 0;
    fb->width = width;
    fb->height = height;
    fb->pixels = (rgb_color_t*)malloc(width * height * sizeof(rgb_color_t));
    if (!fb->pixels) {
        free(fb);
        return 0;
    }
    return fb;
}

static void free_rgb_framebuffer(rgb_framebuffer_t* fb) {
    if (!fb) return;
    if (fb->pixels) free(fb->pixels);
    free(fb);
}

static void rgb_set_pixel(rgb_framebuffer_t* fb, int x, int y, rgb_color_t color) {
    if (!fb || !fb->pixels) return;
    if (x >= 0 && x < fb->width && y >= 0 && y < fb->height) {
        fb->pixels[y * fb->width + x] = color;
    }
}

static void rgb_fill_rect(rgb_framebuffer_t* fb, int rx, int ry, int rw, int rh, rgb_color_t color) {
    for (int y = ry; y < ry + rh; y++) {
        for (int x = rx; x < rx + rw; x++) {
            rgb_set_pixel(fb, x, y, color);
        }
    }
}

static void vga_draw_char_at(char c, unsigned char color, int x, int y);

static void rgb_render_to_vga(rgb_framebuffer_t* fb, int start_x, int start_y) {
    if (!fb || !fb->pixels) return;
    for (int y = 0; y < fb->height; y++) {
        for (int x = 0; x < fb->width; x++) {
            int target_x = start_x + x;
            int target_y = start_y + y;
            if (target_x < VGA_WIDTH && target_y < VGA_HEIGHT) {
                rgb_color_t col = fb->pixels[y * fb->width + x];
                unsigned char vga_col = rgb_to_vga_color(col);
                vga_draw_char_at(219, vga_col | (COLOR_BLACK << 4), target_x, target_y);
            }
        }
    }
}

static void init_env_vars(void) {
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        env_table[i].used = 0;
    }
    strcpy(env_table[0].key, "OS");
    strcpy(env_table[0].value, "RoxiP");
    env_table[0].used = 1;

    strcpy(env_table[1].key, "SHELL");
    strcpy(env_table[1].value, "perf");
    env_table[1].used = 1;
}

static int env_set(const char* key, const char* val) {
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (env_table[i].used && strcmp(env_table[i].key, key) == 0) {
            strcpy(env_table[i].value, val);
            return 1;
        }
    }
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (!env_table[i].used) {
            strcpy(env_table[i].key, key);
            strcpy(env_table[i].value, val);
            env_table[i].used = 1;
            return 1;
        }
    }
    return 0;
}

static int env_unset(const char* key) {
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (env_table[i].used && strcmp(env_table[i].key, key) == 0) {
            env_table[i].used = 0;
            return 1;
        }
    }
    return 0;
}

static void log_boot_event(const char* msg) {
    if (boot_log_count < MAX_BOOT_LOGS) {
        int i = 0;
        while (msg[i] && i < 73) {
            boot_logs[boot_log_count][i] = msg[i];
            i++;
        }
        boot_logs[boot_log_count][i] = '\0';
        boot_log_count++;
    }
}

/* VGA Forward Declarations for Exception Handlers */
static void vga_clear(unsigned char color);
static void vga_draw_string(const char* str, unsigned char color, int x, int y);
static void vga_flush(void);

static inline unsigned char make_color(unsigned char fg, unsigned char bg) {
    return fg | (bg << 4);
}

/* Unrecoverable Kernel Panic Handler */
void kernel_panic(const char* message) {
    vga_clear(make_color(COLOR_WHITE, COLOR_RED));

    vga_draw_string("================================================================================", make_color(COLOR_YELLOW, COLOR_RED), 0, 1);
    vga_draw_string("                           Roxip Kernel Panic!                                  ", make_color(COLOR_WHITE, COLOR_RED), 0, 3);
    vga_draw_string("================================================================================", make_color(COLOR_YELLOW, COLOR_RED), 0, 5);

    vga_draw_string("A critical hardware exception occurred and the system was halted safely.", make_color(COLOR_WHITE, COLOR_RED), 2, 8);
    vga_draw_string("Reason: ", make_color(COLOR_YELLOW, COLOR_RED), 2, 10);
    vga_draw_string(message, make_color(COLOR_WHITE, COLOR_RED), 10, 10);

    vga_draw_string("System Status : HALTED (Triple-Fault Protection Active)", make_color(COLOR_LIGHT_GREY, COLOR_RED), 2, 13);
    vga_draw_string("Action        : Restart your machine to recover.", make_color(COLOR_LIGHT_GREY, COLOR_RED), 2, 15);

    vga_flush();

    /* Log panic via UART Serial */
    uart_puts("\r\n========================================\r\n");
    uart_puts("*** ROXIP KERNEL PANIC: ");
    uart_puts(message);
    uart_puts(" ***\r\n========================================\r\n");

    /* Infinite loop to trap CPU and prevent Triple Fault reboot */
    while (1) {
        asm volatile("nop");
    }
}

/* C Exception Handlers for ARM Aborts */
void c_data_abort_handler(void) {
    kernel_panic("Data Abort (Illegal Memory Access / Pointer Overflow)");
}

void c_undef_handler(void) {
    kernel_panic("Undefined Instruction Exception");
}

void c_prefetch_abort_handler(void) {
    kernel_panic("Prefetch Abort (Execution at Invalid Memory Address)");
}

/* Helper to safely write to absolute physical memory addresses (bypasses GCC NULL pointer optimization) */
static inline void mmio_write32(unsigned int addr, unsigned int val) {
    asm volatile("str %0, [%1]" : : "r"(val), "r"(addr) : "memory");
}

static void init_idt(void) {
    dbg_log("  [VEC 1/2] Setting up ARM Vector Table...");

    /* Install ARM exception vectors at physical RAM base 0x00000000 safely */
    mmio_write32(0x00, 0xE59FF018); /* Reset -> LDR PC, [PC, #24] */
    mmio_write32(0x04, 0xE59FF018); /* Undefined Instruction */
    mmio_write32(0x08, 0xE59FF018); /* Software Interrupt */
    mmio_write32(0x0C, 0xE59FF018); /* Prefetch Abort */
    mmio_write32(0x10, 0xE59FF018); /* Data Abort */

    /* Hook C panic handlers into vector jump targets (+0x24, +0x2C, +0x30) */
    mmio_write32(0x24, (unsigned int)c_undef_handler);
    mmio_write32(0x2C, (unsigned int)c_prefetch_abort_handler);
    mmio_write32(0x30, (unsigned int)c_data_abort_handler);

    dbg_log("  [VEC 2/2] Registering Exception Handlers...");
    log_boot_event("[OK] ARM Exception Vector Table & Triple-Fault Guard Active");
}

static void detect_cpu_brand(char* out_buf) {
    strcpy(out_buf, "ARM926EJ-S (ARMv5TE 32-bit)");
}

static void detect_gpu(char* out_buf) {
    strcpy(out_buf, "PL110 Color LCD Framebuffer Adapter");
}

/* Persistent Flash Storage Definitions (CFI NOR Flash Protocol) */
#define STORAGE_FLASH_BASE 0x34000000
#define STORAGE_MAGIC      0x53465053  /* Unificado */

typedef struct {
    unsigned int magic;
    int file_count;
    file_record_t files[MAX_VFS_FILES];
} storage_header_t;

/* Writes a single byte using standard CFI NOR Flash protocol (QEMU pflash01) */
static void pflash_write_byte(unsigned int offset, unsigned char val) {
    volatile unsigned char* flash = (volatile unsigned char*)STORAGE_FLASH_BASE;
    flash[offset] = 0x40; /* CFI Program Command */
    flash[offset] = val;  /* Write byte data */
    flash[offset] = 0xFF; /* Reset Flash to Read Array mode */
}

static void vfs_sync_to_storage(void) {
    volatile unsigned char* flash = (volatile unsigned char*)STORAGE_FLASH_BASE;

    /* 1. Sync magic signature */
    unsigned int magic = STORAGE_MAGIC;
    unsigned char* magic_ptr = (unsigned char*)&magic;
    for (unsigned int i = 0; i < sizeof(unsigned int); i++) {
        if (flash[i] != magic_ptr[i]) {
            pflash_write_byte(i, magic_ptr[i]);
        }
    }

    /* 2. Sync file count */
    unsigned char* count_ptr = (unsigned char*)&fs_file_count;
    unsigned int count_offset = sizeof(unsigned int);
    for (unsigned int i = 0; i < sizeof(int); i++) {
        if (flash[count_offset + i] != count_ptr[i]) {
            pflash_write_byte(count_offset + i, count_ptr[i]);
        }
    }

    /* 3. Sync file records array directly from BSS table */
    unsigned char* fs_ptr = (unsigned char*)fs_table;
    unsigned int table_offset = sizeof(unsigned int) + sizeof(int);
    unsigned int table_size = sizeof(file_record_t) * MAX_VFS_FILES;

    for (unsigned int i = 0; i < table_size; i++) {
        if (flash[table_offset + i] != fs_ptr[i]) {
            pflash_write_byte(table_offset + i, fs_ptr[i]);
        }
    }
}

/* Restores RAM VFS table from roxip.storage if valid signature exists */
static int vfs_load_from_storage(void) {
    volatile storage_header_t* flash = (volatile storage_header_t*)STORAGE_FLASH_BASE;

    if (flash->magic != STORAGE_MAGIC) {
        return 0; /* Unformatted or new storage file */
    }

    fs_file_count = flash->file_count;
    for (int i = 0; i < MAX_VFS_FILES; i++) {
        fs_table[i] = flash->files[i];
    }
    return 1;
}

static void init_filesystems(void) {
    /* Check if persistent storage file contains valid RoxiP data */
    if (vfs_load_from_storage()) {
        log_boot_event("[OK] Persistent VFS mounted from roxip.storage");
        return;
    }

    /* First boot / Unformatted storage: create default VFS structure */
    fs_file_count = 0;

    strcpy(fs_table[0].name, "motd.txt");
    strcpy(fs_table[0].content, "Welcome to RoxiP OS! comcow Compositor Engine Active.");
    fs_table[0].size = strlen(fs_table[0].content);
    fs_table[0].used = 1;
    fs_table[0].is_vfat = 0;
    fs_table[0].is_dir = 0;
    fs_file_count++;

    strcpy(fs_table[1].name, "LongFileName_VFAT.sys");
    strcpy(fs_table[1].content, "vfat_enabled=true\nfat32_clusters=65536\nlfn_support=1");
    fs_table[1].size = strlen(fs_table[1].content);
    fs_table[1].used = 1;
    fs_table[1].is_vfat = 1;
    fs_table[1].is_dir = 0;
    fs_file_count++;

    strcpy(fs_table[2].name, "history.txt");
    strcpy(fs_table[2].content, "");
    fs_table[2].size = 0;
    fs_table[2].used = 1;
    fs_table[2].is_vfat = 0;
    fs_table[2].is_dir = 0;
    fs_file_count++;

    log_boot_event("[OK] Persistent Flash formatted (ROXIPFS1)");
    vfs_sync_to_storage();
}

static int fs_write_file(const char* filename, const char* content) {
    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, filename) == 0) {
            if (fs_table[i].is_dir) return 0;
            strcpy(fs_table[i].content, content);
            fs_table[i].size = strlen(content);
            vfs_sync_to_storage(); /* Sync change to roxip.storage */
            return 1;
        }
    }

    if (fs_file_count < MAX_VFS_FILES) {
        int idx = fs_file_count;
        strcpy(fs_table[idx].name, filename);
        strcpy(fs_table[idx].content, content);
        fs_table[idx].size = strlen(content);
        fs_table[idx].used = 1;
        fs_table[idx].is_vfat = (strlen(filename) > 11) ? 1 : 0;
        fs_table[idx].is_dir = 0;
        fs_file_count++;
        vfs_sync_to_storage(); /* Sync change to roxip.storage */
        return 1;
    }
    return 0;
}

static int fs_mkdir(const char* dirname) {
    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, dirname) == 0) return 0;
    }
    if (fs_file_count < MAX_VFS_FILES) {
        int idx = fs_file_count;
        strcpy(fs_table[idx].name, dirname);
        fs_table[idx].content[0] = '\0';
        fs_table[idx].size = 0;
        fs_table[idx].used = 1;
        fs_table[idx].is_vfat = 0;
        fs_table[idx].is_dir = 1;
        fs_file_count++;
        vfs_sync_to_storage(); /* Sync change to roxip.storage */
        return 1;
    }
    return 0;
}

static void init_comcow(void) {
    strcpy(comcow.mode_name, "640x480 RGB565 Hardware Framebuffer");
    comcow.width = 80;
    comcow.height = 25;
    comcow.refresh_rate = 60;
    comcow.double_buffering = 1;
    comcow.latency_ms = 1;
    comcow.active_mode_id = 1;
    log_boot_event("[OK] comcow Display Compositor Engine initialized");
}

static void comcow_set_mode(int mode_id) {
    if (mode_id == 1) {
        strcpy(comcow.mode_name, "80x25 Standard Virtual Text");
        comcow.active_mode_id = 1;
    } else if (mode_id == 2) {
        strcpy(comcow.mode_name, "80x25 High-Refresh Text");
        comcow.active_mode_id = 2;
    } else if (mode_id == 3) {
        strcpy(comcow.mode_name, "80x50 High-Density Text");
        comcow.active_mode_id = 3;
    } else if (mode_id == 4) {
        strcpy(comcow.mode_name, "320x200 Software Framebuffer");
        comcow.active_mode_id = 4;
    }
}

static inline unsigned short make_vga_entry(char c, unsigned char color) {
    return (unsigned short)c | ((unsigned short)color << 8);
}

static void set_hardware_cursor(int x, int y) {
    (void)x;
    (void)y;
}

static void init_kmi_mouse(void) {
    KMI1_CR = 0x14; /* Enable Mouse KMI controller */
    /* Send enable streaming command to mouse */
    while (!(KMI1_STAT & (1 << 6))) {
        /* Wait TXEMPTY */
    }
    KMI1_DATA = 0xF4;
}

static void handle_mouse_click(int mx, int my) {
    if (current_mode == MODE_DESK) {
        /* Tab 0: Main (x: 2-7, y: 1) */
        if (my == 1 && mx >= 2 && mx <= 7) {
            active_tab = 0;
            render_desktop();
        }
        /* Tab 1: Settings (x: 10-19, y: 1) */
        else if (my == 1 && mx >= 10 && mx <= 19) {
            active_tab = 1;
            render_desktop();
        }
        /* Tab 2: Exit (x: 22-27, y: 1) */
        else if (my == 1 && mx >= 22 && mx <= 27) {
            active_tab = 2;
            current_mode = MODE_PERF_CLI;
            redraw_current_view();
        }
        /* Seleccionar Tema 1 en Settings */
        else if (active_tab == 1 && my == 7 && mx >= 4 && mx <= 30) {
            current_theme = THEME_DEFAULT;
            render_desktop();
        }
        /* Seleccionar Tema 2 (Monocromo) en Settings */
        else if (active_tab == 1 && my == 9 && mx >= 4 && mx <= 30) {
            current_theme = THEME_MAC_MONO;
            render_desktop();
        }
    }
}

static void poll_kmi_mouse(void) {
    static unsigned char packet[3];
    static int p_idx = 0;
    int moved = 0;

    while (KMI1_STAT & (1 << 4)) { /* RXFULL */
        unsigned char b = KMI1_DATA;

        /* Sincronización PS/2: El Byte 0 SIEMPRE tiene el Bit 3 en 1 (0x08) */
        if (p_idx == 0 && !(b & 0x08)) {
            continue; /* Descartar byte desalineado */
        }

        packet[p_idx++] = b;

        if (p_idx == 3) {
            p_idx = 0;

            int raw_dx = (int)packet[1];
            int raw_dy = (int)packet[2];

            /* Extensión de signo usando los bits 4 y 5 del Byte 0 */
            if (packet[0] & 0x10) raw_dx -= 256;
            if (packet[0] & 0x20) raw_dy -= 256;

            int old_x = mouse_x;
            int old_y = mouse_y;

            mouse_x += (raw_dx / 4);
            mouse_y -= (raw_dy / 4);

            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x >= VGA_WIDTH) mouse_x = VGA_WIDTH - 1;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y >= VGA_HEIGHT) mouse_y = VGA_HEIGHT - 1;

            if (mouse_x != old_x || mouse_y != old_y) {
                moved = 1;
            }

            /* Clic Izquierdo (Bit 0 del Byte 0) */
            if (packet[0] & 0x01) {
                handle_mouse_click(mouse_x, mouse_y);
            }
        }
    }

    if (moved) {
        redraw_current_view();
    }
}

static const unsigned char font8x8_basic[256][8] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['!'] = {0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00},
    ['#'] = {0x24, 0x24, 0xff, 0x24, 0x24, 0xff, 0x24, 0x24},
    ['%'] = {0x62, 0x64, 0x08, 0x10, 0x26, 0x46, 0x00, 0x00},
    ['~'] = {0x00, 0x32, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['<'] = {0x18, 0x30, 0x60, 0x30, 0x18, 0x00, 0x00, 0x00},
    ['?'] = {0x3c, 0x66, 0x0c, 0x18, 0x18, 0x00, 0x18, 0x00},
    ['@'] = {0x3c, 0x66, 0x6e, 0x6e, 0x60, 0x3e, 0x00, 0x00},
    ['>'] = {0x60, 0x30, 0x18, 0x0c, 0x18, 0x30, 0x60, 0x00},
    ['='] = {0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00},
    ['-'] = {0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00},
    ['+'] = {0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00},
    ['|'] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
    ['/'] = {0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x00, 0x00},
    ['\\']= {0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x00, 0x00},
    [':'] = {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00},
    ['*'] = {0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00},
    ['.'] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00},
    [','] = {0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 0x00},
    ['_'] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00},
    ['['] = {0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00},
    [']'] = {0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00},
    ['('] = {0x0e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x0e, 0x00},
    [')'] = {0x70, 0x18, 0x18, 0x18, 0x18, 0x18, 0x70, 0x00},
    ['p'] = {0x00, 0x00, 0xdc, 0x66, 0x66, 0x7c, 0x60, 0xf0},
    ['e'] = {0x00, 0x00, 0x3c, 0x66, 0x7e, 0x60, 0x3c, 0x00},
    ['r'] = {0x00, 0x00, 0xdd, 0x66, 0x60, 0x60, 0xf0, 0x00},
    ['f'] = {0x1c, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x78, 0x00},
    ['a'] = {0x00, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00},
    ['b'] = {0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0xdc, 0x00},
    ['c'] = {0x00, 0x00, 0x3c, 0x66, 0x60, 0x66, 0x3c, 0x00},
    ['d'] = {0x06, 0x06, 0x3e, 0x66, 0x66, 0x66, 0x3e, 0x00},
    ['g'] = {0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x3c},
    ['h'] = {0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0xe6, 0x00},
    ['i'] = {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00},
    ['j'] = {0x0c, 0x00, 0x1c, 0x0c, 0x0c, 0x0c, 0xcc, 0x78},
    ['k'] = {0x60, 0x60, 0x66, 0x6c, 0x78, 0x6c, 0x66, 0x00},
    ['l'] = {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00},
    ['m'] = {0x00, 0x00, 0xec, 0xfe, 0xd6, 0xd6, 0xc6, 0x00},
    ['n'] = {0x00, 0x00, 0xdc, 0x66, 0x66, 0x66, 0x66, 0x00},
    ['o'] = {0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00},
    ['q'] = {0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x0f},
    ['s'] = {0x00, 0x00, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x00},
    ['t'] = {0x18, 0x18, 0x7e, 0x18, 0x18, 0x18, 0x0e, 0x00},
    ['u'] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00},
    ['v'] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00},
    ['w'] = {0x00, 0x00, 0xc6, 0xd6, 0xd6, 0xfe, 0x6c, 0x00},
    ['x'] = {0x00, 0x00, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x00},
    ['y'] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x3c},
    ['z'] = {0x00, 0x00, 0x7e, 0x0c, 0x18, 0x30, 0x7e, 0x00},
    ['A'] = {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x00},
    ['B'] = {0x7c, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x7c, 0x00},
    ['C'] = {0x3c, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3c, 0x00},
    ['D'] = {0x78, 0x6c, 0x66, 0x66, 0x66, 0x6c, 0x78, 0x00},
    ['E'] = {0x7e, 0x60, 0x60, 0x7c, 0x60, 0x60, 0x7e, 0x00},
    ['F'] = {0x7e, 0x60, 0x60, 0x7c, 0x60, 0x60, 0x60, 0x00},
    ['G'] = {0x3c, 0x66, 0x60, 0x6e, 0x66, 0x66, 0x3e, 0x00},
    ['H'] = {0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00},
    ['I'] = {0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00},
    ['J'] = {0x1e, 0x0c, 0x0c, 0x0c, 0x0c, 0xcc, 0x78, 0x00},
    ['K'] = {0x66, 0x6c, 0x78, 0x70, 0x78, 0x6c, 0x66, 0x00},
    ['L'] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00},
    ['M'] = {0x63, 0x77, 0x7f, 0x6b, 0x63, 0x63, 0x63, 0x00},
    ['N'] = {0x66, 0x76, 0x7e, 0x7e, 0x6e, 0x66, 0x66, 0x00},
    ['O'] = {0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00},
    ['P'] = {0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x60, 0x00},
    ['Q'] = {0x3c, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x0e, 0x00},
    ['R'] = {0x7c, 0x66, 0x66, 0x7c, 0x6c, 0x66, 0x63, 0x00},
    ['S'] = {0x3c, 0x66, 0x60, 0x3c, 0x06, 0x66, 0x3c, 0x00},
    ['T'] = {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    ['U'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00},
    ['V'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00},
    ['W'] = {0x63, 0x63, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00},
    ['X'] = {0x66, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x66, 0x00},
    ['Y'] = {0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00},
    ['Z'] = {0x7e, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x7e, 0x00},
    ['0'] = {0x3c, 0x66, 0x6f, 0x77, 0x66, 0x66, 0x3c, 0x00},
    ['1'] = {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00},
    ['2'] = {0x3c, 0x66, 0x06, 0x1c, 0x30, 0x60, 0x7e, 0x00},
    ['3'] = {0x3c, 0x66, 0x06, 0x1c, 0x06, 0x66, 0x3c, 0x00},
    ['4'] = {0x0c, 0x1c, 0x3c, 0x6c, 0xfe, 0x0c, 0x0c, 0x00},
    ['5'] = {0x7e, 0x60, 0x7c, 0x06, 0x06, 0x66, 0x3c, 0x00},
    ['6'] = {0x3c, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x3c, 0x00},
    ['7'] = {0x7e, 0x06, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x00},
    ['8'] = {0x3c, 0x66, 0x66, 0x3c, 0x66, 0x66, 0x3c, 0x00},
    ['9'] = {0x3c, 0x66, 0x66, 0x3e, 0x06, 0x0c, 0x38, 0x00},

    [176] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55}
};

/* Pipeline: Render 80x25 Grid EXCLUSIVELY into the QEMU Hardware Framebuffer */
/* Optimized VGA Flush: Uses front/back buffer diffing to stop QEMU CPU saturation */
static void vga_flush(void) {
    for (int ty = 0; ty < VGA_HEIGHT; ty++) {
        for (int tx = 0; tx < VGA_WIDTH; tx++) {
            int entry_idx = ty * VGA_WIDTH + tx;
            unsigned short entry = vga_backbuffer[entry_idx];

            /* Skip character rasterization if screen cell content hasn't changed */
            if (entry == vga_virtual_buffer[entry_idx]) {
                continue;
            }
            vga_virtual_buffer[entry_idx] = entry;

            unsigned char c = (unsigned char)(entry & 0xFF);

            unsigned char color_attr = (entry >> 8) & 0xFF;
            unsigned char bg_idx = (color_attr >> 4) & 0x0F;
            unsigned char fg_idx = color_attr & 0x0F;

            unsigned short bg_color = vga_to_rgb565[bg_idx];
            unsigned short fg_color = vga_to_rgb565[fg_idx];

            int start_x = tx * 8;
            int start_y = ty * 16 + 40;

            for (int py = 0; py < 16; py++) {
                unsigned char font_row = font8x8_basic[c][py / 2];
                int draw_y = start_y + py;
                if (draw_y >= FB_HEIGHT) continue;

                int row_offset = draw_y * FB_WIDTH;
                for (int px = 0; px < 8; px++) {
                    int draw_x = start_x + px;
                    if (draw_x < FB_WIDTH) {
                        int bit_set = (font_row & (1 << (7 - px))) != 0;
                        fb_pixels[row_offset + draw_x] = bit_set ? fg_color : bg_color;
                    }
                }
            }
        }
    }

    PL110_LCDCONTROL = 0x192D;
}

static void vga_clear(unsigned char color) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_backbuffer[y * VGA_WIDTH + x] = make_vga_entry(' ', color);
        }
    }
}

static void vga_force_redraw(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_virtual_buffer[i] = 0xFFFF; /* Force mismatch on next flush */
    }
}

static void vga_draw_char_at(char c, unsigned char color, int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga_backbuffer[y * VGA_WIDTH + x] = make_vga_entry(c, color);
    }
}

static void vga_draw_string(const char* str, unsigned char color, int x, int y) {
    while (*str && x < VGA_WIDTH) {
        vga_draw_char_at(*str, color, x, y);
        x++;
        str++;
    }
}

static void draw_box(int start_x, int start_y, int width, int height, unsigned char border_color, unsigned char bg_color) {
    for (int y = start_y; y < start_y + height; y++) {
        for (int x = start_x; x < start_x + width; x++) {
            vga_draw_char_at(' ', bg_color, x, y);
        }
    }
    vga_draw_char_at('+', border_color, start_x, start_y);
    vga_draw_char_at('+', border_color, start_x + width - 1, start_y);
    vga_draw_char_at('+', border_color, start_x, start_y + height - 1);
    vga_draw_char_at('+', border_color, start_x + width - 1, start_y + height - 1);

    for (int x = start_x + 1; x < start_x + width - 1; x++) {
        vga_draw_char_at('-', border_color, x, start_y);
        vga_draw_char_at('-', border_color, x, start_y + height - 1);
    }
    for (int y = start_y + 1; y < start_y + height - 1; y++) {
        vga_draw_char_at('|', border_color, start_x, y);
        vga_draw_char_at('|', border_color, start_x + width - 1, y);
    }
}

static void draw_mouse_cursor(void) {
    if (mouse_x >= 0 && mouse_x < VGA_WIDTH && mouse_y >= 0 && mouse_y < VGA_HEIGHT) {
        unsigned char c = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_WHITE, COLOR_BLACK);
        vga_draw_char_at('X', c, mouse_x, mouse_y);
    }
}

static void term_add_line_single(const char* text) {
    if (term_row_count < TERM_ROWS) {
        int i = 0;
        while (text[i] && i < TERM_COLS - 1) {
            term_lines[term_row_count][i] = text[i];
            i++;
        }
        term_lines[term_row_count][i] = '\0';
        term_row_count++;
    } else {
        for (int r = 0; r < TERM_ROWS - 1; r++) {
            for (int c = 0; c < TERM_COLS; c++) {
                term_lines[r][c] = term_lines[r + 1][c];
            }
        }
        int i = 0;
        while (text[i] && i < TERM_COLS - 1) {
            term_lines[TERM_ROWS - 1][i] = text[i];
            i++;
        }
        term_lines[TERM_ROWS - 1][i] = '\0';
    }
}

static void term_add_line(const char* text) {
    int len = strlen(text);
    int offset = 0;
    int max_chunk = TERM_COLS - 2;

    if (len == 0) {
        term_add_line_single("");
        return;
    }

    while (offset < len) {
        char chunk[TERM_COLS];
        int c_idx = 0;

        while (c_idx < max_chunk && text[offset] != '\0') {
            chunk[c_idx++] = text[offset++];
        }
        chunk[c_idx] = '\0';
        term_add_line_single(chunk);
    }
}

static void term_clear_history(void) {
    for (int r = 0; r < TERM_ROWS; r++) {
        term_lines[r][0] = '\0';
    }
    term_row_count = 0;
}

static void execute_whatweus(void) {
    term_add_line("  /\\        OS: RoxiP OS v0.1");
    term_add_line(" /  \\       Kernel: Momentanum v0.1");
    term_add_line("/ /\\ \\      Compositor: comcow Engine");

    char gpu_line[74] = "\\ \\/ /      GPU: ";
    int g_idx = 17;
    int p = 0;
    while (gpu_model_string[p] && g_idx < 72) {
        gpu_line[g_idx++] = gpu_model_string[p++];
    }
    gpu_line[g_idx] = '\0';
    term_add_line(gpu_line);

    char audio_line[74] = " \\  /       Audio: ";
    int a_idx = 19;
    p = 0;
    while (audio_model_string[p] && a_idx < 72) {
        audio_line[a_idx++] = audio_model_string[p++];
    }
    audio_line[a_idx] = '\0';
    term_add_line(audio_line);

    char cpu_line[74] = "  \\/        CPU: ";
    int c_idx = 17;
    p = 0;
    while (cpu_model_string[p] && c_idx < 72) {
        cpu_line[c_idx++] = cpu_model_string[p++];
    }
    cpu_line[c_idx] = '\0';
    term_add_line(cpu_line);

    unsigned int total_system_ram_mb = 512;
    unsigned int used_bytes = get_used_heap_bytes();
    unsigned int used_kb = used_bytes / 1024;

    char ram_str[64] = "RAM: ";
    char used_buf[16], total_buf[16];
    int_to_str(used_kb, used_buf);
    int_to_str(total_system_ram_mb, total_buf);

    int idx = 5;
    p = 0;
    while (used_buf[p]) {
        ram_str[idx++] = used_buf[p++];
    }
    ram_str[idx++] = 'K';
    ram_str[idx++] = 'B';
    ram_str[idx++] = ' ';
    ram_str[idx++] = 'U';
    ram_str[idx++] = 's';
    ram_str[idx++] = 'e';
    ram_str[idx++] = 'd';
    ram_str[idx++] = ' ';
    ram_str[idx++] = '/';
    ram_str[idx++] = ' ';
    p = 0;
    while (total_buf[p]) {
        ram_str[idx++] = total_buf[p++];
    }
    ram_str[idx++] = 'M';
    ram_str[idx++] = 'B';
    ram_str[idx++] = ' ';
    ram_str[idx++] = '(';
    ram_str[idx++] = 'A';
    ram_str[idx++] = 'R';
    ram_str[idx++] = 'M';
    ram_str[idx++] = ')';
    ram_str[idx] = '\0';

    char ram_line[74] = "            ";
    c_idx = 12;
    p = 0;
    while (ram_str[p] && c_idx < 72) {
        ram_line[c_idx++] = ram_str[p++];
    }
    ram_line[c_idx] = '\0';
    term_add_line(ram_line);
}

static void render_ztop_frame(void) {
    vga_clear(make_color(COLOR_LIGHT_GREY, COLOR_BLACK));
    unsigned char header_color = make_color(COLOR_BLACK, COLOR_CYAN);
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_draw_char_at(' ', header_color, x, 0);
    }
    vga_draw_string(" RoxiP ztop v0.1 - a minimal system monitor ", header_color, 0, 0);

    unsigned char bar_label = make_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    unsigned char bar_fill  = make_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    unsigned char bar_empty = make_color(COLOR_DARK_GREY, COLOR_BLACK);

    int cpu_percent = 4 + (timer_ticks % 7);
    vga_draw_string("CPU  [", bar_label, 2, 2);
    int cpu_bars = (cpu_percent * 25) / 100;
    for (int i = 0; i < 25; i++) {
        if (i < cpu_bars) vga_draw_char_at('|', bar_fill, 8 + i, 2);
        else vga_draw_char_at('.', bar_empty, 8 + i, 2);
    }
    vga_draw_char_at(']', bar_label, 33, 2);

    char cpu_str[16];
    int_to_str(cpu_percent, cpu_str);
    vga_draw_string(cpu_str, make_color(COLOR_YELLOW, COLOR_BLACK), 35, 2);
    vga_draw_string("%", make_color(COLOR_YELLOW, COLOR_BLACK), 35 + strlen(cpu_str), 2);

    unsigned int used_kb = get_used_heap_bytes() / 1024;
    unsigned int total_kb = HEAP_SIZE / 1024;
    int mem_percent = (used_kb * 100) / total_kb;
    if (mem_percent == 0) mem_percent = 1;

    vga_draw_string("Mem  [", bar_label, 2, 3);
    int mem_bars = (mem_percent * 25) / 100;
    for (int i = 0; i < 25; i++) {
        if (i <= mem_bars) vga_draw_char_at('|', bar_fill, 8 + i, 3);
        else vga_draw_char_at('.', bar_empty, 8 + i, 3);
    }
    vga_draw_char_at(']', bar_label, 33, 3);

    char used_str[16], total_str[16];
    int_to_str(used_kb, used_str);
    int_to_str(total_kb, total_str);
    vga_draw_string(used_str, make_color(COLOR_YELLOW, COLOR_BLACK), 35, 3);
    vga_draw_string("KB/", make_color(COLOR_YELLOW, COLOR_BLACK), 35 + strlen(used_str), 3);
    vga_draw_string(total_str, make_color(COLOR_YELLOW, COLOR_BLACK), 38 + strlen(used_str), 3);
    vga_draw_string("KB", make_color(COLOR_YELLOW, COLOR_BLACK), 38 + strlen(used_str) + strlen(total_str), 3);

    vga_draw_string("Tasks: 6 total, 6 running", make_color(COLOR_WHITE, COLOR_BLACK), 45, 2);
    vga_draw_string("comcow: 60Hz PL110 Framebuffer", make_color(COLOR_WHITE, COLOR_BLACK), 45, 3);

    unsigned char th_color = make_color(COLOR_BLACK, COLOR_LIGHT_GREY);
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_draw_char_at(' ', th_color, x, 5);
    }
    vga_draw_string("  PID  USER       CPU%   MEM (KB)   STATE     COMMAND", th_color, 0, 5);

    process_info_t procs[6] = {
        { 1, "root", cpu_percent - 2, 128, "RUNNING", "arm_kernel" },
        { 2, "root", 1, 64,  "RUNNING", "comcow_wm" },
        { 3, "user", 2, 32,  "RUNNING", "perf_shell" },
        { 4, "root", 1, 16,  "RUNNING", "pl011_serial" },
        { 5, "root", 0, 48,  "SLEEP",   "vfs_engine" },
        { 6, "root", 0, 32,  "RUNNING", "arm_audio" }
    };

    for (int i = 0; i < 6; i++) {
        int y = 6 + i;
        unsigned char proc_color = (i % 2 == 0) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
        char pid_s[8], cpu_s[8], mem_s[8];
        int_to_str(procs[i].pid, pid_s);
        int_to_str(procs[i].cpu_usage, cpu_s);
        int_to_str(procs[i].mem_kb, mem_s);
        vga_draw_string(pid_s, proc_color, 4, y);
        vga_draw_string(procs[i].user, proc_color, 9, y);
        vga_draw_string(cpu_s, proc_color, 20, y);
        vga_draw_string("%", proc_color, 20 + strlen(cpu_s), y);
        vga_draw_string(mem_s, proc_color, 29, y);
        vga_draw_string(procs[i].state, proc_color, 40, y);
        vga_draw_string(procs[i].name, proc_color, 50, y);
    }

    unsigned char footer_color = make_color(COLOR_BLACK, COLOR_CYAN);
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_draw_char_at(' ', footer_color, x, 24);
    }
    vga_draw_string(" Press [Q] or [ESC] to exit ztop and return to perf Terminal ", footer_color, 1, 24);

    draw_mouse_cursor();
    set_hardware_cursor(79, 24);
    vga_flush();
}

static void render_micro_editor(void) {
    vga_clear(make_color(COLOR_BLACK, COLOR_BLUE));
    unsigned char top_color = make_color(COLOR_BLACK, COLOR_LIGHT_GREY);
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_draw_char_at(' ', top_color, x, 0);
    }
    vga_draw_string(" micro - ", top_color, 1, 0);
    vga_draw_string(micro_filename[0] ? micro_filename : "Untitled", top_color, 10, 0);

    unsigned char bot_color = make_color(COLOR_BLACK, COLOR_CYAN);
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_draw_char_at(' ', bot_color, x, 24);
    }
    vga_draw_string(" Ctrl+S: Save  |  Ctrl+X: Exit ", bot_color, 1, 24);

    unsigned char text_color = make_color(COLOR_WHITE, COLOR_BLUE);
    int cur_x = 2, cur_y = 2;

    for (int i = 0; i < micro_pos; i++) {
        if (micro_buffer[i] == '\n') {
            cur_y++;
            cur_x = 2;
        } else {
            vga_draw_char_at(micro_buffer[i], text_color, cur_x, cur_y);
            cur_x++;
            if (cur_x >= VGA_WIDTH - 2) {
                cur_y++;
                cur_x = 2;
            }
        }
    }
    set_hardware_cursor(cur_x, cur_y);
    vga_flush();
}

static void open_micro_editor(const char* filename) {
    strcpy(micro_filename, filename);
    micro_pos = 0;
    micro_buffer[0] = '\0';
    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, filename) == 0 && !fs_table[i].is_dir) {
            strcpy(micro_buffer, fs_table[i].content);
            micro_pos = strlen(micro_buffer);
            break;
        }
    }
    current_mode = MODE_MICRO_EDITOR;
    render_micro_editor();
}

/* CPU delay helper to prevent QEMU thread saturation */
static void delay_cycles(volatile unsigned int count) {
    while (count--) {
        asm volatile("nop");
    }
}

static void draw_line_3d(int x0, int y0, int x1, int y1, char ch, unsigned char color) {
    /* Clamp projected coordinates inside screen bounds */
    if (x0 < 0) x0 = 0;
    if (x0 >= VGA_WIDTH) x0 = VGA_WIDTH - 1;
    if (x1 < 0) x1 = 0;
    if (x1 >= VGA_WIDTH) x1 = VGA_WIDTH - 1;
    if (y0 < 0) y0 = 0;
    if (y0 >= VGA_HEIGHT) y0 = VGA_HEIGHT - 1;
    if (y1 < 0) y1 = 0;
    if (y1 >= VGA_HEIGHT) y1 = VGA_HEIGHT - 1;

    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    int max_steps = 100; /* Safety iteration guard against system freeze */
    while (max_steps-- > 0) {
        vga_draw_char_at(ch, color, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

/* Pre-flight diagnostic helper to verify environment before running 3D engine */
static int validate_3d_pipeline(void) {
    /* 1. Validate Trigonometric Lookup Table integrity */
    if (cos_table[0] != 128 || sin_table[0] != 0 || sin_table[9] != 128) {
        dbg_log("3D Engine Error: Trigonometric table corrupted");
        return 0;
    }

    /* 2. Verify Heap memory arena state */
    if (free_list == 0) {
        dbg_log("3D Engine Error: Heap memory uninitialized");
        return 0;
    }

    return 1;
}

static void render_floatcube_frame(void) {
    static int angle_x = 0;
    static int angle_y = 0;
    static int angle_z = 0;
    static unsigned int frame_counter = 0;

    frame_counter++;
    angle_x = (angle_x + 1) % 36;
    angle_y = (angle_y + 2) % 36;
    angle_z = (angle_z + 1) % 36;

    vga_clear(make_color(COLOR_BLACK, COLOR_BLACK));

    vga_draw_string("=== RoxiP 3D Software Pipeline (ARM Fixed-Point) ===", make_color(COLOR_YELLOW, COLOR_BLACK), 14, 1);
    vga_draw_string("Press [ESC] or [Q] to return to perf Terminal", make_color(COLOR_DARK_GREY, COLOR_BLACK), 18, 2);

    int verts[8][3] = {
        {-18, -18, -18}, { 18, -18, -18}, { 18,  18, -18}, {-18,  18, -18},
        {-18, -18,  18}, { 18, -18,  18}, { 18,  18,  18}, {-18,  18,  18}
    };

    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };

    int proj_x[8], proj_y[8];
    unsigned char rgb_colors[6] = {
        COLOR_LIGHT_RED, COLOR_LIGHT_GREEN, COLOR_YELLOW,
        COLOR_LIGHT_BLUE, COLOR_LIGHT_MAGENTA, COLOR_LIGHT_CYAN
    };

    for (int i = 0; i < 8; i++) {
        int x = verts[i][0];
        int y = verts[i][1];
        int z = verts[i][2];

        int x1 = (x * cos_table[angle_y] - z * sin_table[angle_y]) / 128;
        int z1 = (x * sin_table[angle_y] + z * cos_table[angle_y]) / 128;

        int y2 = (y * cos_table[angle_x] - z1 * sin_table[angle_x]) / 128;
        int z2 = (y * sin_table[angle_x] + z1 * cos_table[angle_x]) / 128;

        int x3 = (x1 * cos_table[angle_z] - y2 * sin_table[angle_z]) / 128;
        int y3 = (x1 * sin_table[angle_z] + y2 * cos_table[angle_z]) / 128;

        int fov = 90;
        int distance = z2 + 120;
        if (distance < 10) distance = 10; /* Safety depth guard */

            int px = 40 + (x3 * fov) / distance;
        int py = 13 + (y3 * (fov / 2)) / distance;

        /* Clamping screen projection limits */
        if (px < 0) px = 0;
        if (px >= VGA_WIDTH) px = VGA_WIDTH - 1;
        if (py < 0) py = 0;
        if (py >= VGA_HEIGHT) py = VGA_HEIGHT - 1;

        proj_x[i] = px;
        proj_y[i] = py;
    }

    for (int e = 0; e < 12; e++) {
        int v0 = edges[e][0];
        int v1 = edges[e][1];
        unsigned char edge_color = rgb_colors[(e + (frame_counter / 3)) % 6];

        draw_line_3d(proj_x[v0], proj_y[v0], proj_x[v1], proj_y[v1], '#', make_color(edge_color, COLOR_BLACK));
    }

    vga_draw_string("Stats:", make_color(COLOR_WHITE, COLOR_BLACK), 2, 21);
    vga_draw_string("Engine: ARM32 Fixed-Point 3D Pipeline (3-Axis)", make_color(COLOR_LIGHT_CYAN, COLOR_BLACK), 2, 22);
    vga_draw_string("Status: Triple-Fault Guard Active", make_color(COLOR_LIGHT_GREEN, COLOR_BLACK), 2, 23);

    vga_flush();
}

/* System Shutdown Handler (ARMv5TE Compatible) */
static void system_shutdown(void) {
    vga_clear(make_color(COLOR_WHITE, COLOR_BLACK));
    vga_draw_string("================================================================================", make_color(COLOR_YELLOW, COLOR_BLACK), 0, 10);
    vga_draw_string("                    System is powering down safely...                           ", make_color(COLOR_WHITE, COLOR_BLACK), 0, 11);
    vga_draw_string("                    It is now safe to turn off.                    ", make_color(COLOR_LIGHT_GREEN, COLOR_BLACK), 0, 12);
    vga_draw_string("================================================================================", make_color(COLOR_YELLOW, COLOR_BLACK), 0, 13);
    vga_flush();

    uart_puts("\r\n[SYSTEM] RoxiP OS powered off safely. CPU Halted.\r\n");

    /* Wait For Interrupt via CP15 c7 register (ARMv5TE architecture) */
    unsigned int zero = 0;
    while (1) {
        asm volatile("mcr p15, 0, %0, c7, c0, 4" : : "r"(zero));
    }
}

/* System Reboot Handler (ARM VersatilePB Syscon Soft Reset) */
static void system_reboot(void) {
    uart_puts("\r\n[RESTART] Rebooting RoxiP OS Kernel...\r\n");
    vga_clear(make_color(COLOR_WHITE, COLOR_BLACK));
    vga_draw_string("Rebooting RoxiP OS...", make_color(COLOR_YELLOW, COLOR_BLACK), 2, 2);
    vga_flush();

    /* Trigger VersatilePB Syscon System Reset */
    volatile unsigned int* sys_lock     = (volatile unsigned int*)(0x10000000);
    volatile unsigned int* sys_resetctl = (volatile unsigned int*)(0x10000040);

    *sys_lock = 0xA05F;       /* Unlock Syscon register write access */
    *sys_resetctl = 0x0100;   /* Assert System Reset line */

    /* Software Fallback: Jump directly to ARM Reset Vector @ 0x00000000 */
    void (*reset_vector)(void) = (void (*)(void))0x00000000;
    reset_vector();
}

/* Hexadecimal string formatter for raw register dumps */
static void hex_to_str(unsigned int val, char* buf) {
    const char hex_chars[] = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 7; i >= 0; i--) {
        buf[2 + (7 - i)] = hex_chars[(val >> (i * 4)) & 0xF];
    }
    buf[10] = '\0';
}

/* Reads ARM CP15 Coprocessor Registers directly via Inline Assembly */
static inline unsigned int read_cp15_main_id(void) {
    unsigned int val;
    asm volatile("mrc p15, 0, %0, c0, c0, 0" : "=r"(val));
    return val;
}

static inline unsigned int read_cp15_control(void) {
    unsigned int val;
    asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(val));
    return val;
}

/* Dynamic Hardware Inspector (Reads REAL ARM CP15 & MMIO Registers) */
static void execute_zcatcher(void) {
    char hex_buf[12];
    char num_buf[16];

    term_add_line(" ");

    /* 1. Read VersatilePB SYS_ID Register (0x10000000) */
    volatile unsigned int* sys_id_reg = (volatile unsigned int*)(0x10000000);
    unsigned int sys_id = *sys_id_reg;
    hex_to_str(sys_id, hex_buf);
    char sys_line[80] = "[SYSCON ID]       : ";
    strcpy(sys_line + 20, hex_buf);
    term_add_line(sys_line);

    /* 2. Read CP15 Main ID Register */
    unsigned int main_id = read_cp15_main_id();
    hex_to_str(main_id, hex_buf);
    char cpu_line[80] = "[CP15 MAIN ID]    : ";
    strcpy(cpu_line + 20, hex_buf);
    if ((main_id & 0x000F0000) == 0x00020000) {
        strcpy(cpu_line + 31, " (ARM9 Family / ARM926EJ-S)");
    }
    term_add_line(cpu_line);

    /* 3. Read CP15 System Control Register */
    unsigned int cp15_ctrl = read_cp15_control();
    hex_to_str(cp15_ctrl, hex_buf);
    char ctrl_line[80] = "[CP15 CTRL REG]   : ";
    strcpy(ctrl_line + 20, hex_buf);
    term_add_line(ctrl_line);

    /* Parse Control Bits (MMU, D-Cache, I-Cache status) */
    char flags_line[80] = "  |-- System Bits : ";
    int flags_idx = 20;
    if (cp15_ctrl & (1 << 0)) { strcpy(flags_line + flags_idx, "MMU:ON "); flags_idx += 7; }
    else { strcpy(flags_line + flags_idx, "MMU:OFF "); flags_idx += 8; }

    if (cp15_ctrl & (1 << 2)) { strcpy(flags_line + flags_idx, "D-Cache:ON "); flags_idx += 11; }
    else { strcpy(flags_line + flags_idx, "D-Cache:OFF "); flags_idx += 12; }

    if (cp15_ctrl & (1 << 12)) { strcpy(flags_line + flags_idx, "I-Cache:ON"); }
    else { strcpy(flags_line + flags_idx, "I-Cache:OFF"); }
    term_add_line(flags_line);

    /* 4. Read PL110 LCD Framebuffer MMIO Base & Control */
    hex_to_str((unsigned int)fb_pixels, hex_buf);
    char fb_line[80] = "[PL110 FB BASE]   : ";
    strcpy(fb_line + 20, hex_buf);
    term_add_line(fb_line);

    unsigned int lcd_ctrl = PL110_LCDCONTROL;
    hex_to_str(lcd_ctrl, hex_buf);
    char lcd_line[80] = "[PL110 CTRL REG]  : ";
    strcpy(lcd_line + 20, hex_buf);
    term_add_line(lcd_line);

    /* 5. Memory & Dynamic Heap Arena Readouts */
    unsigned int used_bytes = get_used_heap_bytes();
    int_to_str(used_bytes, num_buf);
    char mem_line[80] = "[HEAP RAM USED]   : ";
    strcpy(mem_line + 20, num_buf);
    strcpy(mem_line + 20 + strlen(num_buf), " Bytes / 524288 Bytes");
    term_add_line(mem_line);

    term_add_line(" ");
}

/* Dynamic Block Devices & Memory Regions Inspector (lsblk) */
static void execute_lsblk(void) {
    char num_buf[16];
    char hex_buf[12];

    term_add_line("NAME        MAJ:MIN  TYPE   BASE_ADDR   TOTAL_SIZE  USED_BYTES");
    term_add_line("-----------------------------------------------------------------");

    /* Region 1: Physical ARM Vector Table */
    term_add_line("ram_vec0      1:0    ram    0x00000000  4096 B      1024 B (Vectors)");

    /* Region 2: Dynamic Heap Arena */
    char heap_line[80] = "ram_heap0     1:1    ram    ";
    unsigned int heap_base = ((unsigned int)__bss_end + 15) & ~15;
    if (heap_base < 0x00100000) heap_base = 0x00100000 + 0x20000;
    hex_to_str(heap_base, hex_buf);
    strcpy(heap_line + 28, hex_buf);
    strcpy(heap_line + 38, "  512 KB      ");

    unsigned int used_bytes = get_used_heap_bytes();
    int_to_str(used_bytes, num_buf);
    strcpy(heap_line + 52, num_buf);
    strcpy(heap_line + 52 + strlen(num_buf), " B");
    term_add_line(heap_line);

    /* Region 3: Display Framebuffer VRAM */
    char fb_line[80] = "fb_pl1100    29:0    vram   ";
    hex_to_str((unsigned int)fb_pixels, hex_buf);
    strcpy(fb_line + 28, hex_buf);
    strcpy(fb_line + 38, "  614 KB      614400 B");
    term_add_line(fb_line);

    /* Region 4: Virtual File System Memory Table */
    char vfs_line[80] = "vfs_0     31:0    vfs    RAM         ";
    int total_vfs_bytes = 0;
    for (int i = 0; i < fs_file_count; i++) {
        if (fs_table[i].used) total_vfs_bytes += fs_table[i].size;
    }
    int_to_str(fs_file_count * sizeof(file_record_t), num_buf);
    strcpy(vfs_line + 40, num_buf);
    strcpy(vfs_line + 40 + strlen(num_buf), " B       ");

    int_to_str(total_vfs_bytes, num_buf);
    strcpy(vfs_line + 52, num_buf);
    strcpy(vfs_line + 52 + strlen(num_buf), " B (Data)");
    term_add_line(vfs_line);

    /* Region 5: Persistent CFI NOR Flash (roxip.storage) */
    char flash_line[80] = "flash_nor0   31:1    flash  ";
    hex_to_str(STORAGE_FLASH_BASE, hex_buf);
    strcpy(flash_line + 28, hex_buf);
    strcpy(flash_line + 38, "  64 MB       ");
    int_to_str(sizeof(storage_header_t), num_buf);
    strcpy(flash_line + 52, num_buf);
    strcpy(flash_line + 52 + strlen(num_buf), " B (Header)");
    term_add_line(flash_line);
}

static void execute_command(const char* cmd) {
    if (cmd[0] == '\0') {
        term_add_line("perf > ");
        return;
    }
    add_to_history(cmd);

    char prompt_line[128] = "perf > ";
    int p = 7;
    for (int i = 0; cmd[i] && p < 120; i++) {
        prompt_line[p++] = cmd[i];
    }
    prompt_line[p] = '\0';
    term_add_line(prompt_line);

    if (strcmp(cmd, "comcow") == 0 || strcmp(cmd, "comcow info") == 0) {
        term_add_line("--- comcow Display Compositor Engine ---");
        char line1[64] = "Mode: ";
        int idx = 6;
        p = 0;
        while (comcow.mode_name[p]) {
            line1[idx++] = comcow.mode_name[p++];
        }
        line1[idx] = '\0';
        term_add_line(line1);
        term_add_line("Double Buffering: Enabled (PL110 Framebuffer)");
        term_add_line("Render Pipeline: Zero-Tear Atomic Flush");
        term_add_line("Commands of Comcow available :");
        term_add_line("modes, set");
    }
    else if (strcmp(cmd, "comcow modes") == 0) {
        term_add_line("Available Video Modes & Refresh Rates:");
        term_add_line(" 1. 80x25 @ 60Hz (Standard Serial Text)");
        term_add_line(" 2. 80x25 @ 70Hz (High Refresh Text Mode)");
        term_add_line(" 3. 80x50 @ 60Hz (High Density Text Mode)");
        term_add_line(" 4. 320x200 @ 60Hz (ARM Graphics Mode)");
        term_add_line("Usage: comcow set <mode_id>");
    }
    else if (strncmp(cmd, "comcow set ", 11) == 0) {
        const char* arg = cmd + 11;
        int mode_id = arg[0] - '0';
        if (mode_id >= 1 && mode_id <= 4) {
            comcow_set_mode(mode_id);
            term_add_line("comcow: Display mode switched successfully.");
        } else {
            term_add_line("comcow: Invalid mode_id. Type 'comcow modes'");
        }
    }
    else if (strncmp(cmd, "echo ", 5) == 0) {
        const char* echo_arg = cmd + 5;
        int redirect_pos = -1;
        for (int i = 0; echo_arg[i] != '\0'; i++) {
            if (echo_arg[i] == '>') {
                redirect_pos = i;
                break;
            }
        }
        if (redirect_pos != -1) {
            char content[MAX_FILE_CONTENT], filename[MAX_FILENAME_LEN];
            int c_idx = 0;
            for (int i = 0; i < redirect_pos && c_idx < MAX_FILE_CONTENT - 1; i++) {
                if (echo_arg[i] != '"') content[c_idx++] = echo_arg[i];
            }
            while (c_idx > 0 && content[c_idx - 1] == ' ') {
                c_idx--;
            }
            content[c_idx] = '\0';

            int f_idx = 0, start_f = redirect_pos + 1;
            while (echo_arg[start_f] == ' ') {
                start_f++;
            }
            while (echo_arg[start_f] != '\0' && f_idx < MAX_FILENAME_LEN - 1) {
                if (echo_arg[start_f] != ' ') filename[f_idx++] = echo_arg[start_f];
                start_f++;
            }
            filename[f_idx] = '\0';
            if (filename[0] != '\0') {
                if (fs_write_file(filename, content)) term_add_line("FS: File written successfully.");
                else term_add_line("FS Error: Storage full or target is directory.");
            } else {
                term_add_line("echo: Syntax error near '>'");
            }
            return;
        } else {
            term_add_line(echo_arg);
            return;
        }
    }
    else if (strcmp(cmd, "soundtest") == 0 || strcmp(cmd, "play") == 0) {
        if (audio_initialized) {
            term_add_line("ARM Audio: MMIO PCM Buffer configured.");
            term_add_line("ARM Audio: DMA Buffer Ring Active.");
            term_add_line("ARM Audio: Triggering PCM Stream... [Playback Simulation OK]");
        } else {
            term_add_line("Audio Error: ARM Audio Controller not initialized.");
        }
    }
    else if (strcmp(cmd, "env") == 0) {
        for (int i = 0; i < MAX_ENV_VARS; i++) {
            if (env_table[i].used) {
                char env_line[64];
                int idx = 0;
                const char* k = env_table[i].key;
                const char* v = env_table[i].value;
                while (*k) {
                    env_line[idx++] = *k++;
                }
                env_line[idx++] = '=';
                while (*v) {
                    env_line[idx++] = *v++;
                }
                env_line[idx] = '\0';
                term_add_line(env_line);
            }
        }
    }
    else if (strncmp(cmd, "set ", 4) == 0) {
        const char* args = cmd + 4;
        char key[MAX_ENV_KEY], val[MAX_ENV_VAL];
        int eq_pos = -1;
        for (int i = 0; args[i] != '\0'; i++) {
            if (args[i] == '=') {
                eq_pos = i;
                break;
            }
        }
        if (eq_pos > 0) {
            int k = 0;
            for (k = 0; k < eq_pos && k < MAX_ENV_KEY - 1; k++) key[k] = args[k];
            key[k] = '\0';

            int v = 0, src_v = eq_pos + 1;
            while (args[src_v] != '\0' && v < MAX_ENV_VAL - 1) val[v++] = args[src_v++];
            val[v] = '\0';

            if (env_set(key, val)) term_add_line("ENV: Variable set.");
            else term_add_line("ENV Error: Storage full.");
        } else {
            term_add_line("Usage: set KEY=VALUE");
        }
    }
    else if (strncmp(cmd, "unset ", 6) == 0) {
        const char* key = cmd + 6;
        if (env_unset(key)) term_add_line("ENV: Variable removed.");
        else term_add_line("ENV: Variable not found.");
    }
    else if (strcmp(cmd, "mem") == 0) {
        term_add_line("--- RoxiP Dynamic Memory Allocator ---");
        term_add_line("Heap Size: 512 KB (524288 Bytes @ ARM RAM)");
        term_add_line("Status: malloc / free Engine Ready");
    }
    else if (strcmp(cmd, "rgbdemo") == 0) {
        rgb_framebuffer_t* fb = create_rgb_framebuffer(20, 4);
        if (fb) {
            rgb_color_t red = {255, 0, 0}, green = {0, 255, 0}, blue = {0, 0, 255};
            rgb_fill_rect(fb, 0, 0, 6, 4, red);
            rgb_fill_rect(fb, 6, 0, 7, 4, green);
            rgb_fill_rect(fb, 13, 0, 7, 4, blue);
            rgb_render_to_vga(fb, 2, 10);
            free_rgb_framebuffer(fb);
            term_add_line("RGB Software Buffer rendered cleanly.");
        } else {
            term_add_line("RGB Software Error: Memory Allocation Failed.");
        }
    }
    else if (strcmp(cmd, "ztop") == 0) current_mode = MODE_ZTOP;
    else if (strncmp(cmd, "mkdir ", 6) == 0) {
        const char* dirname = cmd + 6;
        if (fs_mkdir(dirname)) term_add_line("FS: Directory created.");
        else term_add_line("FS Error: Directory exists or table full.");
    }
    else if (strncmp(cmd, "micro ", 6) == 0) {
        const char* fname = cmd + 6;
        if (fname[0] != '\0') open_micro_editor(fname);
        else term_add_line("Usage: micro <filename>");
    }
    else if (strcmp(cmd, "help") == 0) {
        term_add_line("desk, fetch, zcatcher, lsblk, comcow, ztop, soundtest, mem, rgbdemo, ls, cat, micro, mkdir, set, unset, env, clear, reboot, shutdown, panic");
    }
    else if (strcmp(cmd, "whatweus") == 0 || strcmp(cmd, "fetch") == 0) execute_whatweus();
    else if (strcmp(cmd, "floatcube") == 0) {
        if (validate_3d_pipeline()) {
            vga_force_redraw();
            current_mode = MODE_FLOATCUBE;
            term_add_line("3D Engine: Environment check PASSED. Starting 3D pipeline...");
        } else {
            term_add_line("3D Engine Panic: Pre-flight checks failed! Launch aborted.");
        }
    }
    else if (strcmp(cmd, "dmesg") == 0) {
        term_add_line("--- RoxiP Kernel Diagnostics Log ---");
        for (int i = 0; i < boot_log_count; i++) term_add_line(boot_logs[i]);
    }
    else if (strcmp(cmd, "desk") == 0 || strcmp(cmd, "DESK") == 0) current_mode = MODE_DESK;
    else if (strcmp(cmd, "ls") == 0 || strncmp(cmd, "ls ", 3) == 0) {
        term_add_line("NAME                    SIZE        FS TYPE");
        term_add_line("-------------------------------------------");
        for (int f = 0; f < fs_file_count; f++) {
            if (fs_table[f].used) {
                char line[64];
                int idx = 0;

                /* Truncate visually to 20 chars with "..." if name exceeds bounds */
                char display_name[24];
                int n_len = strlen(fs_table[f].name);
                if (n_len > 20) {
                    for (int k = 0; k < 17; k++) display_name[k] = fs_table[f].name[k];
                    display_name[17] = '.';
                    display_name[18] = '.';
                    display_name[19] = '.';
                    display_name[20] = '\0';
                } else {
                    strcpy(display_name, fs_table[f].name);
                }

                const char* n = display_name;
                while (*n) {
                    line[idx++] = *n++;
                }
                while (idx < 24) {
                    line[idx++] = ' ';
                }

                if (fs_table[f].is_dir) {
                    const char* d_str = "<DIR>";
                    while (*d_str) {
                        line[idx++] = *d_str++;
                    }
                    while (idx < 36) {
                        line[idx++] = ' ';
                    }
                } else {
                    char sz_buf[16];
                    int_to_str(fs_table[f].size, sz_buf);
                    int s_ptr = 0;
                    while (sz_buf[s_ptr]) {
                        line[idx++] = sz_buf[s_ptr++];
                    }
                    line[idx++] = 'B';
                    while (idx < 36) {
                        line[idx++] = ' ';
                    }
                }

                const char* fs_tag = fs_table[f].is_dir ? "DIR" : (fs_table[f].is_vfat ? "VFAT/FAT32" : "VFS");
                while (*fs_tag) {
                    line[idx++] = *fs_tag++;
                }
                line[idx] = '\0';
                term_add_line(line);
            }
        }
    }
    else if (strncmp(cmd, "cat ", 4) == 0) {
        const char* target = cmd + 4;
        int found = 0;
        for (int f = 0; f < fs_file_count; f++) {
            if (fs_table[f].used && strcmp(target, fs_table[f].name) == 0) {
                if (fs_table[f].is_dir) term_add_line("cat: Target is a directory");
                else term_add_line(fs_table[f].content);
                found = 1;
                break;
            }
        }
        if (!found) term_add_line("cat: File not found");
    }
    else if (strcmp(cmd, "uname") == 0) term_add_line("RoxiP 0.1-RELEASE armv7l monolithic ARM");
    else if (strcmp(cmd, "clear") == 0) term_clear_history();\
    else if (strcmp(cmd, "panic") == 0) {
        kernel_panic("Manual Panic Triggered from perf Terminal");
    }
    else if (strcmp(cmd, "lsblk") == 0) {
        execute_lsblk();
    }
    else if (strcmp(cmd, "zcatcher") == 0) {
        execute_zcatcher();
    }
    else if (strcmp(cmd, "reboot") == 0) {
        system_reboot();
    }
    else if (strcmp(cmd, "shutdown") == 0 || strcmp(cmd, "poweroff") == 0) {
        system_shutdown();
    }
    else if (strcmp(cmd, "pwd") == 0) {
        char pwd_line[80] = "Path: ";
        strcpy(pwd_line + 6, current_dir);
        term_add_line(pwd_line);
    }
    else if (strcmp(cmd, "cd") == 0 || strncmp(cmd, "cd ", 3) == 0) {
        const char* target = (strncmp(cmd, "cd ", 3) == 0) ? cmd + 3 : "/";
        if (strcmp(target, "..") == 0 || strcmp(target, "/") == 0) {
            strcpy(current_dir, "/");
            term_add_line("FS: Current directory set to [/]");
        } else {
            int found = 0;
            for (int f = 0; f < fs_file_count; f++) {
                if (fs_table[f].used && fs_table[f].is_dir && strcmp(fs_table[f].name, target) == 0) {
                    strcpy(current_dir, target);
                    found = 1;
                    char msg[80] = "FS: Directory changed to /";
                    strcpy(msg + 26, target);
                    term_add_line(msg);
                    break;
                }
            }
            if (!found) {
                term_add_line("cd: No such directory");
            }
        }
    }
    else if (strcmp(cmd, "help") == 0) {
        term_add_line("desk, fetch, zcatcher, lsblk, comcow, ztop, soundtest, mem, rgbdemo, ls, cd, pwd, cat, micro, mkdir, set, unset, env, clear, reboot, shutdown, panic");
    }
    else if (strcmp(cmd, "ls") == 0 || strncmp(cmd, "ls ", 3) == 0) {
        term_add_line("NAME                    SIZE        FS TYPE");
        term_add_line("-------------------------------------------");
        for (int f = 0; f < fs_file_count; f++) {
            if (fs_table[f].used) {
                char line[64];
                int idx = 0;

                /* Truncate visually to 20 chars with "..." if name exceeds bounds */
                char display_name[24];
                int n_len = strlen(fs_table[f].name);
                if (n_len > 20) {
                    for (int k = 0; k < 17; k++) display_name[k] = fs_table[f].name[k];
                    display_name[17] = '.';
                    display_name[18] = '.';
                    display_name[19] = '.';
                    display_name[20] = '\0';
                } else {
                    strcpy(display_name, fs_table[f].name);
                }

                const char* n = display_name;
                while (*n) {
                    line[idx++] = *n++;
                }
                while (idx < 24) {
                    line[idx++] = ' ';
                }

                if (fs_table[f].is_dir) {
                    const char* d_str = "<DIR>";
                    while (*d_str) {
                        line[idx++] = *d_str++;
                    }
                    while (idx < 36) {
                        line[idx++] = ' ';
                    }
                } else {
                    char sz_buf[16];
                    int_to_str(fs_table[f].size, sz_buf);
                    int s_ptr = 0;
                    while (sz_buf[s_ptr]) {
                        line[idx++] = sz_buf[s_ptr++];
                    }
                    line[idx++] = 'B';
                    while (idx < 36) {
                        line[idx++] = ' ';
                    }
                }

                const char* fs_tag = fs_table[f].is_dir ? "DIR" : (fs_table[f].is_vfat ? "VFAT/FAT32" : "VFS");
                while (*fs_tag) {
                    line[idx++] = *fs_tag++;
                }
                line[idx] = '\0';
                term_add_line(line);
            }
        }
    }
    else term_add_line("perf: command not found. Type 'help'");
}

static void draw_top_header(void) {
    unsigned char bar_top = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_BLACK, COLOR_LIGHT_GREY);
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_draw_char_at(' ', bar_top, x, 0);
    }
    vga_draw_string(" RoxiP Core (comcow)", bar_top, 0, 0);
    const char anim_chars[] = {'|', '/', '-', '\\'};
    char current_anim = anim_chars[(bios_anim_frame / 4) % 4];
    bios_anim_frame++;
    vga_draw_string("Hub", bar_top, 68, 0);
    vga_draw_char_at('[', bar_top, 74, 0);
    vga_draw_char_at(current_anim, (current_theme == THEME_MAC_MONO) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_LIGHT_RED, COLOR_LIGHT_GREY), 75, 0);
    vga_draw_char_at(']', bar_top, 76, 0);
}

static void render_desktop(void) {
    if (hub_hidden) {
        vga_clear(make_color(COLOR_BLACK, COLOR_BLACK));
        draw_mouse_cursor();
        vga_flush();
        set_hardware_cursor(79, 24);
        return;
    }
    if (current_theme == THEME_MAC_MONO) {
        unsigned char mac_dither = make_color(COLOR_DARK_GREY, COLOR_WHITE);
        for (int y = 0; y < VGA_HEIGHT; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                vga_draw_char_at((char)MAC_PATTERN, mac_dither, x, y);
            }
        }
    } else {
        vga_clear(make_color(COLOR_WHITE, COLOR_BLUE));
    }

    draw_top_header();
    const char* tabs[3] = {" Main ", " Settings ", " Exit "};
    int tab_x[3] = {2, 10, 22};
    for (int i = 0; i < 3; i++) {
        unsigned char tab_color = (current_theme == THEME_MAC_MONO) ? ((i == active_tab) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_BLACK, COLOR_WHITE)) : ((i == active_tab) ? make_color(COLOR_WHITE, COLOR_BLUE) : make_color(COLOR_BLACK, COLOR_LIGHT_GREY));
        vga_draw_string(tabs[i], tab_color, tab_x[i], 1);
    }

    unsigned char box_border = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_LIGHT_CYAN, COLOR_BLUE);
    unsigned char box_bg     = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_WHITE, COLOR_BLUE);
    draw_box(2, 3, 50, 15, box_border, box_bg);

    if (active_tab == 0) {
        vga_draw_string(" System Information ", (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_YELLOW, COLOR_BLUE), 5, 3);
        vga_draw_string("CPU Model      : ", box_bg, 4, 5);
        vga_draw_string(cpu_model_string, box_bg, 21, 5);
        vga_draw_string("Primary Disk   : ", box_bg, 4, 7);
        vga_draw_string(disk_model_string, box_bg, 21, 7);
        vga_draw_string("Compositor     : comcow Engine", box_bg, 4, 9);
        vga_draw_string("System Date    : [09/30/2026]", box_bg, 4, 10);
        vga_draw_string("File System    : VFS (FAT32 / VFAT supports)", box_bg, 4, 12);
        vga_draw_string("Console Shell  : perf Terminal (perf >)", box_bg, 4, 13);
    } else if (active_tab == 1) {
        vga_draw_string(" Settings & Themes ", (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_YELLOW, COLOR_BLUE), 5, 3);
        vga_draw_string("Select Desktop Theme:", box_bg, 4, 5);
        vga_draw_string(current_theme == THEME_DEFAULT ? "[X] 1. Default Blue Classic" : "[ ] 1. Default Blue Classic", box_bg, 6, 7);
        vga_draw_string(current_theme == THEME_MAC_MONO ? "[X] 2. Monochrome Theme" : "[ ] 2. Monochrome Theme", box_bg, 6, 9);
        vga_draw_string("Press [1] or [2] to apply theme.", box_bg, 4, 12);
    } else {
        vga_draw_string(" Exit RoxiP Desk ", (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_YELLOW, COLOR_BLUE), 5, 3);
        vga_draw_string("Press [ENTER] to exit Desk mode and return", box_bg, 4, 6);
        vga_draw_string("to perf Terminal / CLI shell environment.", box_bg, 4, 7);
    }

    draw_box(53, 3, 25, 15, box_border, box_bg);
    vga_draw_string(" Controls ", (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_YELLOW, COLOR_BLUE), 55, 3);
    vga_draw_string("Tab / Arrows:", box_bg, 55, 5);
    vga_draw_string("Switch Tabs", box_bg, 55, 6);
    vga_draw_string("ENTER on Exit:", box_bg, 55, 8);
    vga_draw_string("Close Desk mode", box_bg, 55, 9);
    vga_draw_string("Right Ctrl: Toggle", box_bg, 55, 11);
    vga_draw_string("empty black screen", box_bg, 55, 12);

    unsigned char bar_bottom = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_BLACK, COLOR_CYAN);
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_draw_char_at(' ', bar_bottom, x, VGA_HEIGHT - 1);
    }
    vga_draw_string(" Tab/Arrows: Nav | Enter: Action | R-Ctrl: Hide | Alt/F1: perf Terminal ", bar_bottom, 1, VGA_HEIGHT - 1);

    draw_mouse_cursor();
    vga_flush();
    set_hardware_cursor(79, 24);
}

static void render_perf_window(void) {
    if (hub_hidden) {
        vga_clear(make_color(COLOR_BLACK, COLOR_BLACK));
        draw_mouse_cursor();
        vga_flush();
        set_hardware_cursor(79, 24);
        return;
    }
    draw_top_header();

    unsigned char bg_blue = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_WHITE, COLOR_BLUE);
    for (int y = 1; y < 24; y++) {
        vga_draw_char_at(' ', bg_blue, 0, y);
        vga_draw_char_at(' ', bg_blue, 79, y);
    }

    unsigned char bar_bottom = (current_theme == THEME_MAC_MONO) ? make_color(COLOR_WHITE, COLOR_BLACK) : make_color(COLOR_BLACK, COLOR_CYAN);
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_draw_char_at(' ', bar_bottom, x, VGA_HEIGHT - 1);
    }
    vga_draw_string(" Alt/F1: Back to Hub  |  R-Ctrl: Hide Hub  |  Type 'help' for commands ", bar_bottom, 1, VGA_HEIGHT - 1);

    int start_x = 1, start_y = 1, width = 78, height = 23;
    unsigned char win_bg = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
    unsigned char border_color = make_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    unsigned char prompt_color = make_color(COLOR_LIGHT_GREEN, COLOR_BLACK);

    for (int y = start_y; y < start_y + height; y++) {
        for (int x = start_x; x < start_x + width; x++) {
            vga_draw_char_at(' ', win_bg, x, y);
        }
    }

    vga_draw_char_at('+', border_color, start_x, start_y);
    vga_draw_char_at('+', border_color, start_x + width - 1, start_y);
    vga_draw_char_at('+', border_color, start_x, start_y + height - 1);
    vga_draw_char_at('+', border_color, start_x + width - 1, start_y + height - 1);

    for (int x = start_x + 1; x < start_x + width - 1; x++) {
        vga_draw_char_at('-', border_color, x, start_y);
        vga_draw_char_at('-', border_color, x, start_y + height - 1);
    }
    for (int y = start_y + 1; y < start_y + height - 1; y++) {
        vga_draw_char_at('|', border_color, start_x, y);
        vga_draw_char_at('|', border_color, start_x + width - 1, y);
    }

    vga_draw_string("[ perf Terminal ]", make_color(COLOR_YELLOW, COLOR_BLACK), start_x + 30, start_y);
    for (int i = 0; i < term_row_count; i++) {
        vga_draw_string(term_lines[i], win_bg, start_x + 2, start_y + 1 + i);
    }

    int prompt_y = start_y + 1 + term_row_count;
    vga_draw_string("perf > ", prompt_color, start_x + 2, prompt_y);
    for (int i = 0; i < input_pos; i++) {
        vga_draw_char_at(input_buffer[i], win_bg, start_x + 9 + i, prompt_y);
    }

    draw_mouse_cursor();
    vga_flush();
    set_hardware_cursor(start_x + 9 + input_pos, prompt_y);
}

/* PS/2 Set 2 (AT) Scancodes to ASCII Map for PL050 KMI */
static const char ps2_set2_ascii[128] = {
    [0x1C] = 'a', [0x32] = 'b', [0x21] = 'c', [0x23] = 'd', [0x24] = 'e',
    [0x2B] = 'f', [0x34] = 'g', [0x33] = 'h', [0x43] = 'i', [0x3B] = 'j',
    [0x42] = 'k', [0x4B] = 'l', [0x3A] = 'm', [0x31] = 'n', [0x44] = 'o',
    [0x4D] = 'p', [0x15] = 'q', [0x2D] = 'r', [0x1B] = 's', [0x2C] = 't',
    [0x3C] = 'u', [0x2A] = 'v', [0x1D] = 'w', [0x22] = 'x', [0x35] = 'y',
    [0x1A] = 'z',

    [0x45] = '0', [0x16] = '1', [0x1E] = '2', [0x26] = '3', [0x25] = '4',
    [0x2E] = '5', [0x36] = '6', [0x3D] = '7', [0x3E] = '8', [0x46] = '9',

    [0x29] = ' ',  /* Space */
    [0x5A] = '\n', /* Enter */
    [0x66] = '\b', /* Backspace */
    [0x0D] = '\t', /* Tab */
    [0x76] = 27,   /* Escape */

    [0x4E] = '-', [0x55] = '=', [0x54] = '[', [0x5B] = ']', [0x5D] = '\\',
    [0x4C] = ';', [0x52] = '\'', [0x0E] = '`', [0x41] = ',', [0x49] = '.',
    [0x4A] = '/'
};

/* Shifted ASCII map for PS/2 Set 2 */
static const char ps2_set2_shifted[128] = {
    [0x1C] = 'A', [0x32] = 'B', [0x21] = 'C', [0x23] = 'D', [0x24] = 'E',
    [0x2B] = 'F', [0x34] = 'G', [0x33] = 'H', [0x43] = 'I', [0x3B] = 'J',
    [0x42] = 'K', [0x4B] = 'L', [0x3A] = 'M', [0x31] = 'N', [0x44] = 'O',
    [0x4D] = 'P', [0x15] = 'Q', [0x2D] = 'R', [0x1B] = 'S', [0x2C] = 'T',
    [0x3C] = 'U', [0x2A] = 'V', [0x1D] = 'W', [0x22] = 'X', [0x35] = 'Y',
    [0x1A] = 'Z',

    [0x45] = ')', [0x16] = '!', [0x1E] = '@', [0x26] = '#', [0x25] = '$',
    [0x2E] = '%', [0x36] = '^', [0x3D] = '&', [0x3E] = '*', [0x46] = '(',

    [0x29] = ' ',  [0x5A] = '\n', [0x66] = '\b', [0x0D] = '\t', [0x76] = 27,

    [0x4E] = '_', [0x55] = '+', [0x54] = '{', [0x5B] = '}', [0x5D] = '|',
    [0x4C] = ':', [0x52] = '"', [0x0E] = '~', [0x41] = '<', [0x49] = '>',
    [0x4A] = '?'
};

/* Keep this helper function */
static void init_kmi_keyboard(void) {
    KMI0_CR = 0x14; /* Enable KMI keyboard controller */
}

static char get_keyboard_input(void) {
    char c = uart_getc_nonblocking();
    if (c) return c;

    if (KMI0_STAT & (1 << 4)) {
        unsigned char scancode = KMI0_DATA;

        /* Extended Scancode Prefix (0xE0) */
        if (scancode == 0xE0) {
            ps2_extended_flag = 1;
            return 0;
        }

        /* Key Release Prefix (0xF0) */
        if (scancode == 0xF0) {
            ps2_release_flag = 1;
            return 0;
        }

        /* Key Release Event Handler */
        if (ps2_release_flag) {
            ps2_release_flag = 0;
            if (scancode == 0x12 || scancode == 0x59) shift_pressed = 0;
            if (scancode == 0x11) alt_pressed = 0;
            ps2_extended_flag = 0;
            return 0;
        }

        /* Shift & Alt Key Press Handlers */
        if (scancode == 0x12 || scancode == 0x59) { shift_pressed = 1; return 0; }
        if (scancode == 0x11) { alt_pressed = 1; return 0; }

        /* Caps Lock Toggle */
        if (scancode == 0x58) {
            caps_lock_active = !caps_lock_active;
            return 0;
        }

        /* Extended Navigation Keys (Arrows) */
        if (ps2_extended_flag) {
            ps2_extended_flag = 0;
            if (scancode == 0x75) return KEY_UP;
            if (scancode == 0x72) return KEY_DOWN;
            if (scancode == 0x6B) return KEY_LEFT;
            if (scancode == 0x74) return KEY_RIGHT;
        }

        /* Function Keys */
        if (scancode == 0x05) return KEY_F1;
        if (scancode == 0x06) return KEY_F2;
        if (scancode == 0x04) return KEY_F3;
        if (scancode == 0x0C) return KEY_F4;

        /* Letter/Symbol Character Translation */
        char base_char = ps2_set2_ascii[scancode];
        if (base_char >= 'a' && base_char <= 'z') {
            int use_upper = shift_pressed ^ caps_lock_active;
            return use_upper ? ps2_set2_shifted[scancode] : base_char;
        }

        if (shift_pressed && ps2_set2_shifted[scancode]) {
            return ps2_set2_shifted[scancode];
        }

        return base_char;
    }
    return 0;
}

static void render_cli(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            unsigned int hash = ((x * 37) ^ (y * 109) ^ (x * y * 17)) % 100;
            if (hash < 3) vga_draw_char_at('*', make_color(COLOR_WHITE, COLOR_BLACK), x, y);
            else if (hash < 8) vga_draw_char_at('.', make_color(COLOR_DARK_GREY, COLOR_BLACK), x, y);
            else if (hash < 12) vga_draw_char_at('+', make_color(COLOR_LIGHT_CYAN, COLOR_BLACK), x, y);
            else if (hash < 15) vga_draw_char_at('.', make_color(COLOR_MAGENTA, COLOR_BLACK), x, y);
            else vga_draw_char_at(' ', make_color(COLOR_WHITE, COLOR_BLACK), x, y);
        }
    }

    vga_draw_string("RoxiP OS v0.1 - Monolithic Kernel (perf Terminal)", make_color(COLOR_WHITE, COLOR_BLACK), 0, 0);
    vga_draw_string("Type 'desk' to launch Desktop Hub or 'fetch' for system info.", make_color(COLOR_DARK_GREY, COLOR_BLACK), 0, 1);
    vga_draw_string("Welcome to RoxiP OS! A bare-metal OS based on .bin", make_color(COLOR_LIGHT_CYAN, COLOR_BLACK), 1, 2);

    for (int i = 0; i < term_row_count; i++) {
        vga_draw_string(term_lines[i], make_color(COLOR_LIGHT_GREEN, COLOR_BLACK), 0, 3 + i);
    }

    int prompt_y = 3 + term_row_count;
    vga_draw_string("perf > ", make_color(COLOR_YELLOW, COLOR_BLACK), 0, prompt_y);
    for (int i = 0; i < input_pos; i++) {
        vga_draw_char_at(input_buffer[i], make_color(COLOR_MAGENTA, COLOR_BLACK), 7 + i, prompt_y);
    }

    draw_mouse_cursor();
    vga_flush();
    set_hardware_cursor(7 + input_pos, prompt_y);
}

static void redraw_current_view(void) {
    if (current_mode == MODE_PERF_CLI) render_cli();
    else if (current_mode == MODE_DESK) render_desktop();
    else if (current_mode == MODE_DESK_WINDOW) render_perf_window();
    else if (current_mode == MODE_FLOATCUBE) render_floatcube_frame();
    else if (current_mode == MODE_MICRO_EDITOR) render_micro_editor();
    else if (current_mode == MODE_ZTOP) render_ztop_frame();
}

static void process_char_input(char c) {
    if (c == '\r') c = '\n';

    /* 1. ESC (27) or 'q' / 'Q' Exit Handler for floatcube & ztop */
    if ((c == 27 || c == 'q' || c == 'Q') && (current_mode == MODE_FLOATCUBE || current_mode == MODE_ZTOP)) {
        current_mode = MODE_PERF_CLI;
        redraw_current_view();
        return;
    }

    /* 2. Theme Selection in Desk Mode (Settings Tab: active_tab == 1) */
    if (current_mode == MODE_DESK && active_tab == 1) {
        if (c == '1') {
            current_theme = THEME_DEFAULT;
            redraw_current_view();
            return;
        } else if (c == '2') {
            current_theme = THEME_MAC_MONO;
            redraw_current_view();
            return;
        }
    }

    /* F1 Key or Alt Key: Switch between Hub Desk and perf CLI */
    if (c == KEY_F1 || (alt_pressed && (c == '1' || c == '\n'))) {
        if (current_mode == MODE_DESK) current_mode = MODE_PERF_CLI;
        else current_mode = MODE_DESK;
        redraw_current_view();
        return;
    }

    /* Arrow Keys: Move Mouse Cursor in GUI modes */
    if (c == KEY_UP) { if (mouse_y > 0) mouse_y--; redraw_current_view(); return; }
    if (c == KEY_DOWN) { if (mouse_y < VGA_HEIGHT - 1) mouse_y++; redraw_current_view(); return; }
    if (c == KEY_LEFT) { if (mouse_x > 0) mouse_x--; redraw_current_view(); return; }
    if (c == KEY_RIGHT) { if (mouse_x < VGA_WIDTH - 1) mouse_x++; redraw_current_view(); return; }

    /* Standard Tab Navigation */
    if (c == '\t') {
        active_tab = (active_tab + 1) % 3;
        if (current_mode == MODE_DESK) render_desktop();
        return;
    }

    /* Standard text processing logic */
    if (c == '\n') {
        input_buffer[input_pos] = '\0';
        if (current_mode == MODE_DESK) {
            if (active_tab == 2) {
                current_mode = MODE_PERF_CLI;
                active_tab = 0;
            }
        } else {
            execute_command(input_buffer);
        }
        input_pos = 0;
        redraw_current_view();
    } else if (c == '\b' || c == 127) {
        if (input_pos > 0) {
            input_pos--;
            redraw_current_view();
        }
    } else if (c >= 32 && c <= 126 && input_pos < 72) {
        input_buffer[input_pos++] = c;
        redraw_current_view();
    }
}

/* VersatilePB SP804 System Timer MMIO Registers */
#define TIMER0_BASE  0x101E2000
#define TIMER0_LOAD  (*((volatile unsigned int*)(TIMER0_BASE + 0x00)))
#define TIMER0_VALUE (*((volatile unsigned int*)(TIMER0_BASE + 0x04)))
#define TIMER0_CTRL  (*((volatile unsigned int*)(TIMER0_BASE + 0x08)))

/* Precise Hardware Delay in Milliseconds using ARM SP804 Timer (1 MHz Clock) */
static void delay_ms(unsigned int ms) {
    TIMER0_CTRL = 0x82; /* Bit 7: Enable | Bit 1: 32-bit counter mode */
    unsigned int start = TIMER0_VALUE;
    unsigned int ticks_to_wait = ms * 1000; /* 1000 ticks = 1 ms */

    while ((start - TIMER0_VALUE) < ticks_to_wait) {
        asm volatile("nop");
    }
}

/* Interactive ARM Bootloader Menu with Real-Time Hardware Timer & Arrow Navigation */
static void run_arm_bootloader(void) {
    int seconds_remaining = 10;
    int selected_option = 0; /* 0 = Boot OS, 1 = Reboot */

    unsigned char logo_color  = make_color(COLOR_LIGHT_MAGENTA, COLOR_BLACK);
    unsigned char title_color = make_color(COLOR_YELLOW, COLOR_BLACK);
    unsigned char timer_color = make_color(COLOR_LIGHT_CYAN, COLOR_BLACK);

    while (seconds_remaining > 0) {
        vga_clear(make_color(COLOR_WHITE, COLOR_BLACK));

        /* 1. Background Starfield Effect */
        for (int y = 0; y < VGA_HEIGHT; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                unsigned int hash = ((x * 37) ^ (y * 109) ^ (x * y * 17)) % 100;
                if (hash < 2) vga_draw_char_at('*', make_color(COLOR_WHITE, COLOR_BLACK), x, y);
                else if (hash < 6) vga_draw_char_at('.', make_color(COLOR_DARK_GREY, COLOR_BLACK), x, y);
            }
        }

        /* 2. Render Purple whatweus Logo */
        vga_draw_string("  /\\  ", logo_color, 37, 2);
        vga_draw_string(" /  \\ ", logo_color, 37, 3);
        vga_draw_string("/ /\\ \\", logo_color, 37, 4);
        vga_draw_string("\\ \\/ /", logo_color, 37, 5);
        vga_draw_string(" \\  / ", logo_color, 37, 6);
        vga_draw_string("  \\/  ", logo_color, 37, 7);

        /* 3. Title Banner shifted down */
        vga_draw_string("=== RoxiP Bootloader v0.2 (Core) ===", title_color, 20, 9);

        /* 4. Menu Options with Dynamic Arrow Selection */
        unsigned char opt1_color = (selected_option == 0) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_WHITE, COLOR_BLACK);
        unsigned char opt2_color = (selected_option == 1) ? make_color(COLOR_BLACK, COLOR_WHITE) : make_color(COLOR_WHITE, COLOR_BLACK);

        vga_draw_string(selected_option == 0 ? " > [1] Boot RoxiP OS   " : "   [1] Boot RoxiP OS   ", opt1_color, 28, 12);
        vga_draw_string(selected_option == 1 ? " > [2] Reboot System   " : "   [2] Reboot System   ", opt2_color, 28, 14);

        /* 5. Countdown Timer String */
        char timer_str[80] = "Auto-booting in ";
        char sec_buf[8];
        int_to_str(seconds_remaining, sec_buf);
        strcpy(timer_str + 16, sec_buf);
        strcpy(timer_str + 16 + strlen(sec_buf), "s... (Arrows=Select, ENTER=Confirm)");

        vga_draw_string(timer_str, timer_color, 16, 17);

        vga_flush();

        /* Poll input for exactly 1 second (20 sub-steps * 50ms) */
        int key_interrupted = 0;
        for (int step = 0; step < 20; step++) {
            char c = get_keyboard_input();
            if (c) {
                if (c == KEY_UP) {
                    selected_option = 0;
                    key_interrupted = 1;
                    break;
                }
                else if (c == KEY_DOWN) {
                    selected_option = 1;
                    key_interrupted = 1;
                    break;
                }
                else if (c == '1') {
                    selected_option = 0;
                    return;
                }
                else if (c == '2') {
                    system_reboot();
                }
                else if (c == '\n' || c == '\r') {
                    if (selected_option == 0) {
                        return;
                    } else {
                        system_reboot();
                    }
                }
            }
            delay_ms(50); /* Real-time 50 ms delay via SP804 Timer */
        }

        if (!key_interrupted) {
            seconds_remaining--;
        }
    }
}

/* Kernel Entry Point */
void kmain(void) {
    dbg_line = 0;
    dbg_log("[01/12] Entering kmain()...");

    dbg_log("[02/12] Calling init_heap()...");
    init_heap();

    dbg_log("[03/12] Initializing ARM Vector Table...");
    init_idt();

    dbg_log("[04/12] Calling init_env_vars()...");
    init_env_vars();

    dbg_log("[05/12] Querying CPU Architecture...");
    detect_cpu_brand(cpu_model_string);

    dbg_log("[06/12] Initializing PL110 LCD Framebuffer...");
    init_pl110_lcd();
    detect_gpu(gpu_model_string);

    dbg_log("[07/12] Checking Audio Engine...");
    init_hda_audio();

    dbg_log("[08/12] Querying Storage Controller...");
    detect_primary_disk(disk_model_string);

    dbg_log("[09/12] Mounting File Systems...");
    init_filesystems();

    dbg_log("[10/12] Initializing comcow Compositor...");
    init_comcow();

    dbg_log("[11/12] Initializing UART Serial Input Driver...");
    log_boot_event("[OK] ARM PL011 UART Serial Input Active");

    init_kmi_keyboard();
    init_kmi_mouse();

    dbg_log("[12/12] Launching Interactive Bootloader...");
    run_arm_bootloader(); /* <--- Menú del Bootloader con conteo */

    dbg_log("Launching CLI Shell View...");
    render_cli();

    init_kmi_keyboard();
    init_kmi_mouse();

    while (1) {
        timer_ticks++;
        poll_kmi_mouse();

        if (current_mode == MODE_FLOATCUBE) {
            render_floatcube_frame();
            delay_cycles(150000); /* Throttle render rate to ~60 FPS */
        }
        else if (current_mode == MODE_ZTOP && (timer_ticks % 1000 == 0)) {
            render_ztop_frame();
        }

        char c = get_keyboard_input();
        if (c) {
            process_char_input(c);
        }
    }
}
