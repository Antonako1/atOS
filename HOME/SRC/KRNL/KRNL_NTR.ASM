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


; e820
; find kernel
; vbe

;enter 32-bit
; load gdt
; a20
; pe bit in cr0
; far jump to 32-bit code
.use32

; reset segments
; load idt
; jump to kernel entry point