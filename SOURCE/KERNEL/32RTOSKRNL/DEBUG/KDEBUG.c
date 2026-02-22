#include <DEBUG/KDEBUG.h>
#include <STD/ASM.h>

#define DBG_PORT 0xE9
#define COM1_BASE 0x3F8

static inline U8 com1_lsr(void) { return _inb(COM1_BASE + 5); }
static inline void com1_out(U8 val) { _outb(COM1_BASE + 0, val); }

VOID KDEBUG_INIT(VOID) {
    // Initialize COM1: 115200 8N1, enable FIFO
    _outb(COM1_BASE + 1, 0x00);    // disable interrupts
    _outb(COM1_BASE + 3, 0x80);    // enable DLAB
    _outb(COM1_BASE + 0, 0x01);    // divisor low (115200 baud)
    _outb(COM1_BASE + 1, 0x00);    // divisor high
    _outb(COM1_BASE + 3, 0x03);    // 8 bits, no parity, one stop
    _outb(COM1_BASE + 2, 0xC7);    // enable FIFO, clear, 14-byte threshold
    _outb(COM1_BASE + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

VOID KDEBUG_PUTC(U8 c) {
    _outb(DBG_PORT, c);
    // Also write to COM1 if THR empty
    while ((com1_lsr() & 0x20) == 0) { /* wait */ }
    com1_out(c);
}

VOID KDEBUG_PUTS(const U8 *s) {
    if (!s) return;
    while (*s) {
        if (*s == '\n') {
            _outb(DBG_PORT, '\r');
        }
        _outb(DBG_PORT, *s++);
        // mirror to COM1
        U8 c = *(s-1);
        if (c == '\n') {
            while ((com1_lsr() & 0x20) == 0) { }
            com1_out('\r');
        }
        while ((com1_lsr() & 0x20) == 0) { }
        com1_out(c);
    }
}

static inline U8 hex_nibble(U8 v) {
    v &= 0xF;
    return (v < 10) ? ('0' + v) : ('A' + (v - 10));
}

VOID KDEBUG_HEX32(U32 v) {
    _outb(DBG_PORT, '0');
    _outb(DBG_PORT, 'x');
    for (int i = 7; i >= 0; --i) {
        U8 nib = (v >> (i * 4)) & 0xF;
        _outb(DBG_PORT, hex_nibble(nib));
    }
}

VOID KDEBUG_PUTSN(const U8 *s, U32 len) {
    if (!s) return;
    for (U32 i = 0; i < len && s[i]; i++) {
        if (s[i] == '\n') {
            _outb(DBG_PORT, '\r');
        }
        _outb(DBG_PORT, s[i]);
        // mirror to COM1
        U8 c = s[i];
        if (c == '\n') {
            while ((com1_lsr() & 0x20) == 0) { }
            com1_out('\r');
        }
        while ((com1_lsr() & 0x20) == 0) { }
        com1_out(c);
    }
}