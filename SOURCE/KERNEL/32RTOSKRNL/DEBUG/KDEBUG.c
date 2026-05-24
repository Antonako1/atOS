#include <DEBUG/KDEBUG.h>
#include <STD/ASM.h>
#include <DRIVERS/SERIAL/SERIAL.h>

#define COM1_BASE 0x3F8
#define DBG_PORT  0xE9

#ifdef p4pwr
VOID KDEBUG_INIT(VOID){}
VOID KDEBUG_PUTC(U8 c){}
VOID KDEBUG_PUTSN(const U8 *s, U32 len){}
VOID KDEBUG_HEX32(U32 v){}
VOID KDEBUG_PUTS(const U8 *s){}

#else
#ifdef p4pwr
#define DBG_OUT(c) ((void)(c))
#else
#define DBG_OUT(c) _outb(DBG_PORT, (c))
#endif

static inline U8   com1_lsr(void)   { return _inb(COM1_BASE + 5); }
static inline void com1_out(U8 val) { _outb(COM1_BASE + 0, val); }
static inline U8   hex_nibble(U8 v) { v &= 0xF; return (v < 10) ? ('0' + v) : ('A' + (v - 10)); }

VOID KDEBUG_INIT(VOID) {
    _outb(COM1_BASE + 1, 0x00);    // disable interrupts
    _outb(COM1_BASE + 3, 0x80);    // enable DLAB
    _outb(COM1_BASE + 0, 0x01);    // divisor low (115200 baud)
    _outb(COM1_BASE + 1, 0x00);    // divisor high
    _outb(COM1_BASE + 3, 0x03);    // 8N1
    _outb(COM1_BASE + 2, 0xC7);    // enable FIFO, clear, 14-byte threshold
    _outb(COM1_BASE + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

VOID KDEBUG_PUTC(U8 c) {
    DBG_OUT(c);
    while ((com1_lsr() & 0x20) == 0) { }
    com1_out(c);
    
}

VOID KDEBUG_PUTS(const U8 *s) {
    if (!s) return;
    while (*s) {
        U8 c = *s++;
        if (c == '\n') { DBG_OUT('\r'); while ((com1_lsr() & 0x20) == 0) { } com1_out('\r'); }
        DBG_OUT(c);
        while ((com1_lsr() & 0x20) == 0) { }
        com1_out(c);
    }
}

VOID KDEBUG_PUTSN(const U8 *s, U32 len) {
    if (!s) return;
    for (U32 i = 0; i < len && s[i]; i++) {
        U8 c = s[i];
        if (c == '\n') { DBG_OUT('\r'); while ((com1_lsr() & 0x20) == 0) { } com1_out('\r'); }
        DBG_OUT(c);
        while ((com1_lsr() & 0x20) == 0) { }
        com1_out(c);
    }
}

VOID KDEBUG_HEX32(U32 v) {
    DBG_OUT('0');
    DBG_OUT('x');
    for (int i = 7; i >= 0; --i) {
        DBG_OUT(hex_nibble((v >> (i * 4)) & 0xF));
    }
}
#endif