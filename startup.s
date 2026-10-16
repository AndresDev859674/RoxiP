/* ==============================================================================
 * RoxiP OS - ARM32 (QEMU versatilepb / ARM926EJ-S) Entry Point
 * Language: ARM Assembly (Unified Syntax)
 * ============================================================================== */

.global _start
.extern kmain, __bss_start, __bss_end

.section .text
_start:
    /* Desactivar interrupciones IRQ y FIQ (Compatible con ARMv5TE) */
    mrs r0, cpsr
    orr r0, r0, #0xC0
    msr cpsr_c, r0

    ldr sp, =stack_top          @ Inicializar Stack Pointer

    @ Bucle optimizado de limpieza BSS
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0
1:  cmp r0, r1
    strlt r2, [r0], #4
    blt 1b

    bl kmain                    @ Saltar a C kernel main

1:  b 1b                        @ Bucle infinito si sale de kmain

.section .bss
.align 3
stack_bottom:
    .space 8192                 @ 8 KB Stack Arena
stack_top:
