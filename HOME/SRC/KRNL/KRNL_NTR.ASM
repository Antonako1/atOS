.code
.use16
.org 0x2000

cli
mov [drive_number], dl

xor ax, ax
mov ss, ax
mov sp, 0x7C00

; Set up the data segment
mov ax, 0x0000
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax

; colour random cell in video memory to show we're alive
mov ax, 0xA000
mov es, ax
mov di, 0
mov al, 0x0F
stosb

LEA SI, error
call print_str

; e820
jmp $

; find kernel
; vbe

;enter 32-bit
; load gdt
; a20
; pe bit in cr0
; far jump to 32-bit code

print_str:
    lodsb
    or al, al
    jz @@ps_done
    mov ah, 0x0E
    int 0x10
    jmp print_str
@@ps_done:
    ret

error db "ERROR: ", 0

.use32

; reset segments
; load idt
; jump to kernel entry point

drive_number db 0