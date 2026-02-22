#ifndef KDEBUG_H
#define KDEBUG_H

#include <STD/TYPEDEF.h>

// Minimal kernel debug output to Bochs/QEMU debug port (I/O 0xE9).
// Use with QEMU options: -debugcon file:OUTPUT/DEBUG/debug.log -global isa-debugcon.iobase=0xe9

VOID KDEBUG_PUTC(U8 c);
VOID KDEBUG_PUTS(const U8 *s);
VOID KDEBUG_PUTSN(const U8 *s, U32 len);
VOID KDEBUG_HEX32(U32 v);
VOID KDEBUG_INIT(VOID);

#define KDEBUG_STR_HEX_LN(str, x) KDEBUG_PUTS(str); KDEBUG_HEX32(x); KDEBUG_PUTC('\n')
#define KDEBUG_HEX32LN(x) KDEBUG_HEX32(x); KDEBUG_PUTC('\n')
#endif // KDEBUG_H
