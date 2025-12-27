#ifndef STD_ASM_H
#define STD_ASM_H

#include "./TYPEDEF.h"

#define ASM_VOLATILE(...) \
    __asm__ __volatile__(__VA_ARGS__)
#define HLT for(;;) ASM_VOLATILE("cli; hlt")

#define LN "\n\t"
#define ASL(line) line LN

#define HALT ASM_VOLATILE("hlt")
#define NOP ASM_VOLATILE("nop")
#define CLI ASM_VOLATILE("cli")
#define STI ASM_VOLATILE("sti")
#define YIELD() ASM_VOLATILE("int $0x81")

/* 
 * Port I/O macros (legacy, kept for compatibility)
 */


// Single, canonical helpers (drop the macro variants)

static inline void _io_wait(void) {
    unsigned char dummy;
    asm volatile ("inb $0x80, %0"
                  : "=a"(dummy)
                  : 
                  : "memory");
}

static inline unsigned char _inb(unsigned short port) {
    unsigned char v;
    asm volatile ("inb %1, %0"
                  : "=a"(v)
                  : "Nd"(port)
                  : "memory");
    return v;
}

static inline void _outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1"
                  :
                  : "a"(val), "Nd"(port)
                  : "memory");
}

static inline unsigned short _inw(unsigned short port) {
    unsigned short v;
    asm volatile ("inw %1, %0"
                  : "=a"(v)
                  : "Nd"(port)
                  : "memory");
    return v;
}

static inline void _outw(unsigned short port, unsigned short val) {
    asm volatile ("outw %0, %1"
                  :
                  : "a"(val), "Nd"(port)
                  : "memory");
}

static inline unsigned int _inl(unsigned short port) {
    unsigned int v;
    asm volatile ("inl %1, %0"
                  : "=a"(v)
                  : "Nd"(port)
                  : "memory");
    return v;
}

static inline void _outl(unsigned short port, unsigned int val) {
    asm volatile ("outl %0, %1"
                  :
                  : "a"(val), "Nd"(port)
                  : "memory");
}




#ifndef KERNEL_ENTRY
static inline void cpu_relax(void) {
    __asm__ volatile ("pause");
}

static inline void mfence(void) {
    __asm__ volatile("mfence" ::: "memory");
}

static inline void NMI_enable() {
    _outb(0x70, _inb(0x70) & 0x7F);
    _inb(0x71);
}

static inline void NMI_disable() {
    _outb(0x70, _inb(0x70) | 0x80);
    _inb(0x71);
}

static inline VOID __delay(U32 ms) {
    for (volatile U32 i = 0; i < ms * 1000; i++) {
        NOP;
    }
}

static inline VOID yield() {
    // __asm__ volatile("int $0x20");
}

#endif // KERNEL_ENRTY
// Write `count` 16-bit words from buffer to port
static inline void _outsw(unsigned short port, const void *buffer, unsigned int count) {
    asm volatile ("cld; rep outsw"
                  : "+S"(buffer), "+c"(count)
                  : "d"(port)
                  : "memory");
}
// Read `count` 16-bit words from port into buffer
static inline void _insw(unsigned short port, void *buffer, unsigned int count) {
    asm volatile (
                 "cld; rep insw"
                  : "+D"(buffer), "+c"(count)
                  : "d"(port)
                  : "memory");
}
#endif // STD_ASM_H
