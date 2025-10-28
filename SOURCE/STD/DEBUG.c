#include <STD/DEBUG.h>
#include <STD/ASM.h>

static inline U8 com1_lsr(void) {
    return _inb(COM1_BASE + 5);
}
static inline void com1_out(U8 val) {
    _outb(COM1_BASE + 0, val);
}

void DEBUG_INIT(void) {
    // Initialize COM1: 115200 8N1
    _outb(COM1_BASE + 1, 0x00);    // disable interrupts
    _outb(COM1_BASE + 3, 0x80);    // enable DLAB
    _outb(COM1_BASE + 0, 0x01);    // divisor low (115200)
    _outb(COM1_BASE + 1, 0x00);    // divisor high
    _outb(COM1_BASE + 3, 0x03);    // 8 bits, no parity, one stop
    _outb(COM1_BASE + 2, 0xC7);    // enable FIFO
    _outb(COM1_BASE + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}
void DEBUG_NL() {
    DEBUG_PUTC("\r");
}
void DEBUG_PUTC(U8 c) {
    _outb(DEBUG_PORT, c);
    while ((com1_lsr() & 0x20) == 0) { /* wait */ }
    com1_out(c);
}

void DEBUG_PUTS(U8 *s) {
    if (!s) return;
    while (*s) {
        if (*s == '\n') {
            DEBUG_PUTC('\r');
        }
        DEBUG_PUTC(*s++);
    }
}

void DEBUG_PUTS_LN(PU8 s) {
    if (!s) return;
    while (*s) {
        if (*s == '\n') {
            DEBUG_PUTC('\r');
        }
        DEBUG_PUTC(*s++);
    }
    DEBUG_PUTC('\r');
}

static inline U8 hex_nibble(U8 v) {
    v &= 0xF;
    return (v < 10) ? ('0' + v) : ('A' + (v - 10));
}

void DEBUG_HEX32(U32 value) {
    DEBUG_PUTC('0');
    DEBUG_PUTC('x');
    for (int i = 7; i >= 0; --i) {
        DEBUG_PUTC(hex_nibble((value >> (i * 4)) & 0xF));
    }
}

// Simple memory dump: 16 bytes per line
void MEMORY_DUMP(const void *addr, U32 length) {
    const U8 *p = (const U8 *)addr;
    for (U32 i = 0; i < length; i++) {
        if (i % 16 == 0) {
            DEBUG_PUTC('\n');
            DEBUG_HEX32((U32)(p + i));
            DEBUG_PUTS(": ");
        }
        U8 b = p[i];
        DEBUG_PUTC(hex_nibble(b >> 4));
        DEBUG_PUTC(hex_nibble(b & 0xF));
        DEBUG_PUTC(' ');
    }
    DEBUG_PUTC('\n');
}
