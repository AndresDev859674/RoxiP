/* linker.ld - roxiP */

ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : {
        *startup.o(.text*)
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
