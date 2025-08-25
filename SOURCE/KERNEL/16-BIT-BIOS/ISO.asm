; SOURCE/KERNEL/16-BIT-BIOS/ISO.asm - ISO9660 filesystem functions
;      
; Licensed under the MIT License. See LICENSE file in the project root for full license information.
;
; DESCRIPTION
;     This file contains functions for handling ISO9660 filesystem.
; 
; AUTHORS
;     Antonako1
; 
; REVISION HISTORY
;     2025/05/10 - Antonako1
;         Initial version.
; 
; REMARKS
;     None
[BITS 16]
%ifndef BIOS_ISO
%define BIOS_ISO

%include "SOURCE/KERNEL/16-BIT-BIOS/DATA.inc"
%include "SOURCE/KERNEL/16-BIT-BIOS/BIOS.inc"


; eax CALCULATE_KRNL_SEGMENT(ebx amount of sectors, ecx sector size, edx buffer_value)
;
; Description:
;     This function calculates the offset for the kernel segment.
;
; Parameters:
;     ebx - amount of sectors
;     ecx - sector size
;     edx - value of the buffer segment
;
; Return:
;     eax - segment of the first free memory after the E820h memory map
;     cf - 0 if there is free memory, 1 if there is no free memory.
CALCULATE_KRNL_SEGMENT:
    xor eax, eax ; Clear eax
    ; eax = edx + ebx * ecx
    mov eax, edx ; eax = edx
    mul ebx ; eax = eax * ebx
    ; eax = eax * ebx

    add eax, edx ; eax = eax + edx
    ; eax = edx + ebx * ecx

    ; Check if eax is less than 0x0000
    cmp eax, 0x0000 ; Compare eax with 0x0000
    jz .no_memory ; If eax is 0, there is no free memory
    ret ; Return eax
.no_memory:
    stc ; Set carry flag
    ret
    

; eax READ_DISK(eax LBA, cx sectors, dx:bx buffer)
;
; Descrition:
; Reads sectors from the disk into the buffer.
; The function will return 0 if the read was successful, or an error code if it failed.
;
; Parameters:
; eax - LBA of the first sector to read
; cx - number of sectors to read
; bx - Buffer offset
; dx - Buffer segment
;
; Return:
; eax - 0 if the read was successful, or an error code if it failed.
READ_DISK:
    pusha
    mov si, DAP
    mov byte [si], 0x10      ; Drive number
    mov byte [si+1], 0       ; Reserved
    mov word [si+2], cx      ; Number of sectors to read
    mov word [si+4], bx      ; Buffer offset
    mov word [si+6], dx      ; Buffer segment
    mov dword [si+8], eax    ; LBA of the first sector to read

    mov byte [retry_count], RETRY_COUNT_MAX ; Max retries
.try:
    mov dl, [drive_number]    ; Drive number
    mov ah, 0x42              ; Extended Read
    int 0x13                  ; Call BIOS
    jc .error                 ; Jump if carry flag set (error)

    popa
    xor eax, eax              ; Success
    ret

.error:
    dec byte [retry_count]
    jnz .try                  ; Retry if retries remain

    popa
    mov eax, 0xFFFF           ; Failure
    ret



; eax READ_DISK_W(eax = LBA, cx = sectors, ebx = linear address)
; Returns:
;     eax = 0 on success, 0xFFFF on error

; READ_DISK_W:
;     pusha

;     ; Prepare Disk Address Packet (DAP)
;     mov si, DAP
;     mov byte [si], 0x10            ; Size of DAP (must be 0x10, not "drive number"!)
;     mov byte [si+1], 0             ; Reserved
;     mov word [si+2], cx            ; Number of sectors to read
;     mov dword [si+4], ebx          ; 32-bit buffer pointer (flat memory address)
;     mov dword [si+8], eax          ; 64-bit LBA - lower 32 bits
;     mov dword [si+12], 0           ; LBA high 32 bits = 0 (assuming LBA < 2TB)

;     ; Read via BIOS
;     mov dl, [drive_number]         ; BIOS drive number (e.g. 0x80 for HDD)
;     mov ah, 0x42                   ; Extended Read
;     int 0x13
;     jc .error                      ; Carry flag set = error

;     popa
;     xor eax, eax                   ; Return 0 on success
;     ret

; .error:
;     popa
;     mov eax, 0xFFFF                ; Return error code
;     ret


; eax READ_DISK_SECTORS(ax extent_length)
;
; Description:
;     This function calculates the number of sectors in the extent.
;
; Parameters:
;     ax - extent length
;
; Return:
;   ax - number of sectors in the extent
; CALCULATE_SECTORS:
;     add ax, 511
;     shr ax, 9
;     ret
%endif ; BIOS_ISO