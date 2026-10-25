#ifndef DRIVERS_H
#define DRIVERS_H

/* UART Serial Driver */
void uart_putc(char c);
void uart_puts(const char* str);
char uart_getc_nonblocking(void);

/* Display Hardware Driver */
void init_pl110_lcd(void);

/* Input Drivers (KMI PS/2 Keyboard & Mouse) */
void init_kmi_mouse(void);
void poll_kmi_mouse(void);
void init_kmi_keyboard(void);
char get_keyboard_input(void);

/* Storage & Audio Drivers */
void detect_primary_disk(char* out_buf);
void init_hda_audio(void);

#endif /* DRIVERS_H */