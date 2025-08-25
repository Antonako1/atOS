; SOURCE/KERNEL/16-BIT-BIOS/IO.asm - BIOS I/O functions
;      
; Licensed under the MIT License. See LICENSE file in the project root for full license information.
;
; DESCRIPTION
;     This file contains BIOS I/O functions for 16-bit mode.
; 
; AUTHORS
;     Antonako1
; 
; REVISION HISTORY
;     2025/05/10 - Antonako1
;         Initial version. 
; 
; REMARKS
;     Functions:
;       PRINTN - Prints a string with given length 
;       PRINT  - Prints a null-terminated string
;       PRINTNLN - Prints a string with given length
;       PRINTLN - Prints a null-terminated string
;       PRINT_LINEFEED - Prints a line break (CR + LF)
;       PRINT_HEX - Prints input as hexadecimal
;       PUTCHAR - Prints a character
;       PRINT__ - Prints a underscore character
;

%ifndef BIOS_IO
%define BIOS_IO

; void PRINTN(DS:SI str, cx len);
; 
; DESCRIPTION
;     Prints a string with given length using BIOS interrupt 0x10
; 
; PARAMETERS
;     DS:SI - pointer to the string
;     CX    - length of the string
;     
; RETURN
;     None
; 
; AUTHORS
;     Antonako1
; 
; REVISION HISTORY
;     2025/05/10 - Antonako1
;         Initial version.
; 

PRINTN:
    pusha                   ; Save all general-purpose registers
.next_char:
    cmp cx, 0               ; Check if we've printed enough characters
    je .done
    lodsb                   ; Load byte at [SI] into AL and increment SI
    mov ah, 0x0E            ; BIOS teletype output function
    int 0x10                ; Call BIOS interrupt to print character in AL
    dec cx                  ; Decrement remaining character count
    jmp .next_char
.done:
    popa                    ; Restore all general-purpose registers
    ret


; void PRINT(DS:SI str);
; 
; DESCRIPTION
;     Prints a null-terminated string using BIOS interrupt 0x10
; 
; PARAMETERS
;     DS:SI - pointer to the string
;     
; RETURN
;     None
; 
; AUTHORS
;     Antonako1
; 
; REVISION HISTORY
;     2025/05/10 - Antonako1
;         Description
; 
PRINT:
    ; save registers
    pusha
print_loop:
    lodsb ; load char to al
    cmp al, 0
    je .print_done
    mov ah, 0x0e
    mov bh, 0
    int 0x10
    jmp print_loop
.print_done:
    popa
    ret  

; void PRINTN_Q(DS:SI str, cx len);
;
;
; DESCRIPTION
;     Prints a string with given length using BIOS interrupt 0x10
;     and adds quotes around it.
;
; PARAMETERS
;     DS:SI - pointer to the string
;     CX    - length of the string
;
; RETURN
;     None
;
; AUTHORS
;     Antonako1
PRINTN_Q:
    pusha                   ; Save all general-purpose registers

    pusha
    mov al, '"'
    mov ah, 0x0E
    int 0x10
    popa 
    
.next_char:
   cmp cx, 0               ; Check if we've printed enough characters
   je .close_quote          ; If CX is 0, jump to print closing quote
   lodsb                   ; Load byte at [SI] into AL and increment SI    
   mov ah, 0x0E            ; BIOS teletype output function
   int 0x10                ; Call BIOS interrupt to print character in AL
   dec cx                  ; Decrement remaining character count
   jmp .next_char          ; Continue printing the next character
   
.close_quote:
    ; Print closing single quote
    mov al, '"'
    mov ah, 0x0E
    int 0x10
    popa                    ; Restore all general-purpose registers
    ret

PRINT_LINEFEED:
    pusha                   ; Save all general-purpose registers
    ; Print line break (CR + LF)
    mov al, 0x0D            ; Carriage return
    mov ah, 0x0E
    int 0x10
    mov al, 0x0A            ; Line feed
    mov ah, 0x0E
    int 0x10
    popa                    ; Restore all general-purpose registers
    ret

; void PRINTNLN(DS:SI str, cx len);
;
; DESCRIPTION
;     Prints a string with given length using BIOS interrupt 0x10
;     and adds \r\n at the end.
;
; PARAMETERS
;     DS:SI - pointer to the string
;     CX    - length of the string
;
; RETURN
;     None
;
; AUTHORS
;     Antonako1
PRINTNLN:
    call PRINTN            ; Call PRINTN to print the string
    call PRINT_LINEFEED ; Call PRINT_LINEFEED to print CR + LF
    ret

; void PRINTLN(DS:SI str);
;
; DESCRIPTION
;     Prints a null-terminated string using BIOS interrupt 0x10
;     and adds \r\n at the end.
;
; PARAMETERS
;     DS:SI - pointer to the string
;
; RETURN
;     None
;
; AUTHORS
;     Antonako1
PRINTLN:
    call PRINT            ; Call PRINTN to print the string
    ; Print line break (CR + LF)
    call PRINT_LINEFEED ; Call PRINT_LINEFEED to print CR + LF
    ret

; void PUTCHAR(AL char);
;
; DESCRIPTION
;     Prints a character
;
; PARAMETERS
;     AL - character to print
;
; RETURN
;     None
PUTCHAR:
    pusha
    mov ah, 0x0E
    int 0x10
    popa
    ret


PRINT__:
    pusha
    mov al, '_'
    call PUTCHAR
    popa
    ret 


;  void PRINT_HEX(AX);
;
; DESCRIPTION
;     Prints the value in AX as hexadecimal using BIOS interrupt 0x21
;     and INT 0x10.
;     The value is printed in 8 digits (32 bits).
; PARAMETERS
;     EAX - value to print
; RETURN
;     None
PRINT_HEX:
    pusha

    push eax
    mov al, '0'
    call PUTCHAR
    mov al, 'X'
    call PUTCHAR
    pop eax

    ; Move AX to a working register so we don't destroy it
    mov cx, 8             ; 4 nibbles (16 bits)
    mov ebx, eax            ; Copy AX into BX

.print_hex_loop:
    rol ebx, 4             ; Rotate left 4 bits to bring next nibble into low nibble
    mov dl, bl            ; Get low 8 bits
    and dl, 0x0F          ; Isolate the lowest nibble
    cmp dl, 9
    jbe .print_digit
    add dl, 7             ; Adjust for 'A'-'F'

.print_digit:
    add dl, '0'
    push eax
    mov al, dl
    call PUTCHAR
    pop eax
    loop .print_hex_loop

    popa
    ret

%endif ; BIOS_IO