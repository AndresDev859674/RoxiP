; ==============================================================================
; RoxiP Bootloader v0.1 - Interactive Menu, Purple Logo & 10s Auto-Boot
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

    ; 2. Render Starfield Galaxy Background on VRAM (0xB800)
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

    ; 3. Render Purple/Magenta whatweus Logo (Attribute 0x0D)
    mov ah, 0x0D
    mov di, (80 * 2 + 37) * 2
    mov si, logo_l1
    call print_str

    mov di, (80 * 3 + 37) * 2
    mov si, logo_l2
    call print_str

    mov di, (80 * 4 + 37) * 2
    mov si, logo_l3
    call print_str

    mov di, (80 * 5 + 37) * 2
    mov si, logo_l4
    call print_str

    mov di, (80 * 6 + 37) * 2
    mov si, logo_l5
    call print_str

    mov di, (80 * 7 + 37) * 2
    mov si, logo_l6
    call print_str

    ; 4. Render Title Banner below Logo
    mov ah, 0x0E            ; Yellow text attribute
    mov di, (80 * 9 + 25) * 2
    mov si, boot_msg
    call print_str

    ; 5. Render Menu Options
    mov ah, 0x0F            ; Bright White
    mov di, (80 * 12 + 31) * 2
    mov si, opt1_msg
    call print_str

    mov ah, 0x07            ; Light Grey
    mov di, (80 * 14 + 31) * 2
    mov si, opt2_msg
    call print_str

    ; 6. Interactive Boot Menu & 10-Second Auto-Boot Loop
    mov byte [countdown], 10

menu_timer_loop:
    mov ah, 0x0B            ; Light Cyan
    mov di, (80 * 17 + 21) * 2
    mov si, timer_prefix
    call print_str

    ; Print remaining seconds digit
    mov al, [countdown]
    cmp al, 10
    jne .single_digit
    mov word [es:di], 0x0B31   ; '1'
    mov word [es:di+2], 0x0B30 ; '0'
    add di, 4
    jmp .print_suffix

.single_digit:
    add al, '0'
    mov ah, 0x0B
    mov [es:di], ax
    add di, 2
    mov word [es:di], 0x0B20   ; Clear trailing char space
    add di, 2

.print_suffix:
    mov si, timer_suffix
    call print_str

    cmp byte [countdown], 0
    je load_kernel

    ; Wait ~1 second (~18 BIOS ticks) while polling keyboard
    mov ah, 0x00
    int 0x1A                ; Get current BIOS ticks (DX)
    mov bx, dx
    add bx, 18              ; Target = ticks + 18 (~1.0 sec)

.wait_second:
    mov ah, 0x01
    int 0x16                ; Check keypress (non-blocking)
    jnz .key_pressed

    mov ah, 0x00
    int 0x1A
    cmp dx, bx
    jb .wait_second

    dec byte [countdown]
    jmp menu_timer_loop

.key_pressed:
    mov ah, 0x00
    int 0x16                ; Read ASCII character into AL
    cmp al, '2'
    je do_reboot
    jmp load_kernel         ; ENTER, '1' or any key boots kernel

do_reboot:
    int 0x19                ; BIOS Reboot Interrupt

print_str:
    lodsb
    or al, al
    jz .done
    mov [es:di], ax
    add di, 2
    jmp print_str
.done:
    ret

; 7. Load Kernel Sectors with Segment Normalization
load_kernel:
    mov ax, 0xb800
    mov es, ax
    mov word [es:0], 0x2F52 ; 'R'
    mov word [es:2], 0x2F31 ; '1'

    mov ax, 0x1000          ; Load Kernel at Physical Address 0x10000
    mov es, ax
    xor bx, bx

    mov ch, 0               ; Cylinder 0
    mov dh, 0               ; Head 0
    mov cl, 2               ; Sector 2
    mov di, 60              ; Read 60 sectors (~30 KB)

.read_sector_loop:
    mov si, 5               ; 5 retries

.retry:
    push cx
    push dx
    push di

    mov ax, 0x0201          ; AH=02 (Read), AL=01 (1 sector)
    mov dl, [boot_drive]
    int 0x13
    jnc .read_ok

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

    mov ax, es
    add ax, 0x0020          ; Advance segment by 512 bytes
    mov es, ax

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

    mov ax, 0xb800
    mov es, ax
    mov word [es:4], 0x2F52 ; 'R'
    mov word [es:6], 0x2F32 ; '2'

    ; 8. Enable A20 Gate
    in al, 0x92
    or al, 2
    out 0x92, al

    ; 9. Switch to 32-bit Protected Mode
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:init_pm_32

disk_error:
    mov ax, 0xb800
    mov es, ax
    mov si, err_msg
    mov di, (80 * 22 + 22) * 2
    mov ah, 0x0C
.print_err:
    lodsb
    or al, al
    jz .halt
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

    jmp 0x08:0x10000

; ------------------------------------------------------------------------------
; Global Descriptor Table (GDT)
; ------------------------------------------------------------------------------
align 8
gdt_start:
    dq 0x0

gdt_code:
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00

gdt_data:
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; ------------------------------------------------------------------------------
; Data Section & Strings
; ------------------------------------------------------------------------------
boot_drive   db 0
countdown    db 10

logo_l1      db "  /", '\', "  ", 0
logo_l2      db " /  ", '\', " ", 0
logo_l3      db "/ /", '\', " ", '\', 0
logo_l4      db '\', " ", '\', "/ /", 0
logo_l5      db " ", '\', "  / ", 0
logo_l6      db "  ", '\', "/  ", 0

boot_msg     db "=== RoxiP Bootloader v1 ===", 0
opt1_msg     db "[1] Boot RoxiP OS", 0
opt2_msg     db "[2] Reboot System", 0
timer_prefix db "Auto-booting in ", 0
timer_suffix db "s... (ENTER/1=Boot, 2=Reboot)", 0
err_msg      db "Bootloader Error: Disk Read Failed!", 0

times 510 - ($ - $$) db 0
dw 0xAA55
