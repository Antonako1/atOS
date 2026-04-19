.data
    msg db "Hello, World!", 0
    
.code
_start:
    ; mov eax, 59
    ; int 0x80
@@1:
    jmp @@1 