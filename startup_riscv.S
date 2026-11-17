/*
 * startup_riscv.S - RoxiP RISC-V entry point
 * Target: riscv64-unknown-elf (rv64gc, lp64 ABI)
 * Machine: QEMU virt
 *
 * Assembled with: riscv64-elf-as
 */

.section .text.init
.global _start
.extern kmain, __bss_start, __bss_end

.equ UART_THR, 0x10000000
.equ UART_LSR, 0x10000005

_start:
    /* Disable machine-mode interrupts */
    csrr t0, mstatus
    andi t0, t0, ~0x8          /* clear MIE */
    csrw mstatus, t0

    /* Set up stack pointer */
    la sp, stack_top

    /* Clear BSS section */
    la t0, __bss_start
    la t1, __bss_end
    li t2, 0
1:   bge t0, t1, 2f
    sw t2, 0(t0)
    addi t0, t0, 4
    j 1b
2:

    /* Early debug: send 'R' to UART to confirm we're alive */
    la t0, UART_LSR
1:   lb t1, 0(t0)
    andi t1, t1, 0x20          /* THRE - Transmit Holding Register Empty */
    beqz t1, 1b                /* Wait until ready */
    la t0, UART_THR
    li t1, 'R'
    sb t1, 0(t0)

    /* Set machine trap vector (mtvec) to trap_handler_entry */
    la t0, trap_handler_entry
    csrw mtvec, t0

    /* Jump to C kernel main */
    call kmain

1:   wfi
    j 1b

.section .bss
.align 3
stack_bottom:
    .space 16384               /* 16 KB stack arena */
stack_top:

.section .text
.global trap_handler_entry
.extern c_trap_handler

trap_handler_entry:
    /* Simple trap handler:
     * Swap sp with mscratch, save a few regs, call C handler, restore, mret */
    csrrw sp, mscratch, sp     /* swap sp into mscratch, get old sp back */
    addi sp, sp, -3*8          /* allocate space for ra, t0, t1 */
    sw ra, 0*8(sp)
    sw t0, 1*8(sp)
    sw t1, 2*8(sp)

    /* mcause and mepc are in their CSRs; c_trap_handler reads them directly */
    call c_trap_handler

    lw t1, 2*8(sp)
    lw t0, 1*8(sp)
    lw ra, 0*8(sp)
    addi sp, sp, 3*8
    csrrw sp, mscratch, sp     /* restore sp */
    mret
