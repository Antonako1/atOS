#include <DRIVERS/PS2/KEYBOARD_MOUSE.h>
#include <CPU/PIC/PIC.h>
#include <RTOSKRNL/RTOSKRNL_INTERNAL.h>
#include <CPU/ISR/ISR.h>
#include <DRIVERS/VIDEO/VBE.h>
#include <STD/BINARY.h>
#include <STD/MATH.h>
#include <DEBUG/KDEBUG.h>

void PARSE_KEYPRESS(KEYPRESS *key);
KEYPRESS ParsePS2_CS2(U32 scancode1, U32 scancode2, U8 scancode1_bytes, U8 scancode2_bytes);
KEYPRESS ParsePS2_CS1(U32 scancode1, U32 scancode2);

static PS2KB_CMD_QUEUE cmd_queue __attribute__((section(".data"))) = {0};
static PS2_INFO ps2_info __attribute__((section(".data"))) = {0};
static KEYPRESS last_key __attribute__((section(".data"))) = {0}; // Store the last key pressed to avoid repeats
static MOUSE_DATA last_mouse __attribute__((section(".data"))) = {0};
static MODIFIERS modifiers __attribute__((section(".data"))) = {0};
static U8 mouse_cycle = 0;
static U8 mouse_bytes[3];
static volatile PS2_KB_MOUSE_DATA GLOBAL_KB_MOUSE_DATA ATTRIB_DATA = { 0 };

PS2_KB_MOUSE_DATA* GET_KB_MOUSE_DATA() {
    return &GLOBAL_KB_MOUSE_DATA;
}
VOID UPDATE_KP_MOUSE_DATA() {
    PS2_INTERNAL_RETVAL kp = GET_CURRENT_KEY_PRESSED();
    GLOBAL_KB_MOUSE_DATA.kb_event = FALSE;
    GLOBAL_KB_MOUSE_DATA.ms_event = FALSE;
    if(kp.type == KEYBOARD_PACKET) {
        GLOBAL_KB_MOUSE_DATA.kb.prev = last_key;
        GLOBAL_KB_MOUSE_DATA.kb.cur = kp.data.kb;
        GLOBAL_KB_MOUSE_DATA.kb.mods = modifiers;
        GLOBAL_KB_MOUSE_DATA.kb.seq++;
        GLOBAL_KB_MOUSE_DATA.kb_event = TRUE;
    }
    if(kp.type == MOUSE_PACKET) {
        GLOBAL_KB_MOUSE_DATA.ms.prev = last_mouse;
        GLOBAL_KB_MOUSE_DATA.ms.cur = kp.data.mouse;
        GLOBAL_KB_MOUSE_DATA.ms.seq++;
        GLOBAL_KB_MOUSE_DATA.ms_event = TRUE;
        KDEBUG_STR_HEX_LN("Mouse: ", kp.data.mouse.x);
    }
}

PS2_INFO *GET_PS2_INFO(VOID) {
    return &ps2_info;
}

BOOLEAN IS_CMD_QUEUE_EMPTY(VOID) { 
    return cmd_queue.head == cmd_queue.tail; 
}
void PUSH_TO_CMD_QUEUE(PS2_PACKET byte) {
    U8 next = (cmd_queue.head + 1) % CMD_QUEUE_SIZE;
    if(next == cmd_queue.tail) {
        return;
    }
    cmd_queue.buffer[cmd_queue.head] = byte;
    cmd_queue.head = next;
}

PS2KB_CMD_QUEUE *GET_CMD_QUEUE(VOID) { 
    return &cmd_queue; 
}
PS2_PACKET POP_FROM_CMD_QUEUE(VOID) {
    CLI;
    if (IS_CMD_QUEUE_EMPTY()) {
        STI;
        return (PS2_PACKET){ 0, 0 }; // Queue is empty
    }
    PS2_PACKET byte = cmd_queue.buffer[cmd_queue.tail];
    cmd_queue.tail = (cmd_queue.tail + 1) % CMD_QUEUE_SIZE;
    STI;
    return byte;
}
BOOLEAN IS_CMD_QUEUE_FULL(VOID) {
    return (cmd_queue.head + 1) % (U8)CMD_QUEUE_SIZE == cmd_queue.tail;
}
U0 CLEAR_CMD_QUEUE(VOID) {
    cmd_queue.head = 0;
    cmd_queue.tail = 0;
}

U8 PS2_INB(U8 port) {
    U8 status = _inb(port);
    return status;
}

void PS2_MOUSE_HANDLER(I32 num, U32 errcode) {
    (void)num; (void)errcode;
    U8 mouse_byte = PS2_INB(PS2_DATAPORT);
    PUSH_TO_CMD_QUEUE((PS2_PACKET){MOUSE_PACKET, mouse_byte});
    pic_send_eoi(12);
}

void PS2_KEYBOARD_HANDLER(I32 num, U32 errcode) {
    (void)num; (void)errcode; // not used
    U8 scancode = PS2_INB(PS2_DATAPORT);
    PUSH_TO_CMD_QUEUE((PS2_PACKET){KEYBOARD_PACKET, scancode});
    pic_send_eoi(1); // IRQ1 is for keyboard
}

BOOLEAN PS2_WAIT_FOR_INPUT_CLEAR(VOID) {
    int timeout = 10000;
    while(PS2_INB(PS2_STATUSPORT) & PS2_INPUT_BUFFER_FULL && --timeout);
    if(timeout == 0) return FALSE;
    return TRUE;
}

BOOLEAN PS2_ACKNOWLEDGED() {
    if(!PS2_WAIT_FOR_INPUT_CLEAR()) return FALSE;
    U8 status = PS2_INB(PS2_DATAPORT);
    if(status == PS2_SPECIAL_RESEND) return FALSE;
    if(status != PS2_SPECIAL_ACK) return FALSE;
    return TRUE;
}

BOOLEAN PS2_OUTB(U8 port, U8 data) {
    if(!PS2_WAIT_FOR_INPUT_CLEAR()) return FALSE;
    _outb(port, data);
    U8 status = PS2_INB(PS2_DATAPORT);
    if(status == PS2_SPECIAL_RESEND) {
        for(U8 i = 0; i < 3; i++) {
            if(!PS2_WAIT_FOR_INPUT_CLEAR()) return FALSE;
            _outb(port, data);
            status = PS2_INB(PS2_DATAPORT);
            if(status == PS2_SPECIAL_ACK) break;
            if(status != PS2_SPECIAL_RESEND) return FALSE;
        }
    } else {
        if(status != PS2_SPECIAL_ACK) return FALSE;
    }
    if(!PS2_WAIT_FOR_INPUT_CLEAR()) return FALSE;
    return TRUE;
}

BOOLEAN PS2_SET_SCAN_CODE_SET(U8 scan_code_set) {
    if(!PS2_OUTB(PS2_WRITEPORT, PS2_SET_SCANCODE_SET)) return FALSE;
    if(!PS2_OUTB(PS2_DATAPORT, scan_code_set)) return FALSE;
    return TRUE;
}

BOOLEAN PS2_KEYBOARD_RETRY_TRY(VOID) {
    if(!PS2_OUTB(PS2_DATAPORT, PS2_RESET_AND_SELF_TEST)) return FALSE;
    if(PS2_INB(PS2_DATAPORT) != PS2_SPECIAL_SELF_TEST_PASSED) return FALSE;
    return TRUE;
}

BOOLEAN PS2_KEYBOARD_RESET(VOID) {
    for(U8 i = 0; i < 3; i++) {
        PS2_WAIT_FOR_INPUT_CLEAR();
        if(PS2_KEYBOARD_RETRY_TRY()) {
            return TRUE; // Success
        }
    }
    return FALSE; // All attempts failed
}

BOOLEAN PS2_EMPTY_KEYBOARD_BUFFER(VOID) {
    while((PS2_INB(PS2_DATAPORT) & PS2_OUTPUT_BUFFER_FULL)) {
        (void)_inb(PS2_DATAPORT);
    }
    return TRUE;
}

BOOLEAN PS2_EnableScanning(VOID) {
    if(!PS2_OUTB(PS2_DATAPORT, PS2_ENABLE_SCANNING)) {
        return FALSE;
    }
    if(!PS2_ACKNOWLEDGED()) {
        return FALSE;
    }
    return TRUE;
}

BOOLEAN PS2_DisableScanning(VOID) {
    if(!PS2_OUTB(PS2_DATAPORT, PS2_DISABLE_SCANNING)) {
        return FALSE;
    }
    if(!PS2_ACKNOWLEDGED()) {
        return FALSE;
    }
    return TRUE;
}

U16 PS2_Identify(VOID) {
    if(!PS2_OUTB(PS2_DATAPORT, PS2_IDENTIFY_KEYBOARD)) {
        return 0; // Failed to send identify command
    }
    U8 first_byte = PS2_INB(PS2_DATAPORT);
    if(first_byte == 0xAB) {
        U8 second_byte = PS2_INB(PS2_DATAPORT);
        return first_byte << 8 | second_byte;
    } else {
        return first_byte; // Unknown device
    }
}

// #define PS2_TEST
BOOLEAN PS2_KEYBOARD_INIT(VOID) {
    #ifndef PS2_TEST
    // Initialize USB controllers
    
    if(!PS2_DisableScanning()) return FALSE;
    
    U16 PS2_type = PS2_Identify();
    ps2_info.type = PS2_type;
    ps2_info.dual_channel = FALSE;
    ps2_info.exists = TRUE;
    
    // Disable devices
    _outb(PS2_CMDPORT, DISABLE_FIRST_PS2_PORT);
    _outb(PS2_CMDPORT, DISABLE_SECOND_PS2_PORT);
    
    // Set up controller configuration byte
    _outb(PS2_CMDPORT, GET_CONTROLLER_CONFIGURATION_BYTE);
    U8 config1 = _inb(PS2_DATAPORT);
    FLAG_UNSET(config1, 0b01010010);
    _outb(PS2_CMDPORT, SET_CONTROLLER_CONFIGURATION_BYTE);  // send “write config byte” command
    _outb(PS2_WRITEPORT, config1);  // write value
    
    // Test whether there are 2 or 1 channels
    _outb(PS2_CMDPORT, ENABLE_SECOND_PS2_PORT);
    _outb(PS2_CMDPORT, GET_CONTROLLER_CONFIGURATION_BYTE);
    U8 config2 = _inb(PS2_DATAPORT);
    // If bit 5 is clear, it exists
    if(IS_FLAG_UNSET(config2, 0b00100000)) {
        ps2_info.dual_channel = TRUE;
        FLAG_UNSET(config2, 0b00100010);
        _outb(PS2_CMDPORT, SET_CONTROLLER_CONFIGURATION_BYTE);
        _outb(PS2_WRITEPORT, config2);
    }
    
    // Perform inteface test
    _outb(PS2_CMDPORT, TEST_FIRST_PS2_PORT);
    ps2_info.port1_check = PS2_INB(PS2_DATAPORT);
    if(ps2_info.dual_channel) {
        _outb(PS2_CMDPORT, TEST_SECOND_PS2_PORT);
        ps2_info.port2_check = PS2_INB(PS2_DATAPORT);
    } else {
        ps2_info.port2_check = 0;
    }
    
    // Enable first PS/2 port
    _outb(PS2_CMDPORT, ENABLE_FIRST_PS2_PORT);
    FLAG_SET(config1, 0b00000001); // Enable interrupt for first PS/2 port
    _outb(PS2_CMDPORT, SET_CONTROLLER_CONFIGURATION_BYTE);
    _outb(PS2_WRITEPORT, config1);
    
    // Enable second PS/2 port if exists
    if(ps2_info.dual_channel) {
        _outb(PS2_CMDPORT, ENABLE_SECOND_PS2_PORT);
        FLAG_SET(config2, 0b00000100); // Enable interrupt for second PS/2 port
        _outb(PS2_CMDPORT, SET_CONTROLLER_CONFIGURATION_BYTE);
        _outb(PS2_WRITEPORT, config2);
    }
    
    if(!PS2_KEYBOARD_RESET()) return FALSE;
    
    if(!PS2_WAIT_FOR_INPUT_CLEAR()) return FALSE;
    
    
    if(!PS2_SET_SCAN_CODE_SET(DEFAULT_SCANCODESET)) {
        if(!PS2_SET_SCAN_CODE_SET(SECONDARY_SCANCODESET)) {
            return FALSE; // Set scan code set failed
        }
        ps2_info.scancode_set = SECONDARY_SCANCODESET;
    } else {
        ps2_info.scancode_set = DEFAULT_SCANCODESET;
    }

    if(!PS2_EMPTY_KEYBOARD_BUFFER()) { // Clear keyboard buffer
        return FALSE;
    }

    #endif // PS2_TEST

    ISR_REGISTER_HANDLER(PIC_REMAP_OFFSET + 1, PS2_KEYBOARD_HANDLER);
    PIC_Unmask(1);
    #ifndef PS2_TEST
    // enable scanning
    if(!PS2_EnableScanning()) return FALSE;
    #endif // PS2_TEST
    CLEAR_CMD_QUEUE();
    modifiers.shift = FALSE;
    modifiers.ctrl = FALSE;
    modifiers.alt = FALSE;
    modifiers.capslock = FALSE;
    modifiers.numlock = FALSE;
    modifiers.scrolllock = FALSE;
    last_key.keycode = KEY_UNKNOWN;
    last_key.pressed = FALSE;
    return TRUE;
}

BOOLEAN PS2_MOUSE_INIT() {
    PIC_Unmask(12);
    return TRUE;
}

KEYPRESS *GET_LAST_KEY_PRESSED(VOID) {
    return &last_key;
}

// Move these to static scope so they persist between calls
static U32 scancode1 = 0, scancode2 = 0;
static U8 scancode1_size = 0, scancode2_size = 0;

PS2_INTERNAL_RETVAL GET_CURRENT_KEY_PRESSED(VOID) {
    if(IS_CMD_QUEUE_EMPTY()) return (PS2_INTERNAL_RETVAL){0};
    
    PS2_INTERNAL_RETVAL retval = {0};
    retval.type = 0;

    // We process until we find a COMPLETE event (1 full key or 3 full mouse bytes)
    while(!IS_CMD_QUEUE_EMPTY()) {
        PS2_PACKET packet = POP_FROM_CMD_QUEUE();
        
        if(packet.device == KEYBOARD_PACKET) {
            U8 scancode = packet.data;
            
            // Build the scancode buffers
            if(scancode1_size < 4) {
                scancode1 = (scancode1 << 8) | scancode;
                scancode1_size++;
            } else {
                scancode2 = (scancode2 << 8) | scancode;
                scancode2_size++;
            }

            KEYPRESS result = { 0 };
            if(ps2_info.scancode_set == SCANCODESET1) {
                result = ParsePS2_CS1(scancode1, scancode2);
            } else {
                // If it's a prefix byte in Set 2, we must wait for the next byte
                if(scancode == 0xF0 || scancode == 0xE0) {
                    continue; 
                }
                result = ParsePS2_CS2(scancode1, scancode2, scancode1_size, scancode2_size);
            }

            // Reset buffers for next key
            scancode1 = 0; scancode2 = 0;
            scancode1_size = 0; scancode2_size = 0;

            if(result.keycode != KEY_UNKNOWN) {
                retval.type = KEYBOARD_PACKET;
                retval.data.kb = result;
                // Update global state
                PARSE_KEYPRESS(&retval.data.kb); // Handle modifiers
                last_key = retval.data.kb;
            }
            return retval;
        } 
        else if(packet.device == MOUSE_PACKET) {
            mouse_bytes[mouse_cycle++] = packet.data;
            
            // Sync check
            if (mouse_cycle == 1 && !(mouse_bytes[0] & 0x08)) {
                mouse_cycle = 0;
                continue;
            }

            if (mouse_cycle == 3) {
                mouse_cycle = 0;
                
                I32 rel_x = (I32)mouse_bytes[1];
                I32 rel_y = (I32)mouse_bytes[2];
                
                // Sign extension for 9-bit relative movement
                if (mouse_bytes[0] & 0x10) rel_x -= 256;
                if (mouse_bytes[0] & 0x20) rel_y -= 256;

                // Update absolute coordinates in last_mouse
                // Assuming SCREEN_WIDTH and HEIGHT are defined
                last_mouse.x = clampi(last_mouse.x + rel_x, 0, SCREEN_WIDTH);
                last_mouse.y = clampi(last_mouse.y - rel_y, 0, SCREEN_HEIGHT); // Y is usually inverted
                
                last_mouse.mouse1 = (mouse_bytes[0] & 0x01) ? TRUE : FALSE;
                last_mouse.mouse2 = (mouse_bytes[0] & 0x02) ? TRUE : FALSE;
                last_mouse.mouse3 = (mouse_bytes[0] & 0x04) ? TRUE : FALSE;

                retval.type = MOUSE_PACKET;
                retval.data.mouse = last_mouse;
                return retval;
            }
        }
    }
    return retval;
}

void PARSE_KEYPRESS(KEYPRESS *key) {
    if (!key) return;
    
    if(key->pressed) {
        switch(key->keycode) {
            case KEY_CAPSLOCK:
                modifiers.capslock = !modifiers.capslock;
                return; // No further processing needed
            case KEYPAD_NUMLOCK:
                modifiers.numlock = !modifiers.numlock;
                return; // No further processing needed
            case KEY_SCROLLLOCK:
                modifiers.scrolllock = !modifiers.scrolllock;
                return; // No further processing needed
            case KEY_RALT:
            case KEY_LALT:
                modifiers.alt = TRUE;
                break; // No further processing needed
            case KEY_LCTRL:
            case KEY_RCTRL:
                modifiers.ctrl = TRUE;
                break; // No further processing needed
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                modifiers.shift = TRUE;
                break; // No further processing needed
            default:
                break;
        }
    } else {
        switch(key->keycode) {
            case KEY_RALT:
            case KEY_LALT:
                modifiers.alt = FALSE;
                break; // No further processing needed
            case KEY_LCTRL:
            case KEY_RCTRL:
                modifiers.ctrl = FALSE;
                break; // No further processing needed
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                modifiers.shift = FALSE;
                break; // No further processing needed
            default:
                break;
        }
    }

    if(modifiers.shift)
        switch(key->keycode) {
            case KEY_1: key->keycode = KEY_EXCLAMATION; break;
            case KEY_2: key->keycode = KEY_AT; break;
            case KEY_3: key->keycode = KEY_HASH; break;
            case KEY_4: key->keycode = KEY_DOLLAR; break;
            case KEY_5: key->keycode = KEY_PERCENT; break;
            case KEY_6: key->keycode = KEY_CARET; break;
            case KEY_7: key->keycode = KEY_AMPERSAND; break;
            case KEY_8: key->keycode = KEY_ASTERISK; break;
            case KEY_9: key->keycode = KEY_LEFT_PAREN; break;
            case KEY_0: key->keycode = KEY_RIGHT_PAREN; break;
            case KEY_MINUS: key->keycode = KEY_UNDERSCORE; break;
            case KEY_EQUALS: key->keycode = KEY_PLUS; break;
            case KEY_GRAVE: key->keycode = KEY_TILDE; break;
            case KEY_LBRACKET: key->keycode = KEY_LEFT_CURLY; break;
            case KEY_RBRACKET: key->keycode = KEY_RIGHT_CURLY; break;
            case KEY_BACKSLASH: key->keycode = KEY_PIPE; break;
            case KEY_SEMICOLON: key->keycode = KEY_COLON; break;
            case KEY_APOSTROPHE: key->keycode = KEY_QUOTE; break;
            case KEY_COMMA: key->keycode = KEY_LESS; break;
            case KEY_DOT: key->keycode = KEY_GREATER; break;
            case KEY_SLASH: key->keycode = KEY_QUESTION; break;
            default:
                break;
        }
}

KEYPRESS ParsePS2_CS1(U32 scancode1, U32 scancode2) {
    return (KEYPRESS){0};
}
KEYPRESS ParsePS2_CS2(U32 scancode1, U32 scancode2, U8 scancode1_bytes, U8 scancode2_bytes) {
    U8 byte1 = 0, byte2 = 0, byte3 = 0, byte4 = 0,
        byte5 = 0, byte6 = 0, byte7 = 0, byte8 = 0;
    KEYPRESS keypress = {0};
    keypress.keycode = KEY_UNKNOWN;
    keypress.pressed = TRUE; // Assume pressed, unless we detect it's released

    byte1 = (scancode1 >> 8*(scancode1_bytes-1)) & 0xFF; // first sent byte
    byte2 = (scancode1 >> 8*(scancode1_bytes-2)) & 0xFF; // second byte
    byte3 = (scancode1 >> 8*(scancode1_bytes-3)) & 0xFF;
    byte4 = (scancode1 >> 8*(scancode1_bytes-4)) & 0xFF;
    byte5 = (scancode2 & 8*(scancode2_bytes-1)) & 0xFF; // first sent byte of second scancode
    byte6 = (scancode2 & 8*(scancode2_bytes-2)) & 0xFF; // second byte of second scancode
    byte7 = (scancode2 & 8*(scancode2_bytes-3)) & 0xFF;
    byte8 = (scancode2 & 8*(scancode2_bytes-4)) & 0xFF;

    switch(byte1) {
        case SC2_PRESSED_F9: keypress.keycode = KEY_F9; break;
        case SC2_PRESSED_F5: keypress.keycode = KEY_F5; break;
        case SC2_PRESSED_F3: keypress.keycode = KEY_F3; break;
        case SC2_PRESSED_F1: keypress.keycode = KEY_F1; break;
        case SC2_PRESSED_F2: keypress.keycode = KEY_F2; break;
        case SC2_PRESSED_F12: keypress.keycode = KEY_F12; break;
        case SC2_PRESSED_F10: keypress.keycode = KEY_F10; break;
        case SC2_PRESSED_F8: keypress.keycode = KEY_F8; break;
        case SC2_PRESSED_F6: keypress.keycode = KEY_F6; break;
        case SC2_PRESSED_F4: keypress.keycode = KEY_F4; break;
        case SC2_PRESSED_TAB: keypress.keycode = KEY_TAB; break;
        case SC2_PRESSED_GRAVE: keypress.keycode = KEY_GRAVE; break;
        case SC2_PRESSED_LALT: keypress.keycode = KEY_LALT; break;
        case SC2_PRESSED_LSHIFT: keypress.keycode = KEY_LSHIFT; break;
        case SC2_PRESSED_LCTRL: keypress.keycode = KEY_LCTRL; break;
        case SC2_PRESSED_Q: keypress.keycode = KEY_Q; break;
        case SC2_PRESSED_1: keypress.keycode = KEY_1; break;
        case SC2_PRESSED_Z: keypress.keycode = KEY_Z; break;
        case SC2_PRESSED_S: keypress.keycode = KEY_S; break;
        case SC2_PRESSED_A: keypress.keycode = KEY_A; break;
        case SC2_PRESSED_W: keypress.keycode = KEY_W; break;
        case SC2_PRESSED_2: keypress.keycode = KEY_2; break;
        case SC2_PRESSED_C: keypress.keycode = KEY_C; break;
        case SC2_PRESSED_X: keypress.keycode = KEY_X; break;
        case SC2_PRESSED_D: keypress.keycode = KEY_D; break;
        case SC2_PRESSED_E: keypress.keycode = KEY_E; break;
        case SC2_PRESSED_4: keypress.keycode = KEY_4; break;
        case SC2_PRESSED_3: keypress.keycode = KEY_3; break;
        case SC2_PRESSED_SPACE: keypress.keycode = KEY_SPACE; break;
        case SC2_PRESSED_V: keypress.keycode = KEY_V; break;
        case SC2_PRESSED_F: keypress.keycode = KEY_F; break;
        case SC2_PRESSED_T: keypress.keycode = KEY_T; break;
        case SC2_PRESSED_R: keypress.keycode = KEY_R; break;
        case SC2_PRESSED_5: keypress.keycode = KEY_5; break;
        case SC2_PRESSED_N: keypress.keycode = KEY_N; break;
        case SC2_PRESSED_B: keypress.keycode = KEY_B; break;
        case SC2_PRESSED_H: keypress.keycode = KEY_H; break;
        case SC2_PRESSED_G: keypress.keycode = KEY_G; break;
        case SC2_PRESSED_Y: keypress.keycode = KEY_Y; break;
        case SC2_PRESSED_6: keypress.keycode = KEY_6; break;
        case SC2_PRESSED_M: keypress.keycode = KEY_M; break;
        case SC2_PRESSED_J: keypress.keycode = KEY_J; break;
        case SC2_PRESSED_U: keypress.keycode = KEY_U; break;
        case SC2_PRESSED_7: keypress.keycode = KEY_7; break;
        case SC2_PRESSED_8: keypress.keycode = KEY_8; break;
        case SC2_PRESSED_COMMA: keypress.keycode = KEY_COMMA; break;
        case SC2_PRESSED_K: keypress.keycode = KEY_K; break;
        case SC2_PRESSED_I: keypress.keycode = KEY_I; break;
        case SC2_PRESSED_O: keypress.keycode = KEY_O; break;
        case SC2_PRESSED_ZERO: keypress.keycode = KEY_0; break;
        case SC2_PRESSED_NINE: keypress.keycode = KEY_9; break;
        case SC2_PRESSED_DOT: keypress.keycode = KEY_DOT; break;
        case SC2_PRESSED_SLASH: keypress.keycode = KEY_SLASH; break;
        case SC2_PRESSED_L: keypress.keycode = KEY_L; break;
        case SC2_PRESSED_SEMICOLON: keypress.keycode = KEY_SEMICOLON; break;    
        case SC2_PRESSED_P: keypress.keycode = KEY_P; break;
        case SC2_PRESSED_MINUS: keypress.keycode = KEY_MINUS; break;
        case SC2_PRESSED_APOSTROPHE: keypress.keycode = KEY_APOSTROPHE; break;
        case SC2_PRESSED_LSQUARE: keypress.keycode = KEY_LBRACKET; break;
        case SC2_PRESSED_EQUALS: keypress.keycode = KEY_EQUALS; break;
        case SC2_PRESSED_CAPSLOCK: keypress.keycode = KEY_CAPSLOCK; break;
        case SC2_PRESSED_RSHIFT: keypress.keycode = KEY_RSHIFT; break;
        case SC2_PRESSED_ENTER: keypress.keycode = KEY_ENTER; break;
        case SC2_PRESSED_RSQUARE: keypress.keycode = KEY_RBRACKET; break;
        case SC2_PRESSED_BACKSLASH: keypress.keycode = KEY_BACKSLASH; break;
        case SC2_PRESSED_BACKSPACE: keypress.keycode = KEY_BACKSPACE; break;
        case SC2_PRESSED_KEYPAD1: keypress.keycode = KEYPAD_1; break;
        case SC2_PRESSED_KEYPAD4: keypress.keycode = KEYPAD_4; break;
        case SC2_PRESSED_KEYPAD7: keypress.keycode = KEYPAD_7; break;
        case SC2_PRESSED_KEYPAD0: keypress.keycode = KEYPAD_0; break;
        case SC2_PRESSED_KEYPADDOT: keypress.keycode = KEYPAD_DOT; break;
        case SC2_PRESSED_KEYPAD2: keypress.keycode = KEYPAD_2; break;
        case SC2_PRESSED_KEYPAD5: keypress.keycode = KEYPAD_5; break;
        case SC2_PRESSED_KEYPAD6: keypress.keycode = KEYPAD_6; break;
        case SC2_PRESSED_KEYPAD8: keypress.keycode = KEYPAD_8; break;
        case SC2_PRESSED_ESC: keypress.keycode = KEY_ESC; break;
        case SC2_PRESSED_NUMLOCK: keypress.keycode = KEYPAD_NUMLOCK; break;
        case SC2_PRESSED_F11: keypress.keycode = KEY_F11; break;
        case SC2_PRESSED_KEYPADPLUS: keypress.keycode = KEYPAD_PLUS; break;
        case SC2_PRESSED_KEYPAD3: keypress.keycode = KEYPAD_3; break;
        case SC2_PRESSED_KEYPADMINUS: keypress.keycode = KEYPAD_MINUS; break;
        case SC2_PRESSED_KEYPADASTERISK: keypress.keycode = KEYPAD_ASTERISK; break;
        case SC2_PRESSED_KEYPAD9: keypress.keycode = KEYPAD_9; break;
        case SC2_PRESSED_SCROLLLOCK: keypress.keycode = KEY_SCROLLLOCK; break;
        case SC2_PRESSED_F7: keypress.keycode = KEY_F7; break;

        // 0xF0 starting brakes
        case SC2_RELEASE_F0: {
            keypress.pressed = FALSE;
            switch(byte2){
                case SC2_RELEASED_PART2_F9: keypress.keycode = KEY_F9; break;
                case SC2_RELEASED_PART2_F5: keypress.keycode = KEY_F5; break;
                case SC2_RELEASED_PART2_F3: keypress.keycode = KEY_F3; break;
                case SC2_RELEASED_PART2_F1: keypress.keycode = KEY_F1; break;
                case SC2_RELEASED_PART2_F2: keypress.keycode = KEY_F2; break;
                case SC2_RELEASED_PART2_F12: keypress.keycode = KEY_F12; break;
                case SC2_RELEASED_PART2_F10: keypress.keycode = KEY_F10; break;
                case SC2_RELEASED_PART2_F8: keypress.keycode = KEY_F8; break;
                case SC2_RELEASED_PART2_F6: keypress.keycode = KEY_F6; break;
                case SC2_RELEASED_PART2_F4: keypress.keycode = KEY_F4; break;
                case SC2_RELEASED_PART2_TAB: keypress.keycode = KEY_TAB; break;
                case SC2_RELEASED_PART2_GRAVE: keypress.keycode = KEY_GRAVE; break;
                case SC2_RELEASED_PART2_LALT: keypress.keycode = KEY_LALT; break;
                case SC2_RELEASED_PART2_LSHIFT: keypress.keycode = KEY_LSHIFT; break;
                case SC2_RELEASED_PART2_LCTRL: keypress.keycode = KEY_LCTRL; break;
                case SC2_RELEASED_PART2_Q: keypress.keycode = KEY_Q; break;
                case SC2_RELEASED_PART2_1: keypress.keycode = KEY_1; break;
                case SC2_RELEASED_PART2_Z: keypress.keycode = KEY_Z; break;
                case SC2_RELEASED_PART2_S: keypress.keycode = KEY_S; break;
                case SC2_RELEASED_PART2_A: keypress.keycode = KEY_A; break;
                case SC2_RELEASED_PART2_W: keypress.keycode = KEY_W; break;
                case SC2_RELEASED_PART2_2: keypress.keycode = KEY_2; break;
                case SC2_RELEASED_PART2_C: keypress.keycode = KEY_C; break;
                case SC2_RELEASED_PART2_X: keypress.keycode = KEY_X; break;
                case SC2_RELEASED_PART2_D: keypress.keycode = KEY_D; break;
                case SC2_RELEASED_PART2_E: keypress.keycode = KEY_E; break;
                case SC2_RELEASED_PART2_4: keypress.keycode = KEY_4; break;
                case SC2_RELEASED_PART2_3: keypress.keycode = KEY_3; break;
                case SC2_RELEASED_PART2_SPACE: keypress.keycode = KEY_SPACE; break;
                case SC2_RELEASED_PART2_V: keypress.keycode = KEY_V; break;
                case SC2_RELEASED_PART2_F: keypress.keycode = KEY_F; break;
                case SC2_RELEASED_PART2_T: keypress.keycode = KEY_T; break;
                case SC2_RELEASED_PART2_R: keypress.keycode = KEY_R; break;
                case SC2_RELEASED_PART2_5: keypress.keycode = KEY_5; break;
                case SC2_RELEASED_PART2_N: keypress.keycode = KEY_N; break;
                case SC2_RELEASED_PART2_B: keypress.keycode = KEY_B; break;
                case SC2_RELEASED_PART2_H: keypress.keycode = KEY_H; break;
                case SC2_RELEASED_PART2_G: keypress.keycode = KEY_G; break;
                case SC2_RELEASED_PART2_Y: keypress.keycode = KEY_Y; break;
                case SC2_RELEASED_PART2_6: keypress.keycode = KEY_6; break;
                case SC2_RELEASED_PART2_M: keypress.keycode = KEY_M; break;
                case SC2_RELEASED_PART2_U: keypress.keycode = KEY_U; break;
                case SC2_RELEASED_PART2_7: keypress.keycode = KEY_7; break;
                case SC2_RELEASED_PART2_8: keypress.keycode = KEY_8; break;
                case SC2_RELEASED_PART2_COMMA: keypress.keycode = KEY_COMMA; break;
                case SC2_RELEASED_PART2_K: keypress.keycode = KEY_K; break;
                case SC2_RELEASED_PART2_I: keypress.keycode = KEY_I; break;
                case SC2_RELEASED_PART2_O: keypress.keycode = KEY_O; break;
                case SC2_RELEASED_PART2_ZERO: keypress.keycode = KEY_0; break;
                case SC2_RELEASED_PART2_NINE: keypress.keycode = KEY_9; break;
                case SC2_RELEASED_PART2_DOT: keypress.keycode = KEY_DOT; break;
                case SC2_RELEASED_PART2_SLASH: keypress.keycode = KEY_SLASH; break;
                case SC2_RELEASED_PART2_L: keypress.keycode = KEY_L; break;
                case SC2_RELEASED_PART2_SEMICOLON: keypress.keycode = KEY_SEMICOLON; break;    
                case SC2_RELEASED_PART2_P: keypress.keycode = KEY_P; break;
                case SC2_RELEASED_PART2_MINUS: keypress.keycode = KEY_MINUS; break;
                case SC2_RELEASED_PART2_APOSTROPHE: keypress.keycode = KEY_APOSTROPHE; break;
                case SC2_RELEASED_PART2_LSQUARE: keypress.keycode = KEY_LBRACKET; break;
                case SC2_RELEASED_PART2_EQUALS: keypress.keycode = KEY_EQUALS; break;
                case SC2_RELEASED_PART2_CAPSLOCK: keypress.keycode = KEY_CAPSLOCK; break;
                case SC2_RELEASED_PART2_RSHIFT: keypress.keycode = KEY_RSHIFT; break;
                case SC2_RELEASED_PART2_ENTER: keypress.keycode = KEY_ENTER; break;
                case SC2_RELEASED_PART2_RSQUARE: keypress.keycode = KEY_RBRACKET; break;
                case SC2_RELEASED_PART2_BACKSLASH: keypress.keycode = KEY_BACKSLASH; break;
                case SC2_RELEASED_PART2_BACKSPACE: keypress.keycode = KEY_BACKSPACE; break;
                case SC2_RELEASED_PART2_KEYPAD1: keypress.keycode = KEYPAD_1; break;
                case SC2_RELEASED_PART2_KEYPAD4: keypress.keycode = KEYPAD_4; break;
                case SC2_RELEASED_PART2_KEYPAD7: keypress.keycode = KEYPAD_7; break;
                case SC2_RELEASED_PART2_KEYPAD0: keypress.keycode = KEYPAD_0; break;
                case SC2_RELEASED_PART2_KEYPADDOT: keypress.keycode = KEYPAD_DOT; break;
                case SC2_RELEASED_PART2_KEYPAD2: keypress.keycode = KEYPAD_2; break;
                case SC2_RELEASED_PART2_KEYPAD5: keypress.keycode = KEYPAD_5; break;
                case SC2_RELEASED_PART2_KEYPAD6: keypress.keycode = KEYPAD_6; break;
                case SC2_RELEASED_PART2_KEYPAD8: keypress.keycode = KEYPAD_8; break;
                case SC2_RELEASED_PART2_ESC: keypress.keycode = KEY_ESC; break;
                case SC2_RELEASED_PART2_NUMLOCK: keypress.keycode = KEYPAD_NUMLOCK; break;
                case SC2_RELEASED_PART2_F11: keypress.keycode = KEY_F11; break;
                case SC2_RELEASED_PART2_KEYPADPLUS: keypress.keycode = KEYPAD_PLUS; break;
                case SC2_RELEASED_PART2_KEYPAD3: keypress.keycode = KEYPAD_3; break;
                case SC2_RELEASED_PART2_KEYPADMINUS: keypress.keycode = KEYPAD_MINUS; break;
                case SC2_RELEASED_PART2_KEYPADASTERISK: keypress.keycode = KEYPAD_ASTERISK; break;
                case SC2_RELEASED_PART2_KEYPAD9: keypress.keycode = KEYPAD_9; break;
                case SC2_RELEASED_PART2_SCROLLLOCK: keypress.keycode = KEY_SCROLLLOCK; break;
                case SC2_RELEASED_PART2_F7: keypress.keycode = KEY_F7; break;
                default: keypress.keycode = KEY_UNKNOWN; break;
            }
        } break;


        // 0xE0 starting codes
        case SC2_RELEASE_E0:
            if(byte2 == SC2_RELEASE_F0) { // Released
                keypress.pressed = FALSE;
                    switch(byte3) {
                    case SC2_RELEASED_PART3_PRINT_SCREEN:
                        if(byte4 == SC2_RELEASED_PART4_PRINT_SCREEN && 
                            byte5 == SC2_RELEASED_PART5_PRINT_SCREEN &&
                            byte6 == SC2_RELEASED_PART6_PRINT_SCREEN &&
                            byte7 == SC2_RELEASED_PART7_PRINT_SCREEN
                        ) {
                            keypress.pressed = FALSE;
                            keypress.keycode = KEY_PRINT_SCREEN;
                        } else {
                            keypress.keycode = KEY_UNKNOWN;
                        }
                        break;
                    case SC2_RELEASED_PART3_MEDIA_WWW_SEARCH: keypress.keycode = KEY_MEDIA_WWW_SEARCH; break;
                    case SC2_RELEASED_PART3_RALT: keypress.keycode = KEY_RALT; break;
                    case SC2_RELEASED_PART3_RCTRL: keypress.keycode = KEY_RCTRL; break;
                    case SC2_RELEASED_PART3_MEDIA_PREVIOUS_TRACK: keypress.keycode = KEY_MEDIA_PREVIOUS_TRACK; break;
                    case SC2_RELEASED_PART3_MEDIA_WWW_FAVOURITES: keypress.keycode = KEY_MEDIA_WWW_FAVOURITES; break;
                    case SC2_RELEASED_PART3_MEDIA_WWW_REFRESH: keypress.keycode = KEY_MEDIA_WWW_REFRESH; break;
                    case SC2_RELEASED_PART3_MEDIA_LGUI: keypress.keycode = KEY_MEDIA_LGUI; break;
                    case SC2_RELEASED_PART3_MEDIA_VOLUME_DOWN: keypress.keycode = KEY_MEDIA_VOLUME_DOWN; break;
                    case SC2_RELEASED_PART3_MEDIA_MUTE: keypress.keycode = KEY_MEDIA_MUTE; break;
                    case SC2_RELEASED_PART3_MEDIA_RGUI: keypress.keycode = KEY_MEDIA_RGUI; break;
                    case SC2_RELEASED_PART3_MEDIA_WWW_STOP: keypress.keycode = KEY_MEDIA_WWW_STOP; break;
                    case SC2_RELEASED_PART3_MEDIA_CALCULATOR: keypress.keycode = KEY_MEDIA_CALCULATOR; break;
                    case SC2_RELEASED_PART3_APPS: keypress.keycode = KEY_APPS; break;
                    case SC2_RELEASED_PART3_MEDIA_FORWARD: keypress.keycode = KEY_MEDIA_FORWARD; break;
                    case SC2_RELEASED_PART3_MEDIA_VOLUME_UP: keypress.keycode = KEY_MEDIA_VOLUME_UP; break;
                    case SC2_RELEASED_PART3_MEDIA_PLAY_PAUSE: keypress.keycode = KEY_MEDIA_PLAY_PAUSE; break;
                    case SC2_RELEASED_PART3_ACPI_POWER: keypress.keycode = KEY_ACPI_POWER; break;
                    case SC2_RELEASED_PART3_MEDIA_WWW_BACK: keypress.keycode = KEY_MEDIA_WWW_BACK; break;
                    case SC2_RELEASED_PART3_MEDIA_WWW_HOME: keypress.keycode = KEY_MEDIA_WWW_HOME; break;
                    case SC2_RELEASED_PART3_MEDIA_STOP: keypress.keycode = KEY_MEDIA_STOP; break;
                    case SC2_RELEASED_PART3_ACPI_SLEEP: keypress.keycode = KEY_ACPI_SLEEP; break;
                    case SC2_RELEASED_PART3_MEDIA_MY_COMPUTER: keypress.keycode = KEY_MEDIA_MY_COMPUTER; break;
                    case SC2_RELEASED_PART3_MEDIA_EMAIL: keypress.keycode = KEY_MEDIA_EMAIL; break;
                    case SC2_RELEASED_PART3_KEYPAD_BACKSLASH: keypress.keycode = KEYPAD_BACKSLASH; break;
                    case SC2_RELEASED_PART3_MEDIA_NEXT_TRACK: keypress.keycode = KEY_MEDIA_NEXT_TRACK; break;
                    case SC2_RELEASED_PART3_MEDIA_SELECT: keypress.keycode = KEY_MEDIA_SELECT; break;
                    case SC2_RELEASED_PART3_KEYPAD_ENTER: keypress.keycode = KEYPAD_ENTER; break;
                    case SC2_RELEASED_PART3_ACPI_WAKE: keypress.keycode = KEY_ACPI_WAKE; break;
                    case SC2_RELEASED_PART3_END: keypress.keycode = KEY_END; break;
                    case SC2_RELEASED_PART3_CURSOR_LEFT: keypress.keycode = KEY_ARROW_LEFT; break;
                    case SC2_RELEASED_PART3_HOME: keypress.keycode = KEY_HOME; break;
                    case SC2_RELEASED_PART3_INSERT: keypress.keycode = KEY_INSERT; break;
                    case SC2_RELEASED_PART3_DELETE: keypress.keycode = KEY_DELETE; break;
                    case SC2_RELEASED_PART3_CURSOR_DOWN: keypress.keycode = KEY_ARROW_DOWN; break;
                    case SC2_RELEASED_PART3_CURSOR_RIGHT: keypress.keycode = KEY_ARROW_RIGHT; break;
                    case SC2_RELEASED_PART3_CURSOR_UP: keypress.keycode = KEY_ARROW_UP; break;
                    case SC2_RELEASED_PART3_PAGE_DOWN: keypress.keycode = KEY_PAGEDOWN; break;
                    case SC2_RELEASED_PART3_PAGE_UP: keypress.keycode = KEY_PAGEUP; break;
                    default: keypress.keycode = KEY_UNKNOWN; break;
                }
            } else {
                switch(byte2) {
                    case SC2_PRESSED_PART2_PRINT_SCREEN:
                        if(byte3 == SC2_PRESSED_PART3_PRINT_SCREEN && byte4 == SC2_PRESSED_PART4_PRINT_SCREEN) {
                            keypress.keycode = KEY_PRINT_SCREEN;
                        } else {
                            keypress.keycode = KEY_UNKNOWN;
                        }
                        break;
                    case SC2_PRESSED_PART2_MEDIA_WWW_SEARCH: keypress.keycode = KEY_MEDIA_WWW_SEARCH; break;
                    case SC2_PRESSED_PART2_RALT: keypress.keycode = KEY_RALT; break;
                    case SC2_PRESSED_PART2_RCTRL: keypress.keycode = KEY_RCTRL; break;
                    case SC2_PRESSED_PART2_MEDIA_PREVIOUS_TRACK: keypress.keycode = KEY_MEDIA_PREVIOUS_TRACK; break;
                    case SC2_PRESSED_PART2_MEDIA_WWW_FAVOURITES: keypress.keycode = KEY_MEDIA_WWW_FAVOURITES; break;
                    case SC2_PRESSED_PART2_MEDIA_WWW_REFRESH: keypress.keycode = KEY_MEDIA_WWW_REFRESH; break;
                    case SC2_PRESSED_PART2_MEDIA_LGUI: keypress.keycode = KEY_MEDIA_LGUI; break;
                    case SC2_PRESSED_PART2_MEDIA_VOLUME_DOWN: keypress.keycode = KEY_MEDIA_VOLUME_DOWN; break;
                    case SC2_PRESSED_PART2_MEDIA_MUTE: keypress.keycode = KEY_MEDIA_MUTE; break;
                    case SC2_PRESSED_PART2_MEDIA_RGUI: keypress.keycode = KEY_MEDIA_RGUI; break;
                    case SC2_PRESSED_PART2_MEDIA_WWW_STOP: keypress.keycode = KEY_MEDIA_WWW_STOP; break;
                    case SC2_PRESSED_PART2_MEDIA_CALCULATOR: keypress.keycode = KEY_MEDIA_CALCULATOR; break;
                    case SC2_PRESSED_PART2_APPS: keypress.keycode = KEY_APPS; break;
                    case SC2_PRESSED_PART2_MEDIA_FORWARD: keypress.keycode = KEY_MEDIA_FORWARD; break;
                    case SC2_PRESSED_PART2_MEDIA_VOLUME_UP: keypress.keycode = KEY_MEDIA_VOLUME_UP; break;
                    case SC2_PRESSED_PART2_MEDIA_PLAY_PAUSE: keypress.keycode = KEY_MEDIA_PLAY_PAUSE; break;
                    case SC2_PRESSED_PART2_ACPI_POWER: keypress.keycode = KEY_ACPI_POWER; break;
                    case SC2_PRESSED_PART2_MEDIA_WWW_BACK: keypress.keycode = KEY_MEDIA_WWW_BACK; break;
                    case SC2_PRESSED_PART2_MEDIA_WWW_HOME: keypress.keycode = KEY_MEDIA_WWW_HOME; break;
                    case SC2_PRESSED_PART2_MEDIA_STOP: keypress.keycode = KEY_MEDIA_STOP; break;
                    case SC2_PRESSED_PART2_ACPI_SLEEP: keypress.keycode = KEY_ACPI_SLEEP; break;
                    case SC2_PRESSED_PART2_MEDIA_MY_COMPUTER: keypress.keycode = KEY_MEDIA_MY_COMPUTER; break;
                    case SC2_PRESSED_PART2_MEDIA_EMAIL: keypress.keycode = KEY_MEDIA_EMAIL; break;
                    case SC2_PRESSED_PART2_KEYPAD_BACKSLASH: keypress.keycode = KEYPAD_BACKSLASH; break;
                    case SC2_PRESSED_PART2_MEDIA_NEXT_TRACK: keypress.keycode = KEY_MEDIA_NEXT_TRACK; break;
                    case SC2_PRESSED_PART2_MEDIA_SELECT: keypress.keycode = KEY_MEDIA_SELECT; break;
                    case SC2_PRESSED_PART2_KEYPAD_ENTER: keypress.keycode = KEYPAD_ENTER; break;
                    case SC2_PRESSED_PART2_ACPI_WAKE: keypress.keycode = KEY_ACPI_WAKE; break;
                    case SC2_PRESSED_PART2_END: keypress.keycode = KEY_END; break;
                    case SC2_PRESSED_PART2_CURSOR_LEFT: keypress.keycode = KEY_ARROW_LEFT; break;
                    case SC2_PRESSED_PART2_HOME: keypress.keycode = KEY_HOME; break;
                    case SC2_PRESSED_PART2_INSERT: keypress.keycode = KEY_INSERT; break;
                    case SC2_PRESSED_PART2_DELETE: keypress.keycode = KEY_DELETE; break;
                    case SC2_PRESSED_PART2_CURSOR_DOWN: keypress.keycode = KEY_ARROW_DOWN; break;
                    case SC2_PRESSED_PART2_CURSOR_RIGHT: keypress.keycode = KEY_ARROW_RIGHT; break;
                    case SC2_PRESSED_PART2_CURSOR_UP: keypress.keycode = KEY_ARROW_UP; break;
                    case SC2_PRESSED_PART2_PAGE_DOWN: keypress.keycode = KEY_PAGEDOWN; break;
                    case SC2_PRESSED_PART2_PAGE_UP: keypress.keycode = KEY_PAGEUP; break;
                    default: keypress.keycode = KEY_UNKNOWN; break;
                }
            }
            break;
        case SC2_PRESSED_PART1_PAUSE: // Pause key (special case)
            if(byte2 == SC2_PRESSED_PART2_PAUSE &&
               byte3 == SC2_PRESSED_PART3_PAUSE &&
               byte4 == SC2_PRESSED_PART4_PAUSE &&
               byte5 == SC2_PRESSED_PART5_PAUSE &&
               byte6 == SC2_PRESSED_PART6_PAUSE &&
               byte7 == SC2_PRESSED_PART7_PAUSE &&
               byte8 == SC2_PRESSED_PART8_PAUSE
            ) {
                keypress.keycode = KEY_PAUSE;
            } else {
                keypress.keycode = KEY_UNKNOWN;
            }
            break;
        default:
            keypress.keycode = KEY_UNKNOWN;
            keypress.pressed = FALSE;
            break;
    }

    return keypress;
}


U8 KEYPRESS_TO_CHARS(U32 kcode) {
    static U8 str = 0;
    switch(kcode) {
        case KEY_A: str = modifiers.shift || modifiers.capslock ? 'A' : 'a'; break;
        case KEY_B: str = modifiers.shift || modifiers.capslock ? 'B' : 'b'; break;
        case KEY_C: str = modifiers.shift || modifiers.capslock ? 'C' : 'c'; break;
        case KEY_D: str = modifiers.shift || modifiers.capslock ? 'D' : 'd'; break;
        case KEY_E: str = modifiers.shift || modifiers.capslock ? 'E' : 'e'; break;
        case KEY_F: str = modifiers.shift || modifiers.capslock ? 'F' : 'f'; break;
        case KEY_G: str = modifiers.shift || modifiers.capslock ? 'G' : 'g'; break;
        case KEY_H: str = modifiers.shift || modifiers.capslock ? 'H' : 'h'; break;
        case KEY_I: str = modifiers.shift || modifiers.capslock ? 'I' : 'i'; break;
        case KEY_J: str = modifiers.shift || modifiers.capslock ? 'J' : 'j'; break;
        case KEY_K: str = modifiers.shift || modifiers.capslock ? 'K' : 'k'; break;
        case KEY_L: str = modifiers.shift || modifiers.capslock ? 'L' : 'l'; break;
        case KEY_M: str = modifiers.shift || modifiers.capslock ? 'M' : 'm'; break;
        case KEY_N: str = modifiers.shift || modifiers.capslock ? 'N' : 'n'; break;
        case KEY_O: str = modifiers.shift || modifiers.capslock ? 'O' : 'o'; break;
        case KEY_P: str = modifiers.shift || modifiers.capslock ? 'P' : 'p'; break;
        case KEY_Q: str = modifiers.shift || modifiers.capslock ? 'Q' : 'q'; break;
        case KEY_R: str = modifiers.shift || modifiers.capslock ? 'R' : 'r'; break;
        case KEY_S: str = modifiers.shift || modifiers.capslock ? 'S' : 's'; break;
        case KEY_T: str = modifiers.shift || modifiers.capslock ? 'T' : 't'; break;
        case KEY_U: str = modifiers.shift || modifiers.capslock ? 'U' : 'u'; break;
        case KEY_V: str = modifiers.shift || modifiers.capslock ? 'V' : 'v'; break;
        case KEY_W: str = modifiers.shift || modifiers.capslock ? 'W' : 'w'; break;
        case KEY_X: str = modifiers.shift || modifiers.capslock ? 'X' : 'x'; break;
        case KEY_Y: str = modifiers.shift || modifiers.capslock ? 'Y' : 'y'; break;
        case KEY_Z: str = modifiers.shift || modifiers.capslock ? 'Z' : 'z'; break;

        case KEY_1: 
        case KEYPAD_1:
            str = '1'; break;
        case KEY_2: 
        case KEYPAD_2:
            str = '2'; break;
        case KEY_3: 
        case KEYPAD_3:
            str = '3'; break;
        case KEY_4: 
        case KEYPAD_4:
            str = '4'; break;
        case KEY_5: 
        case KEYPAD_5:
            str = '5'; break;
        case KEY_6: 
        case KEYPAD_6:
            str = '6'; break;
        case KEY_7: 
        case KEYPAD_7:
            str = '7'; break;
        case KEY_8: 
        case KEYPAD_8:
            str = '8'; break;
        case KEY_9: 
        case KEYPAD_9:
            str = '9'; break;
        case KEY_0: 
        case KEYPAD_0:
            str = '0'; break;
        case KEY_SPACE: str  = ' '; break;
        case KEY_ENTER: str = '\n'; break;
        case KEY_TAB: str = '\t'; break;
        case KEY_GRAVE: str = '`'; break;
        case KEY_MINUS: str = '-'; break;
        case KEY_EQUALS: str = '='; break;
        case KEY_LBRACKET: str = '['; break;
        case KEY_RBRACKET: str = ']'; break;
        case KEY_BACKSLASH: str = '\\'; break;
        case KEY_SEMICOLON: str = ';'; break;
        case KEY_APOSTROPHE: str = '\''; break;
        case KEY_COMMA: str = ','; break;
        case KEY_DOT: str = '.'; break;
        case KEY_SLASH: str = '/'; break;
        case KEY_EXCLAMATION: str = '!'; break;
        case KEY_AT: str = '@'; break;
        case KEY_HASH: str = '#'; break;
        case KEY_DOLLAR: str = '$'; break;
        case KEY_PERCENT: str = '%'; break;
        case KEY_CARET: str = '^'; break;
        case KEY_AMPERSAND: str = '&'; break;
        case KEY_ASTERISK: str = '*'; break;
        case KEY_LEFT_PAREN: str = '('; break;
        case KEY_RIGHT_PAREN: str = ')'; break;
        case KEY_UNDERSCORE: str = '_'; break;
        case KEY_PLUS: str = '+'; break;
        case KEY_TILDE: str = '~'; break;
        case KEY_LEFT_CURLY: str = '{'; break;
        case KEY_RIGHT_CURLY: str = '}'; break;
        case KEY_PIPE: str = '|'; break;
        case KEY_COLON: str = ':'; break;
        case KEY_QUOTE: str = '"'; break;
        case KEY_LESS: str = '<'; break;
        case KEY_GREATER: str = '>'; break;
        case KEY_QUESTION: str = '?'; break;
        default:
            str = '\0'; // Non-printable key
            break;
    }


    return str;
}

MODIFIERS *GET_KEYBOARD_MODIFIERS(VOID) {
    return &modifiers;
}