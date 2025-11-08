.use16
.org 0x7c00 
jmp 0x1000:0x1000

jmp [es:di]

.use32
; Test program for 80386 and 8086 lexer

#include <TEST.INC>

; --- Data Section ---
.data
QBERT     dd 1234        ; define a 32-bit constant
STR       db "STR", 0 ; Automatically make a null terminated string
STRLEN    DD $STR ; Gets length of STR defined above (3+1)

.CODE
_start:
@@1:
    MOV EAX, ABC
    ADD EAX, DWORD PTR [QBERT]
    SUB EBX, EAX 
    PUSH EAX
    POP ECX
    JMP _start
    LEA EAX, [ESI + 100]
    @f
    jmp @@1
test:

    
.times 510-($-$$) db 0
0xAA55
0b00001111