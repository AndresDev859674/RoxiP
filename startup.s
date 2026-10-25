/* 
 * startup.s - roxiP
/

.global _start
.extern kmain, __bss_start, __bss_end

.section .text
_start:
    b reset_handler
    .space 0x3C                 @ Reserve 60 bytes for ARM Exception Vector Table (init_idt)

reset_handler:
    /* Disable IRQ and FIQ interrupts (ARMv5TE compatible) */
    mrs r0, cpsr
    orr r0, r0, #0xC0
    msr cpsr_c, r0

    ldr sp, =stack_top          @ Initialize Stack Pointer

    /* Optimized BSS clearing loop */
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0
1:  cmp r0, r1
        strlt r2, [r0], #4
        blt 1b

    bl kmain                    @ Jump to C kernel main

1:  b 1b                        @ Infinite loop if kmain returns

.section .bss
.align 3
stack_bottom:
    .space 16384                @ 16 KB Stack Arena
stack_top: