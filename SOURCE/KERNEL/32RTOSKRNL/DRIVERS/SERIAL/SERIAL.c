#include <SERIAL/SERIAL.h>
#include <DRIVER_INC.h>
#include <DEBUG/KDEBUG.h>

#define COM1_PORT 0x3F8
#define COM2_PORT 0x2F8
#define COM3_PORT 0x3E8
#define COM4_PORT 0x2E8

VOID SERIAL_PORT_INIT(U16 port) {
    _outb(port + 1, 0x00);    // Disable interrupts
    _outb(port + 3, 0x80);    // Enable DLAB
    _outb(port + 0, 0x03);    // Divisor low byte (38400 baud)
    _outb(port + 1, 0x00);    // Divisor high byte
    _outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
    _outb(port + 2, 0xC7);    // FIFO enabled
    _outb(port + 4, 0x0B);    // Modem control
}

VOID SERIAL_INIT() {
    SERIAL_PORT_INIT(COM1_PORT);
    KDEBUG_PUTS("[SERIAL] COM1 initialized\n");
    SERIAL_PORT_INIT(COM2_PORT);
    KDEBUG_PUTS("[SERIAL] COM2 initialized\n");
    SERIAL_PORT_INIT(COM3_PORT);
    KDEBUG_PUTS("[SERIAL] COM3 initialized\n");
    SERIAL_PORT_INIT(COM4_PORT);
    KDEBUG_PUTS("[SERIAL] COM4 initialized\n");
}

I32 IS_SERIAL_TRANSIT_EMPTY(U16 port) {
    return _inb(port + 5) & 0x20;
}

VOID SERIAL_WRITE_BYTE(U16 port, U8 data) {
    U32 poll = 0;
    while(!IS_SERIAL_TRANSIT_EMPTY(port) && poll < 1000000) {
        poll++;
    }
    if(poll == 1000000) {
        KDEBUG_STR_HEX_LN("[SERIAL] Warning: Serial port 0x", port);
        KDEBUG_PUTS(" is not responding. Data may be lost.\n");
    }
    _outb(port, data);
}

VOID SERIAL_WRITE_DATA(U16 port, PU8 data, U32 len) {
    for(U32 i = 0; i < len; i++) {
        SERIAL_WRITE_BYTE(port, data[i]);
    }
}


VOID SERIAL_READ_BYTE(U16 port, PU8 data) {
    // Wait for Data Ready (bit 0 of LSR)
    while (!(_inb(port + 5) & 0x01));
    *data = _inb(port); // Read from Receiver Buffer Register
}

VOID SERIAL_READ_STRING(U16 port, PU8 buffer, U32 max_len) {
    U32 i = 0;
    U8 c = 0;
    if (max_len == 0) return;

    while (i < max_len - 1) {
        SERIAL_READ_BYTE(port, &c);
        buffer[i++] = c;
        if (c == '\n' || c == '\r') break; // Stop at newline
    }
    buffer[i] = 0; // Null-terminate string
}

VOID SERIAL_READ_BUFFER(U16 port, PU8 buffer, U32 max_len) {
    U32 i = 0;
    while (i < max_len) {
        SERIAL_READ_BYTE(port, &buffer[i]);
        i++;
    }
}

PU8 SERIAL_READ_WHOLE_BUFFER_HEAP(U16 port, U32* out_len)
{
    U32 capacity = 64;
    U32 length = 0;

    PU8 buffer = KMALLOC(capacity);
    if (!buffer) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    U32 idle = 0;
    const U32 IDLE_LIMIT = 200000; // tuning value

    while (idle < IDLE_LIMIT)
    {
        if (_inb(port + 5) & 0x01) // data ready
        {
            idle = 0; // reset timeout

            if (length >= capacity) {
                PU8 tmp = KREALLOC(buffer, capacity * 2);
                if (!tmp) break;
                buffer = tmp;
                capacity *= 2;
            }

            buffer[length++] = _inb(port);
        }
        else
        {
            idle++;
        }
    }

    if (out_len) *out_len = length;
    return buffer;
}

PU8 SERIAL_READ_WHOLE_STRING_HEAP(U16 port, U32* out_len) {
    U32 capacity = 16;
    U32 length = 0;
    PU8 buffer = (PU8) KCALLOC(capacity, sizeof(U8)); // zero-initialized
    if (!buffer) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    while (_inb(port + 5) & 0x01) {
        if (length >= capacity - 1) { // leave space for null
            PU8 tmp = (PU8) KREALLOC(buffer, capacity * 2);
            if (!tmp) break;
            buffer = tmp;
            capacity *= 2;
        }
        buffer[length++] = _inb(port);
    }

    buffer[length] = 0; // Null-terminate
    if (out_len) *out_len = length;
    return buffer;
}