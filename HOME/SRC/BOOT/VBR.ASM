; SOURCE\BOOTLOADER\BOOTLOADER.asm - Bootloader for the OS
;      
; Licensed under the MIT License. See LICENSE file in the project root for full license information.
;
; DESCRIPTION
;     Bootloader for the OS. This is the first code that runs when the computer starts
; 
; AUTHORS
;     Antonako1
; 
; REVISION HISTORY
;     2025/02/06 - Antonako1
;         FAT16 Boot Sector Header and welcome message
;     2025/02/09 - Antonako1
;         Removed FAT16 Boot sector and replaced it with ISO9660 header
;     2025/05/9 - Antonako1
;         Kernel loads
;     2025/05/10 - Antonako1
;         Bootloader jumps to kernel entry point
; REMARKS
;     None
.use16
.org 0x7C00
.code

#define SECTOR_SIZE             2048        ; 2KB per sector
#define VOL_DESC_START          16          ; Volume descriptor starts at sector 16

#define PVD_OFFSET              VOL_DESC_START*SECTOR_SIZE/SECTOR_SIZE

#define ROOT_DIRECTORY_OFFSET   0x9C        ; Root directory offset in the ISO9660 image
#define ROOT_DIRECTORY_SIZE     0x22        ; Size of the root directory in sectors

#define MAX_PATH                256         ; Maximum path length
#define KERNEL_OUTPUT_VAR 0xE800

#define KERNEL_LOAD_SEGMENT 0x0000
#define KERNEL_LOAD_OFFSET  0x2000

#define BUFFER_SEGMENT KERNEL_LOAD_SEGMENT
#define BUFFER_OFFSET  KERNEL_LOAD_OFFSET

#define KERNEL_LEN 12

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [drive_number], dl

    sti

    mov ax, PVD_OFFSET
    push cx
    push dx
    mov cx, 1
    mov dx, BUFFER_SEGMENT
    call READ_DISK
    pop cx
    pop dx

    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    mov al, [si]
    cmp al, 0x01
    jne ISO_ERROR


    ; DEBUG FUNCTION
    ; mov ax, BUFFER_SEGMENT
    ; mov ds, ax
    ; mov si, BUFFER_OFFSET
    ; inc si
    ; mov cx, 5
    ; call bios_print

    ; Read lba and size of root dir
    ; 156: Root directory record
    ; +2 LBA location
    ; +10 Data length
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, 156 + 10       ; Point to extent length
    mov bx, si
    mov ax, [bx+2]
    mov [extentLengthLE+2], ax
    mov ax, [bx]
    mov [extentLengthLE], ax
    ;
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, 156 + 2        ; Point to extent location LBA
    mov bx, si             ; Save SI for use
    mov ax, [bx+2]
    mov [extentLocationLE_LBA+2], ax
    mov ax, [bx]
    mov [extentLocationLE_LBA], ax

    ; For testing root dir LBA
    ; mov ax, [extentLengthLE]
    ; cmp ax, 2048
    ; jne ISO_ERROR

    ; mov ax, [extentLocationLE_LBA]
    ; cmp ax, 29
    ; jne ISO_ERROR



    ; offset_var    Offset in the directory tree
    ; buffer_var    Sector buffer
    ; name_buf      Holds filename. KERNEL_LEN bytes long for "KERNEL.BIN"
    ; record is at (buffer+offset)
    ;     uint8_t length;                               (buffer+offset+0). Record length
    ;     uint32_t extentLocationLE_LBA                 (buffer+offset+2). LBA of data
    ;     uint32_t extentLengthLE;                      (buffer+offset+10) Length of data
    ;     fileFlag fileFlags;                           (buffer+offset+25). uint8_t. Fileflags. 0b00000010 if directory, 0b00000000 for file
    ;     uint8_t fileNameLength;                       (buffer+offset+32). Length of fileIdentifier
    ;     strD fileIdentifier[1];                       (buffer+offset+33). uint8_t. File name
    ;
    ;
    ;
    ;
    ; TOOLS\ISO9660\ISO9660.c
    ;   read_directory() logic cut down, only goal is to read KERNEL.BIN;1
    ;
    ;1.     read extentLengthLE*extentLocationLE_LBA into buffer_var. Output should be 0xE800, 59392
    ; AB * CD = (A*C << 32) + ((B*C + A*D) << 16) + B*D
    ; (A_lo*B_lo << 32) + ((A_hi*B_lo + A_lo*B_hi) << 16) + A_hi + B_hi
    ; Skip this mess for now since value fixed "0xE800" is under 0xFFFF
    ; mov ax, [extentLocationLE_LBA]      ; A_lo
    ; mov cx, [extentLengthLE]            ; B_lo
    ; mul cx
    ; cmp ax, KERNEL_OUTPUT_VAR
    ; jne ISO_ERROR
    ; mov [buffer_var], ax

    ; seek to new lba
    mov ax, [extentLocationLE_LBA]
    push cx
    push dx
    mov cx, 1
    mov dx, BUFFER_SEGMENT
    call READ_DISK
    pop cx
    pop dx
    mov ax, 0
    mov [offset_var], ax

    ;2.     while(offset_var < extentLengthLE)
    @@main_loop_start:
        mov ax, [offset_var]
        mov bx, [extentLengthLE]
        cmp ax, bx
        jge @@main_loop_end
        
        ;3.     if ((record)(buffer_var+offset_var).length == 0) jmp ISO_ERROR
        mov ax, BUFFER_SEGMENT
        mov ds, ax
        mov si, BUFFER_OFFSET
        add si, [offset_var]
        mov al, byte [si]
        cmp al, 0
        je ISO_ERROR

        ; Skip directories
        ;5.     if(!((record)(buffer_var+offset_var).fileFlags & 0b00000010)) 
        mov ax, BUFFER_SEGMENT
        mov ds, ax
        mov si, BUFFER_OFFSET
        add si, [offset_var]
        add si, 25
        mov al, byte [si]
        and al, 0b00000010
        jne @@to_loop_increment
        
        ;4.     strncpy(name_buf, record->fileIdentifier, record->fileNameLength);
        ; 
        mov ax, BUFFER_SEGMENT
        mov ds, ax
        mov si, BUFFER_OFFSET

        add si, [offset_var]
        add si, 32                  ; skip to filename length
        xor cx, cx
        mov cl, byte [si]           ; string length
        inc si                      ; Skip length byte
        lea di, name_buf            ; address of name buffer
        call strncopy

        ;6. if(strcmp(KERNEL_FILENAME, name_buf) == 0)
        lea si, KERNEL_FILENAME
        lea di, name_buf
        mov cx, KERNEL_LEN

        ; pusha
        ; call bios_print
        ; mov si, di
        ; call bios_print
        ; popa
        
        call strncmp
        cmp ax, 1
        jne @@to_loop_increment
        ;             KERNEL.BIN record found
        ;             Load to memory and jump to it by using:
        ;               extentLocationLE_LBA
        ;               extentLengthLE

        mov ax, BUFFER_SEGMENT
        mov ds, ax
        mov si, BUFFER_OFFSET
        add si, [offset_var]
        add si, 2
        mov bx, [si]
        mov cx, [si+2]
        mov [extentLocationLE_LBA], bx
        mov [extentLocationLE_LBA+2], cx
        jmp @@main_loop_end
        ; Continues in bootend

        ;11.    If nothing happens: offset_var += (record)(buffer_var+offset_var).length
@@to_loop_increment:
        mov ax, BUFFER_SEGMENT
        mov ds, ax
        mov si, BUFFER_OFFSET
        xor ax, ax
        add si, [offset_var]
        mov al, byte [si]   ; .length

        add ax, [offset_var]
        mov [offset_var], ax
        
        jmp @@main_loop_start
    @@main_loop_end:
    jmp bootend
bootend:

    ; extentLocationLE_LBA contains lba kernel
    ; extentLengthLE contains data length for root
    ; fseek(iso, record->extentLocationLE_LBA * extentLengthLE, SEEK_SET);

    ; Compute number of sectors
    mov ax, [extentLengthLE]
    add ax, 511
    shr ax, 9
    mov cx, ax        ; sector count

    ; Load kernel
    mov ax, [extentLocationLE_LBA]  ; Load LBA into ax
    mov dx, KERNEL_LOAD_SEGMENT                  ; Set segment register
    call READ_DISK                  ; Read the kernel into memory

    mov dl, [drive_number]
    mov ax, KERNEL_LOAD_OFFSET
    mov es, ax
    mov bx, ax
    mov dl, [drive_number]
    mov ax, 0xb800
    mov es, ax
    ; mov word [es:0x0000], 0x1A<<8 | 'A'

    jmp KERNEL_LOAD_SEGMENT:KERNEL_LOAD_OFFSET    ; Jump to kernel


disk_error:
    mov si, diskErrorMessage
    call print_string
    jmp $
ISO_ERROR:
    mov si, ISOErrorMessage
    call print_string
    jmp $
    
; Copies a string from source to destination with a given maximum length
; Inputs:
;   DS:SI - pointer to the source string
;   ES:DI - pointer to the destination buffer
;   CX    - maximum number of characters to copy
strncopy:
    pusha                   ; Save all general-purpose registers
@@copy_loop:
    cmp cx, 0               ; Check if we've reached the maximum length
    je @@done                ; Exit if CX is 0
    lodsb                   ; Load byte at [SI] into AL and increment SI
    stosb                   ; Store byte in AL to [DI] and increment DI
    or al, al               ; Check if the byte is null (end of string)
    jz @@done                ; Exit if null terminator is found
    dec cx                  ; Decrement remaining character count
    jmp @@copy_loop          ; Repeat the loop
@@done:
    popa                    ; Restore all general-purpose registers
    ret

; Compares two strings up to a given length
; Inputs:
;   DS:SI - pointer to the first string (SOURCE1)
;   ES:DI - pointer to the second string (SOURCE2)
;   CX    - maximum number of characters to compare
; Outputs:
;   AX = 1 if strings are equal up to CX characters
;   AX = 0 if strings are not equal
strncmp:
@@cmp_loop:
    cmp cx, 0               ; Check if we've reached the maximum length
    je @@equal               ; If CX is 0, strings are equal up to this point
    lodsb                   ; Load byte from [SI] into AL and increment SI
    scasb                   ; Compare AL with byte at [DI] and increment DI
    jne @@not_equal          ; If bytes are not equal, jump to not_equal
    or al, al               ; Check if the byte is null (end of string)
    jz @@equal               ; If null terminator is found, strings are equal
    dec cx                  ; Decrement remaining character count
    jmp @@cmp_loop           ; Continue comparing the next characters
@@not_equal:
    xor ax, ax              ; Set AX = 0 (not equal)
    jmp @@done
@@equal:
    mov ax, 1               ; Set AX = 1 (equal)
@@done:
    ret

print_string:
    lodsb
    or al, al
    jz @@done
    mov ah, 0x0E
    int 0x10
    jmp print_string
@@done:
    ret

; ax: LBA_LOW
; cx: sector count
; dx: buffer_segment
READ_DISK:
    ; Set up the Disk Address Packet (DAP)
    mov si, dap
    mov byte [si], 0x10                 ; Size of DAP
    mov byte [si+1], 0                  ; Reserved
    mov word [si+2], cx                 ; Number of sectors to read
    mov word [si+4], BUFFER_OFFSET      ; Offset
    mov word [si+6], dx                 ; Segment

    ; DWORD LBA divided into 2 words. No LBA larger than 0xFFFF will ever be used in 16-bit mode
    mov word [si+8], ax                 ; 2-byte LBA
    mov word [si+10], 0                 

    mov ah, 0x42
    mov dl, [drive_number]              ; Drive number
    int 0x13
    jc disk_error
    ret



diskErrorMessage    db "diskERROR", 0
ISOErrorMessage     db "isoERROR", 0
KERNEL_FILENAME     db "KERNEL.BIN;1", KERNEL_LEN
result0             dw 0
result1             dw 0
drive_number        db 0

extentLocationLE_LBA    dd 0    ; lba. contains either root or kernel lba
extentLengthLE          dd 0    ; size of lba, ROOT dir
offset_var    dd 0     ; read_dir var. Offset in PVD
buffer_var    dd 0     ; read_dir var. Directory buffer in PVD
name_buf     db 12 ; Name buffer for filenames. KERNEL.BIN length is set here

; Disk Address Packet (DAP)
dap db 0
.times 16-1 db 0

; Align boot sector
; .times ($-$$)-510 db 0
dw 0xAA55
