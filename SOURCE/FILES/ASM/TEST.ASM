; Test program for i386 lexer

#include <TEST.INC>

; --- Data Section ---
.data
QBERT     dd 1234        ; define a 32-bit constant

; --- Code Section ---
.text

.CODE
_start:
    MOV EAX, ABC        ; load ABC into EAX
    ADD EAX, 5          ; add immediate
    SUB EBX, EAX        ; subtract
    PUSH EAX            ; push value onto stack
    POP ECX             ; pop value into ECX
    JMP _start          ; infinite loop for testing
