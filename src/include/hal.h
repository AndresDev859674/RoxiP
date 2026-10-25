#ifndef HAL_H
#define HAL_H

/* Hardware Abstraction Layer Interface for RoxiP OS */

void board_init(void);
void board_detect_cpu(char* out_buf);
void board_detect_gpu(char* out_buf);
void board_delay_ms(unsigned int ms);
void board_delay_cycles(volatile unsigned int count);

void system_shutdown(void);
void system_reboot(void);

void init_idt(void);

#endif /* HAL_H */