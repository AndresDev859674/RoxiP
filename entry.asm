; ==============================================================================
; RoxiP Kernel Entry Trampoline, Standalone GDT & Multiboot Header
; Target: x86 32-bit Protected Mode
; ==============================================================================

[bits 32]
global _start
global dummy_isr_wrapper
[extern kmain]
[extern __bss_start]
[extern __bss_end]

; Multiboot Header Setup
MULTIBOOT_PAGE_ALIGN   equ 1 << 0
MULTIBOOT_MEMORY_INFO  equ 1 << 1
MULTIBOOT_HEADER_MAGIC equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MULTIBOOT_CHECKSUM     equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

SECTION .multiboot
align 4
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_HEADER_FLAGS
    dd MULTIBOOT_CHECKSUM

; SECCIÓN DEDICADA PARA EL STACK (Separada de .bss para evitar autodestrucción)
SECTION .stack nobits align=16
stack_bottom:
    resb 16384              ; 16 KB Dedicated Stack
stack_top:

SECTION .text
_start:
    cli                     ; Disable interrupts immediately

    ; 1. Load our own clean GDT
    lgdt [gdt_descriptor]

    ; 2. Far Jump to reload Code Segment (CS = 0x08)
    jmp 0x08:reload_cs

reload_cs:
    ; 3. Reload Data Segment registers (DS, ES, FS, GS, SS = 0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 4. Setup Stack Pointer aligned to 16 bytes
    mov esp, stack_top
    and esp, 0xFFFFFFF0

    ; Visual Checkpoint 1: Protected Mode & GDT OK (White 'P')
    mov word [0xB8000], 0x0F50

    ; 5. Disable PIC & Mask NMI Interrupts
    mov al, 0xFF
    out 0xA1, al
    out 0x21, al

    in al, 0x70
    or al, 0x80
    out 0x70, al            ; Disable NMI

    ; 6. Zero out .bss memory section safely
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    cmp ecx, 0
    jle .skip_bss           ; Evitar bucle si BSS es 0 o negativo
    xor eax, eax
    cld
    rep stosb

.skip_bss:
    ; Visual Checkpoint 2: BSS Cleaned OK (Green 'B' en la segunda posición)
    mov word [0xB8002], 0x0A42

    ; 7. Call C kernel main entry point
    call kmain

.hang:
    cli
    hlt
    jmp .hang

; Catch-All Interrupt Handler
dummy_isr_wrapper:
    cli
.loop:
    hlt
    jmp .loop

; ==============================================================================
; Standalone Flat 4GB GDT (Global Descriptor Table)
; ==============================================================================
align 8
gdt_start:
    ; Null Descriptor (0x00)
    dd 0x0
    dd 0x0

    ; Code Segment Descriptor (Selector 0x08): Base=0, Limit=4GB, Code/Read
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00

    ; Data Segment Descriptor (Selector 0x10): Base=0, Limit=4GB, Data/Write
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
