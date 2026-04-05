.use32
#define ORIGIN 0x10000000
.org ORIGIN

start:
    mov eax, 59
    int 0x80
    jmp ORIGIN