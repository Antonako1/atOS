/*+++
    SOURCE/KERNEL/32RTOSKRNL/DRIVERS/PS2/KEYBOARD.h - PS/2 Keyboard Driver

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    PS/2 Keyboard Driver

AUTHORS
    Antonako1

REVISION HISTORY
    yyyy/mm/dd - Name(s)
        Description

REMARKS
    When compiling, include KEYBOARD.c
    User processes must communicate with the keyboard driver using syscalls.
---*/
#ifndef PS2_KEYBOARD_H
#define PS2_KEYBOARD_H
#include <DRIVERS/PS2/KEYBOARD.h>
#include <STD/ASM.h>
#include <STD/TYPEDEF.h>

/*+++
PS/2 Ports
---*/
#define PS2_DATAPORT 0x60
#define PS2_WRITEPORT 0x60
#define PS2_CMDPORT  0x64

/*+++
PS/2 Status bits
---*/
#define PS2_OUTPUT_BUFFER_FULL  0b00000001 // Data ready
#define PS2_INPUT_BUFFER_FULL   0b00000010 // Controller busy
#define PS2_SYSTEM_FLAG         0b00000100 // System flag
#define PS2_COMMAND_BIT         0b00010000 // Command bit
#define PS2_DATA_BIT            0b00000000 // Data bit

/*+++
PS/2 Keyboard Commands
---*/
#define PS2_SET_LEDS    0xED
#define PS2_ECHO        0xEE
#define PS2_GET_SCANCODE_SET 0xF0
#define PS2_SET_SCANCODE_SET 0xF0
#define PS2_IDENTIFY_KEYBOARD 0xF2
#define PS2_SET_TYPEMATIC_RATE_AND_DELAY 0xF3
#define PS2_ENABLE_SCANNING 0xF4
#define PS2_DISABLE_SCANNING 0xF5
#define PS2_SET_DEF_PARAMETERS 0xF6
#define PS2_SET_ALL_KEYS_TYPEMATIC_AUTOREPEAT 0xF7
#define PS2_SET_ALL_KEYS_MAKE_RELEASE 0xF8
#define PS2_SET_ALL_KEYS_MAKE_ONLY 0xF9
#define PS2_SET_ALL_KEYS_TYPEMATIC_AUTOREPEAT_MAKE_RELEASE 0xFA
#define PS2_SET_SPECIFIC_KEY_TYPEMATIC_AUTORELEASE 0xFB
#define PS2_SET_SPECIFIC_KEY_MAKE_RELEASE_ONLY 0xFC
#define PS2_SET_SPECIFIC_KEY_MAKE_ONLY 0xFD
#define PS2_RESEND_LAST_BYTE 0xFE
#define PS2_RESET_AND_SELF_TEST 0xFF

/*+++
PS/2 Keyboard Special Bytes
---*/
#define PS2_SPECIAL_KEYDETECT_BUFFER_ERROR1 0x00
#define PS2_SPECIAL_SELF_TEST_PASSED 0xAA
#define PS2_SPECIAL_ECHO_RESPONSE 0xEE
#define PS2_SPECIAL_ACK 0xFA // Command acknowledgment
#define PS2_SPECIAL_SELF_TEST_FAILED1 0xFC // After reset or powerup
#define PS2_SPECIAL_SELF_TEST_FAILED2 0xFD // After reset or powerup
#define PS2_SPECIAL_RESEND 0xFE // Repeat last command
#define PS2_SPECIAL_KEYDETECT_BUFFER_ERROR2 0xFF


#define GET_CONTROLLER_CONFIGURATION_BYTE 0x20
#define SET_CONTROLLER_CONFIGURATION_BYTE 0x60
#define ENABLE_SECOND_PS2_PORT 0xA8
#define TEST_FIRST_PS2_PORT 0xAB
#define TEST_SECOND_PS2_PORT 0xA9
#define ENABLE_FIRST_PS2_PORT 0xAE
#define DISABLE_FIRST_PS2_PORT 0xAD
#define DISABLE_SECOND_PS2_PORT 0xA7

typedef struct {
    U16 type; // 0 if unknown, 0xABxx if dual-channel keyboard
    BOOLEAN dual_channel; // TRUE if dual-channel controller
    BOOLEAN exists; // TRUE if controller exists
    U8 port1_check; // Result of test of first PS/2 port
    U8 port2_check; // Result of test of second PS/2 port
    U8 scancode_set; // Current scan code set (1 or 2)
} PS2_INFO;

extern PS2_INFO *PS2_info;

#define CMD_QUEUE_SIZE 8
typedef struct {
    U8 head; // Points to the next write position
    U8 tail; // Points to the last read position
    U8 buffer[CMD_QUEUE_SIZE];
} PS2KB_CMD_QUEUE;

#ifdef __RTOS__
PS2_INFO *GET_PS2_INFO(VOID);

PS2KB_CMD_QUEUE *GET_CMD_QUEUE(VOID);
U0 CLEAR_CMD_QUEUE(VOID);
U0 PUSH_TO_CMD_QUEUE(U8 byte);
U8 POP_FROM_CMD_QUEUE(VOID);
BOOLEAN IS_CMD_QUEUE_EMPTY(VOID);
BOOLEAN IS_CMD_QUEUE_FULL(VOID);


BOOLEAN PS2_KEYBOARD_INIT(VOID);
BOOLEAN PS2_KEYBOARD_RESET(VOID);
BOOLEAN PS2_SET_SCAN_CODE_SET(U8 set);
void PS2_KEYBOARD_HANDLER(I32 num, U32 errcode);
#endif // __RTOS__

/*+++
We only use 2 scan code sets: 1 and 2

2 is the default for modern keyboards, and 1 is a fallback.

Because we only use 2 sets, we can hardcode the key values for each set here.

Third scan code set is not supported (rarely used anyway).

---*/

/*+++
Scan code set 1
---*/
#define SCANCODESET1 0x01
#define SC1(x) SC1_##x

typedef enum {
    SC1(ESC) = 0x01
} SC1;
/*+++
Scan code set 2
---*/
#define SCANCODESET2 0x02
#define SC2_RELEASE_F0 0xF0
#define SC2_RELEASE_E0 0xE0
#define SC2_RELEASE_E0F0 0xE0F0
// Macros to generate enum entries
#define SC2_PRESSED(x, n)       SC2_PRESSED_##x = n
#define SC2_RELEASED(x, n)      SC2_RELEASED_##x = n

#define SC2_2PART_PRESSED(x, n1, n2) \
    SC2_PRESSED_PART1_##x = n1, \
    SC2_PRESSED_PART2_##x = n2
#define SC2_2PART_RELEASED(x, n1, n2) \
    SC2_RELEASED_PART1_##x = n1, \
    SC2_RELEASED_PART2_##x = n2


#define SC2_3PART_RELEASED(x, n1, n2, n3) \
    SC2_RELEASED_PART1_##x = n1, \
    SC2_RELEASED_PART2_##x = n2, \
    SC2_RELEASED_PART3_##x = n3

#define SC2_4PART_PRESSED(x, n1, n2, n3, n4) \
    SC2_PRESSED_PART1_##x = n1, \
    SC2_PRESSED_PART2_##x = n2, \
    SC2_PRESSED_PART3_##x = n3, \
    SC2_PRESSED_PART4_##x = n4
#define SC2_4PART_RELEASED(x, n1, n2, n3, n4) \
    SC2_RELEASED_PART1_##x = n1, \
    SC2_RELEASED_PART2_##x = n2, \
    SC2_RELEASED_PART3_##x = n3, \
    SC2_RELEASED_PART4_##x = n4

#define SC2_7PART_RELEASED(x, n1, n2, n3, n4, n5, n6, n7) \
    SC2_RELEASED_PART1_##x = n1, \
    SC2_RELEASED_PART2_##x = n2, \
    SC2_RELEASED_PART3_##x = n3, \
    SC2_RELEASED_PART4_##x = n4, \
    SC2_RELEASED_PART5_##x = n5, \
    SC2_RELEASED_PART6_##x = n6, \
    SC2_RELEASED_PART7_##x = n7
#define SC2_8PART_PRESSED(x, n1, n2, n3, n4, n5, n6, n7, n8) \
    SC2_PRESSED_PART1_##x = n1, \
    SC2_PRESSED_PART2_##x = n2, \
    SC2_PRESSED_PART3_##x = n3, \
    SC2_PRESSED_PART4_##x = n4, \
    SC2_PRESSED_PART5_##x = n5, \
    SC2_PRESSED_PART6_##x = n6, \
    SC2_PRESSED_PART7_##x = n7, \
    SC2_PRESSED_PART8_##x = n8

#define SC2_PRESSED_EXT( \
    x, \
    n1, \
    n2, \
    n3, \
    n4, \
    n5, \
    n6, \
    n7, \
    n8 \
) \
    SC2_PRESSED_PART1_##x = n1, \
    SC2_PRESSED_PART2_##x = n2, \
    SC2_PRESSED_PART3_##x = n3, \
    SC2_PRESSED_PART4_##x = n4, \
    SC2_PRESSED_PART5_##x = n5, \
    SC2_PRESSED_PART6_##x = n6, \
    SC2_PRESSED_PART7_##x = n7, \
    SC2_PRESSED_PART8_##x = n8
    
#define SC2_RELEASED_EXT( \
    x, \
    n1, \
    n2, \
    n3,\
    n4,\
    n5,\
    n6,\
    n7,\
    n8\
) \
 SC2_RELEASED_PART1_##x = n1, \
 SC2_RELEASED_PART2_##x = n2, \
 SC2_RELEASED_PART3_##x = n3, \
 SC2_RELEASED_PART4_##x = n4, \
 SC2_RELEASED_PART5_##x = n5, \
 SC2_RELEASED_PART6_##x = n6, \
 SC2_RELEASED_PART7_##x = n7, \
 SC2_RELEASED_PART8_##x = n8

typedef enum {
    // Standard pressed keys
    SC2_PRESSED(F9, 0x01), // F9
    SC2_PRESSED(F5, 0x03), // F5
    SC2_PRESSED(F3, 0x04), // F3
    SC2_PRESSED(F1, 0x05), // F1
    SC2_PRESSED(F2, 0x06), // F2
    SC2_PRESSED(F12, 0x07), // F12
    SC2_PRESSED(F10, 0x09), // F10
    SC2_PRESSED(F8, 0x0A), // F8
    SC2_PRESSED(F6, 0x0B), // F6
    SC2_PRESSED(F4, 0x0C), // F4
    SC2_PRESSED(TAB, 0x0D), // TAB
    SC2_PRESSED(GRAVE, 0x0E), // `~
    SC2_PRESSED(LALT, 0x11), // Left Alt
    SC2_PRESSED(LSHIFT, 0x12), // Left Shift
    SC2_PRESSED(LCTRL, 0x14), // Left Ctrl
    SC2_PRESSED(Q, 0x15), // Q
    SC2_PRESSED(1, 0x16), // 1
    SC2_PRESSED(Z, 0x1A), // Z
    SC2_PRESSED(S, 0x1B), // S
    SC2_PRESSED(A, 0x1C), // A
    SC2_PRESSED(W, 0x1D), // W
    SC2_PRESSED(2, 0x1E), // 2
    SC2_PRESSED(C, 0x21), // C
    SC2_PRESSED(X, 0x22), // X
    SC2_PRESSED(D, 0x23), // D
    SC2_PRESSED(E, 0x24), // E
    SC2_PRESSED(4, 0x25), // 4
    SC2_PRESSED(3, 0x26), // 3
    SC2_PRESSED(SPACE, 0x29), // SPACE
    SC2_PRESSED(V, 0x2A), // V
    SC2_PRESSED(F, 0x2B), // F
    SC2_PRESSED(T, 0x2C), // T
    SC2_PRESSED(R, 0x2D), // R
    SC2_PRESSED(5, 0x2E), // 5
    SC2_PRESSED(N, 0x31), // N
    SC2_PRESSED(B, 0x32), // B
    SC2_PRESSED(H, 0x33), // H
    SC2_PRESSED(G, 0x34), // G
    SC2_PRESSED(Y, 0x35), // Y
    SC2_PRESSED(6, 0x36), // 6
    SC2_PRESSED(M, 0x3A), // M
    SC2_PRESSED(J, 0x3B), // J
    SC2_PRESSED(U, 0x3C), // U
    SC2_PRESSED(7, 0x3D), // 7
    SC2_PRESSED(8, 0x3E), // 8
    SC2_PRESSED(COMMA, 0x41), // ,
    SC2_PRESSED(K, 0x42), // K
    SC2_PRESSED(I, 0x43), // I
    SC2_PRESSED(O, 0x44), // O
    SC2_PRESSED(ZERO, 0x45), // 0
    SC2_PRESSED(NINE, 0x46), // 9
    SC2_PRESSED(DOT, 0x49), // .
    SC2_PRESSED(SLASH, 0x4A), // /
    SC2_PRESSED(L, 0x4B), // L
    SC2_PRESSED(SEMICOLON, 0x4C), // ;
    SC2_PRESSED(P, 0x4D), // P
    SC2_PRESSED(MINUS, 0x4E), // -
    SC2_PRESSED(APOSTROPHE, 0x52), // '
    SC2_PRESSED(LSQUARE, 0x54), // [
    SC2_PRESSED(EQUALS, 0x55), // =
    SC2_PRESSED(CAPSLOCK, 0x58), // Caps Lock
    SC2_PRESSED(RSHIFT, 0x59), // Right Shift
    SC2_PRESSED(ENTER, 0x5A), // Enter
    SC2_PRESSED(RSQUARE, 0x5B), // ]
    SC2_PRESSED(BACKSLASH, 0x5D), // \...
    SC2_PRESSED(BACKSPACE, 0x66), // Backspace
    SC2_PRESSED(KEYPAD1, 0x69), // Keypad 1
    SC2_PRESSED(KEYPAD4, 0x6B), // Keypad 4
    SC2_PRESSED(KEYPAD7, 0x6C), // Keypad 7
    SC2_PRESSED(KEYPAD0, 0x70), // Keypad 0
    SC2_PRESSED(KEYPADDOT, 0x71), // Keypad .
    SC2_PRESSED(KEYPAD2, 0x72), // Keypad 2
    SC2_PRESSED(KEYPAD5, 0x73), // Keypad 5
    SC2_PRESSED(KEYPAD6, 0x74), // Keypad 6
    SC2_PRESSED(KEYPAD8, 0x75), // Keypad 8
    SC2_PRESSED(ESC, 0x76), // ESC
    SC2_PRESSED(NUMLOCK, 0x77), // Num Lock
    SC2_PRESSED(F11, 0x78), // F11
    SC2_PRESSED(KEYPADPLUS, 0x79), // Keypad +
    SC2_PRESSED(KEYPAD3, 0x7A), // Keypad 3
    SC2_PRESSED(KEYPADMINUS, 0x7B), // Keypad -
    SC2_PRESSED(KEYPADASTERISK, 0x7C), // Keypad *
    SC2_PRESSED(KEYPAD9, 0x7D), // Keypad 9
    SC2_PRESSED(SCROLLLOCK, 0x7E), // Scroll Lock
    SC2_PRESSED(F7, 0x83), // F7 

    SC2_2PART_PRESSED(MEDIA_WWW_SEARCH, 0xE0, 0x10), // Media Search
    SC2_2PART_PRESSED(RALT, 0xE0, 0x11), // Right Alt
    SC2_2PART_PRESSED(RCTRL, 0xE0, 0x14), // Right Ctrl
    SC2_2PART_PRESSED(MEDIA_PREVIOUS_TRACK, 0xE0, 0x15), // Media Previous Track
    SC2_2PART_PRESSED(MEDIA_WWW_FAVOURITES, 0xE0, 0x18), // Media Favourites
    SC2_2PART_PRESSED(MEDIA_WWW_REFRESH, 0xE0, 0x20), // Media Refresh
    SC2_2PART_PRESSED(MEDIA_LGUI, 0xE0, 0x1F), // Media Left GUI
    SC2_2PART_PRESSED(MEDIA_VOLUME_DOWN, 0xE0, 0x21), // Media Volume Down
    SC2_2PART_PRESSED(MEDIA_MUTE, 0xE0, 0x23), // Media Mute
    SC2_2PART_PRESSED(MEDIA_RGUI, 0xE0, 0x27), // Media Right GUI
    SC2_2PART_PRESSED(MEDIA_WWW_STOP, 0xE0, 0x28), // Media Stop
    SC2_2PART_PRESSED(MEDIA_CALCULATOR, 0xE0, 0x2B), // Media Calculator
    SC2_2PART_PRESSED(APPS, 0xE0, 0x2F), // Apps
    SC2_2PART_PRESSED(MEDIA_FORWARD, 0xE0, 0x30), // Media Forward
    SC2_2PART_PRESSED(MEDIA_VOLUME_UP, 0xE0, 0x32), // Media Volume Up
    SC2_2PART_PRESSED(MEDIA_PLAY_PAUSE, 0xE0, 0x34), // Media Play/Pause
    SC2_2PART_PRESSED(ACPI_POWER, 0xE0, 0x37), // ACPI Power
    SC2_2PART_PRESSED(MEDIA_WWW_BACK, 0xE0, 0x38), // Media Back
    SC2_2PART_PRESSED(MEDIA_WWW_HOME, 0xE0, 0x3A), // Media Home
    SC2_2PART_PRESSED(MEDIA_STOP, 0xE0, 0x3B), // Media Stop
    SC2_2PART_PRESSED(ACPI_SLEEP, 0xE0, 0x3F), // ACPI Sleep
    SC2_2PART_PRESSED(MEDIA_MY_COMPUTER, 0xE0, 0x40), // Media My Computer
    SC2_2PART_PRESSED(MEDIA_EMAIL, 0xE0, 0x41), // Media Email
    SC2_2PART_PRESSED(KEYPAD_BACKSLASH, 0xE0, 0x4A), // Keypad \...
    SC2_2PART_PRESSED(MEDIA_NEXT_TRACK, 0xE0, 0x4D), // Media Next Track
    SC2_2PART_PRESSED(MEDIA_SELECT, 0xE0, 0x50), // Media Select
    SC2_2PART_PRESSED(KEYPAD_ENTER, 0xE0, 0x5A), // Keypad Enter
    SC2_2PART_PRESSED(ACPI_WAKE, 0xE0, 0x5E), // ACPI Wake
    SC2_2PART_PRESSED(END, 0xE0, 0x69), // End
    SC2_2PART_PRESSED(CURSOR_LEFT, 0xE0, 0x6B), // Left Arrow
    SC2_2PART_PRESSED(HOME, 0xE0, 0x6C), // Home
    SC2_2PART_PRESSED(INSERT, 0xE0, 0x70), // Insert
    SC2_2PART_PRESSED(DELETE, 0xE0, 0x71), // Delete
    SC2_2PART_PRESSED(CURSOR_DOWN, 0xE0, 0x72), // Down Arrow
    SC2_2PART_PRESSED(CURSOR_RIGHT, 0xE0, 0x74), // Right Arrow
    SC2_2PART_PRESSED(CURSOR_UP, 0xE0, 0x75), // Up Arrow
    SC2_2PART_PRESSED(PAGE_DOWN, 0xE0, 0x7A), // Page Down
    SC2_2PART_PRESSED(PAGE_UP, 0xE0, 0x7D), // Page Up

    // Standard released keys
    SC2_2PART_RELEASED(F9, 0xF0, 0x01), // F9
    SC2_2PART_RELEASED(F5, 0xF0, 0x03), // F5
    SC2_2PART_RELEASED(F3, 0xF0, 0x04), // F3
    SC2_2PART_RELEASED(F1, 0xF0, 0x05), // F1
    SC2_2PART_RELEASED(F2, 0xF0, 0x06), // F2
    SC2_2PART_RELEASED(F12, 0xF0, 0x07), // F12
    SC2_2PART_RELEASED(F10, 0xF0, 0x09), // F10
    SC2_2PART_RELEASED(F8, 0xF0, 0x0A), // F8
    SC2_2PART_RELEASED(F6, 0xF0, 0x0B), // F6
    SC2_2PART_RELEASED(F4, 0xF0, 0x0C), // F4
    SC2_2PART_RELEASED(TAB, 0xF0, 0x0D), // TAB
    SC2_2PART_RELEASED(GRAVE, 0xF0, 0x0E), // `~
    SC2_2PART_RELEASED(LALT, 0xF0, 0x11), // Left Alt
    SC2_2PART_RELEASED(LSHIFT, 0xF0, 0x12), // Left Shift
    SC2_2PART_RELEASED(LCTRL, 0xF0, 0x14), // Left Ctrl
    SC2_2PART_RELEASED(Q, 0xF0, 0x15), // Q
    SC2_2PART_RELEASED(1, 0xF0, 0x16), // 1
    SC2_2PART_RELEASED(Z, 0xF0, 0x1A), // Z
    SC2_2PART_RELEASED(S, 0xF0, 0x1B), // S
    SC2_2PART_RELEASED(A, 0xF0, 0x1C), // A
    SC2_2PART_RELEASED(W, 0xF0, 0x1D), // W
    SC2_2PART_RELEASED(2, 0xF0, 0x1E), // 2
    SC2_2PART_RELEASED(C, 0xF0, 0x21), // C
    SC2_2PART_RELEASED(X, 0xF0, 0x22), // X
    SC2_2PART_RELEASED(D, 0xF0, 0x23), // D
    SC2_2PART_RELEASED(E, 0xF0, 0x24), // E
    SC2_2PART_RELEASED(4, 0xF0, 0x25), // 4
    SC2_2PART_RELEASED(3, 0xF0, 0x26), // 3
    SC2_2PART_RELEASED(SPACE, 0xF0, 0x29), // SPACE
    SC2_2PART_RELEASED(V, 0xF0, 0x2A), // V
    SC2_2PART_RELEASED(F, 0xF0, 0x2B), // F
    SC2_2PART_RELEASED(T, 0xF0, 0x2C), // T
    SC2_2PART_RELEASED(R, 0xF0, 0x2D), // R
    SC2_2PART_RELEASED(5, 0xF0, 0x2E), // 5
    SC2_2PART_RELEASED(N, 0xF0, 0x31), // N
    SC2_2PART_RELEASED(B, 0xF0, 0x32), // B
    SC2_2PART_RELEASED(H, 0xF0, 0x33), // H
    SC2_2PART_RELEASED(G, 0xF0, 0x34), // G
    SC2_2PART_RELEASED(Y, 0xF0, 0x35), // Y
    SC2_2PART_RELEASED(6, 0xF0, 0x36), // 6
    SC2_2PART_RELEASED(M, 0xF0, 0x3B), // M
    SC2_2PART_RELEASED(U, 0xF0, 0x3C), // U
    SC2_2PART_RELEASED(7, 0xF0, 0x3D), // 7
    SC2_2PART_RELEASED(8, 0xF0, 0x3E), // 8
    SC2_2PART_RELEASED(COMMA, 0xF0, 0x41), // ,
    SC2_2PART_RELEASED(K, 0xF0, 0x42), // K
    SC2_2PART_RELEASED(I, 0xF0, 0x43), // I
    SC2_2PART_RELEASED(O, 0xF0, 0x44), // O
    SC2_2PART_RELEASED(ZERO, 0xF0, 0x45), // 0
    SC2_2PART_RELEASED(NINE, 0xF0, 0x46), // 9
    SC2_2PART_RELEASED(DOT, 0xF0, 0x49), // .
    SC2_2PART_RELEASED(SLASH, 0xF0, 0x4A), // /
    SC2_2PART_RELEASED(L, 0xF0, 0x4B), // L
    SC2_2PART_RELEASED(SEMICOLON, 0xF0, 0x4C), //
    SC2_2PART_RELEASED(P, 0xF0, 0x4D), // P
    SC2_2PART_RELEASED(MINUS, 0xF0, 0x4E), // -
    SC2_2PART_RELEASED(APOSTROPHE, 0xF0, 0x52), // '
    SC2_2PART_RELEASED(LSQUARE, 0xF0, 0x54), // [
    SC2_2PART_RELEASED(EQUALS, 0xF0, 0x55), // =
    SC2_2PART_RELEASED(CAPSLOCK, 0xF0, 0x58), // Caps Lock
    SC2_2PART_RELEASED(RSHIFT, 0xF0, 0x59), // Right Shift
    SC2_2PART_RELEASED(ENTER, 0xF0, 0x5A), // Enter
    SC2_2PART_RELEASED(RSQUARE, 0xF0, 0x5B), // ]
    SC2_2PART_RELEASED(BACKSLASH, 0xF0, 0x5D), // \...
    SC2_2PART_RELEASED(BACKSPACE, 0xF0, 0x66), // Backspace
    SC2_2PART_RELEASED(KEYPAD1, 0xF0, 0x69), // Keypad
    SC2_2PART_RELEASED(KEYPAD4, 0xF0, 0x6B), // Keypad 4
    SC2_2PART_RELEASED(KEYPAD7, 0xF0, 0x6C), // Keypad
    SC2_2PART_RELEASED(KEYPAD0, 0xF0, 0x70), // Keypad 0
    SC2_2PART_RELEASED(KEYPADDOT, 0xF0, 0x71), // Keypad
    SC2_2PART_RELEASED(KEYPAD2, 0xF0, 0x72), // Keypad 2
    SC2_2PART_RELEASED(KEYPAD5, 0xF0, 0x73), // Keypad
    SC2_2PART_RELEASED(KEYPAD6, 0xF0, 0x74), // Keypad 6
    SC2_2PART_RELEASED(KEYPAD8, 0xF0, 0x75), // Keypad
    SC2_2PART_RELEASED(ESC, 0xF0, 0x76), // ESC
    SC2_2PART_RELEASED(NUMLOCK, 0xF0, 0x77), // Num Lock
    SC2_2PART_RELEASED(F11, 0xF0, 0x78), // F11
    SC2_2PART_RELEASED(KEYPADPLUS, 0xF0, 0x79), // Keypad +
    SC2_2PART_RELEASED(KEYPAD3, 0xF0, 0x7A), // Keypad
    SC2_2PART_RELEASED(KEYPADMINUS, 0xF0, 0x7B), // Keypad -
    SC2_2PART_RELEASED(KEYPADASTERISK, 0xF0, 0x7C), // Keypad *
    SC2_2PART_RELEASED(KEYPAD9, 0xF0, 0x7D), // Keypad 9
    SC2_2PART_RELEASED(SCROLLLOCK, 0xF0, 0x7E), // Scroll Lock
    SC2_2PART_RELEASED(F7, 0xF0, 0x83), // F7

    SC2_4PART_PRESSED(PRINT_SCREEN, 0xE0, 0x12, 0xE0, 0x7C), // Print Screen
    SC2_3PART_RELEASED(MEDIA_WWW_SEARCH, 0xE0, 0xF0, 0x10),
    SC2_3PART_RELEASED(RALT, 0xE0, 0xF0, 0x11), // Right Alt
    SC2_3PART_RELEASED(RCTRL, 0xE0, 0xF0, 0x14), // Right Ctrl
    SC2_3PART_RELEASED(MEDIA_PREVIOUS_TRACK, 0xE0, 0xF0, 0x15),
    SC2_3PART_RELEASED(MEDIA_WWW_FAVOURITES, 0xE0, 0xF0, 0x18),
    SC2_3PART_RELEASED(MEDIA_LGUI, 0xE0, 0xF0, 0x1F), // Media Left GUI
    SC2_3PART_RELEASED(MEDIA_WWW_REFRESH, 0xE0, 0xF0, 0x20),
    SC2_3PART_RELEASED(MEDIA_VOLUME_DOWN, 0xE0, 0xF0, 0x21),
    SC2_3PART_RELEASED(MEDIA_MUTE, 0xE0, 0xF0, 0x23),
    SC2_3PART_RELEASED(MEDIA_RGUI, 0xE0, 0xF0, 0x27), // Media Right GUI
    SC2_3PART_RELEASED(MEDIA_WWW_STOP, 0xE0, 0xF0, 0x28),
    SC2_3PART_RELEASED(MEDIA_CALCULATOR, 0xE0, 0xF0, 0x2B),
    SC2_3PART_RELEASED(APPS, 0xE0, 0xF0, 0x2F), // Apps
    SC2_3PART_RELEASED(MEDIA_FORWARD, 0xE0, 0xF0, 0x30),
    SC2_3PART_RELEASED(MEDIA_VOLUME_UP, 0xE0, 0xF0, 0x32),
    SC2_3PART_RELEASED(MEDIA_PLAY_PAUSE, 0xE0, 0xF0, 0x34),
    SC2_3PART_RELEASED(ACPI_POWER, 0xE0, 0xF0, 0x37), // ACPI Power
    SC2_3PART_RELEASED(MEDIA_WWW_BACK, 0xE0, 0xF0, 0x38), // Media Back
    SC2_3PART_RELEASED(MEDIA_WWW_HOME, 0xE0, 0xF0, 0x3A), // Media Home
    SC2_3PART_RELEASED(MEDIA_STOP, 0xE0, 0xF0, 0x3B), // Media Stop
    SC2_3PART_RELEASED(ACPI_SLEEP, 0xE0, 0xF0, 0x3F), // ACPI Sleep
    SC2_3PART_RELEASED(MEDIA_MY_COMPUTER, 0xE0, 0xF0, 0x40), // Media My Computer
    SC2_3PART_RELEASED(MEDIA_EMAIL, 0xE0, 0xF0, 0x41), // Media Email
    SC2_3PART_RELEASED(KEYPAD_BACKSLASH, 0xE0, 0xF0, 0x4A), // Keypad \...
    SC2_3PART_RELEASED(MEDIA_NEXT_TRACK, 0xE0, 0xF0, 0x4D), // Media Next Track
    SC2_3PART_RELEASED(MEDIA_SELECT, 0xE0, 0xF0, 0x50), // Media Select
    SC2_3PART_RELEASED(KEYPAD_ENTER, 0xE0, 0xF0, 0x5A), // Keypad Enter
    SC2_3PART_RELEASED(ACPI_WAKE, 0xE0, 0xF0, 0x5E), // ACPI Wake
    SC2_3PART_RELEASED(END, 0xE0, 0xF0, 0x69), // End
    SC2_3PART_RELEASED(CURSOR_LEFT, 0xE0, 0xF0, 0x6B), // Left Arrow
    SC2_3PART_RELEASED(HOME, 0xE0, 0xF0, 0x6C), // Home
    SC2_3PART_RELEASED(INSERT, 0xE0, 0xF0, 0x70), // Insert
    SC2_3PART_RELEASED(DELETE, 0xE0, 0xF0, 0x71), // Delete
    SC2_3PART_RELEASED(CURSOR_DOWN, 0xE0, 0xF0, 0x72), // Down Arrow
    SC2_3PART_RELEASED(CURSOR_RIGHT, 0xE0, 0xF0, 0x74), // Right Arrow
    SC2_3PART_RELEASED(CURSOR_UP, 0xE0, 0xF0, 0x75), // Up Arrow
    SC2_3PART_RELEASED(PAGE_DOWN, 0xE0, 0xF0, 0x7A), // Page Down
    SC2_3PART_RELEASED(PAGE_UP, 0xE0, 0xF0, 0x7D), // Page Up
    SC2_7PART_RELEASED(PRINT_SCREEN, 0xE0, 0xF0, 0x7C, 0xE0, 0xE0, 0xF0, 0x12), // Print Screen
    SC2_8PART_PRESSED(PAUSE, 0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77), // Pause
} SC2;

typedef enum {
    KEY_UNKNOWN = 0,

    // Function keys
    KEY_ESC,
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    KEY_PRINT_SCREEN,
    KEY_SCROLLLOCK,
    KEY_PAUSE,

    // Number row + symbols
    KEY_GRAVE, KEY_TILDE,     // ` ~
    KEY_1, KEY_EXCLAMATION,   // 1 !
    KEY_2, KEY_AT,            // 2 @
    KEY_3, KEY_HASH,          // 3 #
    KEY_4, KEY_DOLLAR,        // 4 $
    KEY_5, KEY_PERCENT,       // 5 %
    KEY_6, KEY_CARET,         // 6 ^
    KEY_7, KEY_AMPERSAND,     // 7 &
    KEY_8, KEY_ASTERISK,      // 8 *
    KEY_9, KEY_LEFT_PAREN,    // 9 (
    KEY_0, KEY_RIGHT_PAREN,   // 0 )
    KEY_MINUS, KEY_UNDERSCORE,// - _
    KEY_EQUALS, KEY_PLUS,     // = +
    KEY_BACKSPACE,

    // Navigation cluster
    KEY_INSERT, KEY_HOME, KEY_PAGEUP,
    KEY_DELETE, KEY_END, KEY_PAGEDOWN,

    // Tab row
    KEY_TAB,
    KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P,
    KEY_LBRACKET, KEY_LEFT_CURLY,  // [ {
    KEY_RBRACKET, KEY_RIGHT_CURLY, // ] }
    KEY_ENTER,
    KEY_BACKSLASH, KEY_PIPE,       // \ |

    // Caps lock row
    KEY_CAPSLOCK,
    KEY_A, KEY_S, KEY_D, KEY_F, KEY_G,
    KEY_H, KEY_J, KEY_K, KEY_L,
    KEY_SEMICOLON, KEY_COLON,      // ; :
    KEY_APOSTROPHE, KEY_QUOTE,     // ' "
    
    // Shift row
    KEY_LSHIFT,
    KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B,
    KEY_N, KEY_M,
    KEY_COMMA, KEY_LESS,            // , <
    KEY_DOT, KEY_GREATER,           // . >
    KEY_SLASH, KEY_QUESTION,        // / ?
    KEY_RSHIFT,

    // Space row
    KEY_LCTRL, KEY_LALT, KEY_SPACE, KEY_RALT, KEY_RCTRL, KEY_MENU,

    // Arrow keys
    KEY_ARROW_UP, KEY_ARROW_LEFT, KEY_ARROW_DOWN, KEY_ARROW_RIGHT,

    // Keypad
    KEYPAD_NUMLOCK,
    KEYPAD_DIVIDE, KEYPAD_MULTIPLY, KEYPAD_SUBTRACT,
    KEYPAD_7, KEYPAD_8, KEYPAD_9,
    KEYPAD_4, KEYPAD_5, KEYPAD_6,
    KEYPAD_1, KEYPAD_2, KEYPAD_3,
    KEYPAD_0, KEYPAD_DOT, KEYPAD_ENTER, KEYPAD_PLUS,
    KEYPAD_COMMA, KEYPAD_EQUALS, KEYPAD_BACKSLASH,
    KEYPAD_ASTERISK, KEYPAD_SLASH, KEYPAD_MINUS,

    // Multimedia keys
    KEY_MEDIA_PLAY_PAUSE,
    KEY_MEDIA_STOP,
    KEY_MEDIA_PREVIOUS_TRACK,
    KEY_MEDIA_NEXT_TRACK,
    KEY_MEDIA_FORWARD,
    KEY_MEDIA_MUTE,
    KEY_MEDIA_VOLUME_UP,
    KEY_MEDIA_VOLUME_DOWN,
    KEY_MEDIA_WWW_HOME,
    KEY_MEDIA_WWW_BACK,
    KEY_MEDIA_WWW_FORWARD,
    KEY_MEDIA_WWW_STOP,
    KEY_MEDIA_WWW_REFRESH,
    KEY_MEDIA_WWW_FAVOURITES,
    KEY_MEDIA_WWW_SEARCH,
    KEY_MEDIA_CALCULATOR,
    KEY_MEDIA_MY_COMPUTER,
    KEY_MEDIA_EMAIL,
    KEY_MEDIA_SELECT,

    KEY_MEDIA_LGUI, // Left Windows / Command key
    KEY_MEDIA_RGUI, // Right Windows / Command key

    KEY_APPS, // Menu key

    // ACPI keys
    KEY_ACPI_POWER,
    KEY_ACPI_SLEEP,
    KEY_ACPI_WAKE,
} KEYCODES;

typedef struct {
    KEYCODES keycode;
    BOOLEAN pressed; // TRUE if key is pressed, FALSE if released
} KEYPRESS;

typedef struct {
    BOOLEAN shift;
    BOOLEAN ctrl;
    BOOLEAN alt;
    BOOLEAN capslock;
    BOOLEAN numlock;
    BOOLEAN scrolllock;
} MODIFIERS;

#define DEFAULT_SCANCODESET SCANCODESET2
#define SECONDARY_SCANCODESET SCANCODESET1

#ifdef __RTOS__
MODIFIERS *GET_KEYBOARD_MODIFIERS(VOID);
KEYPRESS GET_CURRENT_KEY_PRESSED(VOID);
U8 KEYPRESS_TO_CHARS(U32 kcode);
KEYPRESS *GET_LAST_KEY_PRESSED(VOID);
#endif // __RTOS__

#endif // PS2_KEYBOARD_H