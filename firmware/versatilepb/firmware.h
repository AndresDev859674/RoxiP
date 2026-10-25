#ifndef VERSATILEPB_FIRMWARE_H
#define VERSATILEPB_FIRMWARE_H

#define TIMER0_BASE  0x101E2000
#define TIMER0_VALUE (*((volatile unsigned int*)(TIMER0_BASE + 0x04)))
#define TIMER0_CTRL  (*((volatile unsigned int*)(TIMER0_BASE + 0x08)))

#endif /* VERSATILEPB_FIRMWARE_H */