; SOURCE/KERNEL/16-BIT-BIOS/STR.asm - String manipulation functions for 16-bit mode
;      
; Licensed under the MIT License. See LICENSE file in the project root for full license information.
;
; DESCRIPTION
;     String manipulation functions for 16-bit mode
; 
; AUTHORS
;     Antonako1
; 
; REVISION HISTORY
;     2025/05/10 - Antonako1
;         Added strncmp and strncopy functions
; 
; REMARKS
;     None
%ifndef BIOS_STR
%define BIOS_STR
; ax strncmp(DS:SI src, ES:DI dest, CX src_length)
; 
; DESCRIPTION
;     Compares two strings up to a given length
; 
; PARAMETERS
;     DS:SI    src - pointer to the first string (SOURCE1)
;     ES:DI    dest - pointer to the second string (SOURCE2)
;     CX       src_length - maximum number of characters to compare
;     
; RETURN
;     ax = 1 if strings are equal up to CX characters. cmp ax, 1 ; je not_equal
;     ax = 0 if strings are not equal. cmp ax, 0 ; je equal
; 
; AUTHORS
;     Antonako1
; 
; REVISION HISTORY
;     2025/05/10 - Antonako1
;         Initial version
; 
; REMARKS
;     None
strncmp:
.cmp_loop:
    cmp cx, 0               ; Check if we've reached the maximum length
    je .equal               ; If CX is 0, strings are equal up to this point
    lodsb                   ; Load byte from [SI] into AL and increment SI
    scasb                   ; Compare AL with byte at [DI] and increment DI
    jne .not_equal          ; If bytes are not equal, jump to not_equal
    or al, al               ; Check if the byte is null (end of string)
    jz .equal               ; If null terminator is found, strings are equal
    dec cx                  ; Decrement remaining character count
    jmp .cmp_loop           ; Continue comparing the next characters
.not_equal:
    xor ax, ax              ; Set AX = 0 (not equal)
    jmp .done
.equal:
    mov ax, 1               ; Set AX = 1 (equal)
.done:
    ret

; void strncpy(DS:SI src, ES:DI dest, CX src_length)
; 
; DESCRIPTION
;     Copies a string from source to destination up to a given length
; 
; PARAMETERS
;     DS:SI    src - pointer to the source string (SOURCE1)
;     ES:DI    dest - pointer to the destination buffer (SOURCE2)
;     CX       src_length - maximum number of characters to copy
; 
; AUTHORS
;     Antonako1
; 
; REVISION HISTORY
;     2025/05/10 - Antonako1
;         Initial version
; 
; REMARKS
;     None
; strncopy:
;     pusha                   ; Save all general-purpose registers
; .copy_loop:
;     cmp cx, 0               ; Check if we've reached the maximum length
;     je .done                ; Exit if CX is 0
;     lodsb                   ; Load byte at [SI] into AL and increment SI
;     stosb                   ; Store byte in AL to [DI] and increment DI
;     or al, al               ; Check if the byte is null (end of string)
;     jz .done                ; Exit if null terminator is found
;     dec cx                  ; Decrement remaining character count
;     jmp .copy_loop          ; Repeat the loop
; .done:
;     popa                    ; Restore all general-purpose registers
;     ret

%endif ; BIOS_STR