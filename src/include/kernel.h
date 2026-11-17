#ifndef ROXIP_KERNEL_H
#define ROXIP_KERNEL_H

#ifndef NULL
#define NULL ((void*)0)
#endif

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/*
 * Board-specific MMIO addresses (UART, KMI, PL110/LCD, timers, reset, etc.)
 * live in firmware/<BOARD>/firmware.h and are made available via the
 * -Ifirmware/$(BOARD) include path.  kernel.h intentionally does NOT hardcode
 * any SoC addresses so that the same sources compile for every board.
 */

#define FB_WIDTH  640
#define FB_HEIGHT 480

/* Colors */
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

#define TERM_ROWS 18
#define TERM_COLS 74
#define MAX_BOOT_LOGS 12
#define MAX_YAFS_FILES 16
#define MAX_FILENAME_LEN 64
#define MAX_FILE_CONTENT 2097152
#define MAX_ENV_VARS     16
#define MAX_ENV_KEY 32
#define MAX_ENV_VAL 64
#define MAX_CMD_HIST 16

#define HEAP_SIZE (2048 * 1024)

#define KEY_F1     0x81
#define KEY_F2     0x82
#define KEY_F3     0x83
#define KEY_F4     0x84
#define KEY_UP     0x88
#define KEY_DOWN   0x89
#define KEY_LEFT   0x8A
#define KEY_RIGHT  0x8B

#define MAC_PATTERN 176

#define STORAGE_FLASH_BASE 0x34000000
#define STORAGE_MAGIC      0x53465053

typedef enum {
    MODE_PERF_CLI,
    MODE_DESK,
    MODE_DESK_WINDOW,
    MODE_FLOATCUBE,
    MODE_MICRO_EDITOR,
    MODE_VFM,
    MODE_ZTOP,
    MODE_RGBDEMO
} system_mode_t;

typedef enum {
    THEME_DEFAULT = 0,   /* Industrial Grey (Default) */
    THEME_BLUE = 1,      /* Classic Blue */
    THEME_MONOCHROME = 2   /* Monochrome */
} desk_theme_t;

typedef struct {
    char mode_name[32];
    int width;
    int height;
    int refresh_rate;
    int double_buffering;
    int latency_ms;
    int active_mode_id;
} comcow_compositor_t;

typedef struct block_header {
    unsigned int size;
    int is_free;
    struct block_header* next;
} block_header_t;

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

typedef struct {
    int pid;
    const char* user;
    int cpu_usage;
    unsigned int mem_kb;
    const char* state;
    const char* name;
} process_info_t;

typedef struct {
    char name[MAX_FILENAME_LEN];
    char content[MAX_FILE_CONTENT];
    int size;
    int used;
    int is_vfat;
    int is_dir;
} file_record_t;

typedef struct {
    char key[MAX_ENV_KEY];
    char value[MAX_ENV_VAL];
    int used;
} env_var_t;

typedef struct {
    unsigned int magic;
    int file_count;
    file_record_t files[MAX_YAFS_FILES];
} storage_header_t;

extern char __bss_end[];

/* Shared Globals */
extern system_mode_t current_mode;
extern desk_theme_t current_theme;
extern comcow_compositor_t comcow;

extern char current_dir[MAX_FILENAME_LEN];
extern char input_buffer[128];
extern int input_pos;

extern char cmd_history[MAX_CMD_HIST][128];
extern int hist_count;

extern char micro_filename[MAX_FILENAME_LEN];
extern char micro_buffer[MAX_FILE_CONTENT];
extern int micro_pos;

extern int vfm_selected_index;

extern env_var_t env_table[MAX_ENV_VARS];

extern char cpu_model_string[64];
extern char gpu_model_string[64];
extern char audio_model_string[64];
extern char disk_model_string[64];
extern unsigned int timer_ticks;

extern int audio_initialized;
extern char boot_logs[MAX_BOOT_LOGS][74];
extern int boot_log_count;

extern int active_tab;
extern int hub_hidden;
extern int bios_anim_frame;

extern int mouse_x;
extern int mouse_y;

extern char term_lines[TERM_ROWS][TERM_COLS];
extern int term_row_count;

extern file_record_t fs_table[MAX_YAFS_FILES];
extern int fs_file_count;
extern int dbg_line;

extern unsigned short fb_pixels[FB_WIDTH * FB_HEIGHT];
extern unsigned short vga_virtual_buffer[VGA_WIDTH * VGA_HEIGHT];
extern unsigned short vga_backbuffer[VGA_WIDTH * VGA_HEIGHT];

extern block_header_t* free_list;
extern const unsigned char font8x8_basic[256][8];
extern const int sin_table[36];
extern const int cos_table[36];

/* Memory Prototypes */
void* memset(void* dest, int c, unsigned int n);
void* memcpy(void* dest, const void* src, unsigned int n);
void* memmove(void* dest, const void* src, unsigned int n);
int memcmp(const void* s1, const void* s2, unsigned int n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, int n);
int strlen(const char* str);
void strcpy(char* dest, const char* src);
void int_to_str(int num, char* str);
void hex_to_str(unsigned long val, char* buf);
void init_heap(void);
unsigned int get_used_heap_bytes(void);
void* malloc(unsigned int size);
void free(void* ptr);

/* Hardware & Driver Prototypes */
void uart_putc(char c);
void uart_puts(const char* str);
char uart_getc_nonblocking(void);
void init_pl110_lcd(void);
void detect_primary_disk(char* out_buf);
void init_hda_audio(void);
void kernel_panic(const char* message);
void init_idt(void);
void detect_cpu_brand(char* out_buf);
void detect_gpu(char* out_buf);
void init_kmi_keyboard(void);
void init_kmi_mouse(void);
void poll_kmi_mouse(void);
char get_keyboard_input(void);
void delay_ms(unsigned int ms);
void delay_cycles(volatile unsigned int count);
void system_shutdown(void);
void system_reboot(void);

/* YAFS Prototypes */
const char* fs_read_file(const char* filename);
void YAFS_sync_to_storage(void);
int YAFS_load_from_storage(void);
void init_filesystems(void);
int fs_write_file(const char* filename, const char* content);
int fs_write_file_binary(const char* filename, const char* content, unsigned int size);
int fs_mkdir(const char* dirname);

void execute_tar(const char* args);
int unpack_targz_in_ram(const unsigned char* src, unsigned int src_len);

/* Graphics & UI Prototypes */
int validate_3d_pipeline(void);
void init_comcow(void);
void comcow_set_mode(int mode_id);
void vga_flush(void);
void vga_clear(unsigned char color);
void vga_force_redraw(void);
void vga_draw_char_at(char c, unsigned char color, int x, int y);
void vga_draw_string(const char* str, unsigned char color, int x, int y);
void draw_box(int start_x, int start_y, int width, int height, unsigned char border_color, unsigned char bg_color);
void draw_mouse_cursor(void);

rgb_framebuffer_t* create_rgb_framebuffer(int width, int height);
void free_rgb_framebuffer(rgb_framebuffer_t* fb);
void rgb_fill_rect(rgb_framebuffer_t* fb, int rx, int ry, int rw, int rh, rgb_color_t color);
void rgb_render_to_vga(rgb_framebuffer_t* fb, int start_x, int start_y);

void term_add_line(const char* text);
void term_clear_history(void);
void open_micro_editor(const char* filename);
void render_desktop(void);
void render_ztop_frame(void);
void render_floatcube_frame(void);
void render_vfm(int selected_index);
void render_rgbdemo(void);
void redraw_current_view(void);

/* Shell Prototypes */
void init_env_vars(void);
int env_set(const char* key, const char* val);
int env_unset(const char* key);
void execute_command(const char* cmd);
void process_char_input(char c);

/* Kernel Logging Prototypes */
void log_boot_event(const char* msg);
void dbg_log(const char* msg);

/* Misc Shell Functions */
void execute_stat(const char* filename);
void execute_xxd(const char* filename);
#ifndef __riscv__
void execute_disasm(const char* arg);
#endif /* !__riscv__ */

/* Pipeline & Synthetic Nodes */
extern char pipe_buffer[4096];
extern int pipe_capture_active;

/* Ui Prototypes */
void vga_draw_line(int x0, int y0, int x1, int y1, char c, unsigned char color);

#endif