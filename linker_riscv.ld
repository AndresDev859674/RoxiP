/* linker_riscv.ld - roxiP RISC-V linker script
 * RISC-V virt kernel load address: 0x80000000 (standard for OpenSBI payload)
 */

ENTRY(_start)

SECTIONS
{
    . = 0x80000000;

    .text : {
        *startup_riscv.o(.text*)
        *(.text*)
        *(.rodata*)        
        . = ALIGN(4);
    }

    .data : {
        *(.data*)
        . = ALIGN(4);
    }

    .bss : {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    }

    _end = .;
}
