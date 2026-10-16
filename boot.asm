; ==============================================================================
; RoxiP Bootloader v0.1 - Robust Floppy Loader & PM Transition
; Target: x86 Bare-Metal MBR Bootsector (512 Bytes)
; ==============================================================================

[org 0x7c00]
[bits 16]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov [boot_drive], dl

    ; 1. Query RAM via BIOS (INT 15h)
    xor cx, cx
    xor dx, dx
    mov ax, 0xe801
    int 0x15
    jnc .mem_ok
    mov ah, 0x88
    int 0x15
.mem_ok:
    mov [0x7000], ax
    mov [0x7002], bx

    ; 2. Dibujar Fondo Galaxia en VRAM
    mov ax, 0xb800
    mov es, ax
    xor di, di
    mov cx, 2000
    xor bx, bx

galaxy_loop:
    mov ax, bx
    imul ax, 37
    add ax, 109
    xor ax, bx
    mov dx, 0
    mov si, 100
    div si

    cmp dx, 3
    jl .draw_star1
    cmp dx, 8
    jl .draw_star2
    cmp dx, 12
    jl .draw_star3
    mov word [es:di], 0x0720
    jmp .next_cell

.draw_star1:
    mov word [es:di], 0x0F2A
    jmp .next_cell
.draw_star2:
    mov word [es:di], 0x082E
    jmp .next_cell
.draw_star3:
    mov word [es:di], 0x0B2B

.next_cell:
    add di, 2
    inc bx
    loop galaxy_loop

    ; 3. Render Banner Limine
    xor ax, ax
    mov ds, ax
    mov si, boot_msg
    mov di, (80 * 2 + 18) * 2

print_banner:
    lodsb
    or al, al
    jz load_kernel
    mov ah, 0x0E
    mov [es:di], ax
    add di, 2
    jmp print_banner

; 4. Cargar Kernel Sector por Sector con Normalización de Segmento
load_kernel:
    ; Pinta 'R1'
    mov ax, 0xb800
    mov es, ax
    mov word [es:0], 0x2F52 ; 'R'
    mov word [es:2], 0x2F31 ; '1'

    mov ax, 0x1000          ; Cargar Kernel en Segmento 0x1000 (Dirección Física 0x10000)
    mov es, ax
    xor bx, bx

    mov ch, 0               ; Cilindro 0
    mov dh, 0               ; Cabeza 0
    mov cl, 2               ; Sector 2
    mov di, 60              ; Cargar 60 sectores (~30 KB)

.read_sector_loop:
    mov si, 5               ; 5 reintentos

.retry:
    push cx
    push dx
    push di

    mov ax, 0x0201          ; AH=02 (Read), AL=01 (1 sector)
    mov dl, [boot_drive]
    int 0x13
    jnc .read_ok

    ; Reset Floppy Controller en cada fallo
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13

    pop di
    pop dx
    pop cx
    dec si
    jnz .retry
    jmp disk_error

.read_ok:
    pop di
    pop dx
    pop cx

    ; Avanzar segmento 512 bytes (0x20 párrafos en ES)
    mov ax, es
    add ax, 0x0020
    mov es, ax

    ; Geometría Floppy CHS (18 sectores/pista, 2 cabezas)
    inc cl
    cmp cl, 19
    jne .next_sector
    mov cl, 1
    inc dh
    cmp dh, 2
    jne .next_sector
    mov dh, 0
    inc ch

.next_sector:
    dec di
    jnz .read_sector_loop

    ; Pinta 'R2' al terminar la lectura sin errores
    mov ax, 0xb800
    mov es, ax
    mov word [es:4], 0x2F52 ; 'R'
    mov word [es:6], 0x2F32 ; '2'

    ; 5. Activar A20 Gate
    in al, 0x92
    or al, 2
    out 0x92, al

    ; 6. Entrada a Modo Protegido de 32 bits
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far Jump para limpiar la cola de instrucciones de 16 bits
    jmp 0x08:init_pm_32

disk_error:
    mov ax, 0xb800
    mov es, ax
    mov si, err_msg
    mov di, (80 * 12 + 22) * 2
.print_err:
    lodsb
    or al, al
    jz .halt
    mov ah, 0x0C
    mov [es:di], ax
    add di, 2
    jmp .print_err
.halt:
    cli
    hlt

; ------------------------------------------------------------------------------
; 32-bit Protected Mode Execution Target
; ------------------------------------------------------------------------------
[bits 32]
init_pm_32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov ebp, 0x90000
    mov esp, ebp

    ; Salto final al kernel en la dirección física 0x10000
    jmp 0x08:0x10000

; ------------------------------------------------------------------------------
; Global Descriptor Table (GDT)
; ------------------------------------------------------------------------------
align 8
gdt_start:
    dq 0x0                  ; Descriptor Nulo

gdt_code:                   ; Selector 0x08 (Code, Base=0, Limit=4GB)
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00

gdt_data:                   ; Selector 0x10 (Data, Base=0, Limit=4GB)
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

boot_drive db 0
boot_msg   db "=== RoxiP Bootloader v0.1 (Galaxy Edition) ===", 0
err_msg    db "Bootloader Error: Disk Read Failed!", 0

times 510 - ($ - $$) db 0
dw 0xAA55
