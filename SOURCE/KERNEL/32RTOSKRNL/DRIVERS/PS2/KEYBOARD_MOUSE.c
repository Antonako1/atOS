#include <DRIVERS/PS2/KEYBOARD_MOUSE.h>
#include <CPU/PIC/PIC.h>
#include <RTOSKRNL/RTOSKRNL_INTERNAL.h>
#include <CPU/ISR/ISR.h>
#include <DRIVERS/VESA/VBE.h>
#include <STD/BINARY.h>
#include <STD/MATH.h>
#include <STD/MEM.h>
#include <DEBUG/KDEBUG.h>

KEYPRESS ParsePS2_CS2(U32 scancode1, U32 scancode2, U8 scancode1_bytes, U8 scancode2_bytes);
KEYPRESS ParsePS2_CS1(U32 scancode1, U32 scancode2);
static PS2_EVENT ps2_decode_next(void);
static void update_modifiers(KEYPRESS *k, MODIFIERS *m);

static PS2KB_CMD_QUEUE cmd_queue __attribute__((section(".data"))) = {0};
static PS2_INFO ps2_info __attribute__((section(".data"))) = {0};
static volatile PS2_KB_MOUSE_DATA g_input ATTRIB_DATA = { 0 };
static PS2_DECODER_STATE dec ATTRIB_DATA = {0};


PS2_KB_MOUSE_DATA* GET_KB_MOUSE_DATA() {
    return &g_input;
}

VOID UPDATE_KP_MOUSE_DATA(void) {
    PS2_EVENT ev = ps2_decode_next();

    g_input.kb_event = FALSE;
    g_input.ms_event = FALSE;
    if (ev.type == PS2_EVT_KEY) {
        g_input.kb.prev = g_input.kb.cur;
        g_input.kb.cur  = ev.key;

        update_modifiers(&g_input.kb.cur, &g_input.kb.mods);

        g_input.kb.seq++;
        g_input.kb_event = TRUE;
    }

    else if (ev.type == PS2_EVT_MOUSE) {
        // KDEBUG_STR_HEX_LN("Mouse event! ", cmd_queue.head);
        g_input.ms.prev = g_input.ms.cur;
        g_input.ms.cur  = ev.mouse;

        g_input.ms.seq++;
        g_input.ms_event = TRUE;
    }
}


PS2_INFO *GET_PS2_INFO(VOID) {
    return &ps2_info;
}

BOOLEAN IS_CMD_QUEUE_EMPTY(VOID) {
    BOOLEAN empty = (cmd_queue.head == cmd_queue.tail);
    return empty;
}

void PUSH_TO_CMD_QUEUE(PS2_PACKET byte) {
    // CLI;
    U16 next = (cmd_queue.head + 1) % CMD_QUEUE_SIZE;
    if(next != cmd_queue.tail) {
        cmd_queue.buffer[cmd_queue.head] = byte;
        cmd_queue.head = next;
    }
    // STI;
}

PS2KB_CMD_QUEUE *GET_CMD_QUEUE(VOID) { 
    return &cmd_queue; 
}
PS2_PACKET POP_FROM_CMD_QUEUE(VOID) {
    // CLI;
    if (IS_CMD_QUEUE_EMPTY()) {
        // STI;
        return (PS2_PACKET){ 0, 0 }; // Queue is empty
    }
    PS2_PACKET byte = cmd_queue.buffer[cmd_queue.tail];
    cmd_queue.tail = (cmd_queue.tail + 1) % CMD_QUEUE_SIZE;
    // STI;
    return byte;
}
BOOLEAN IS_CMD_QUEUE_FULL(VOID) {
    return (cmd_queue.head + 1) % CMD_QUEUE_SIZE == cmd_queue.tail;
}
U0 CLEAR_CMD_QUEUE(VOID) {
    cmd_queue.head = 0;
    cmd_queue.tail = 0;
}

U8 PS2_INB(U8 port) {
    return _inb(port);
}

void PS2_MOUSE_HANDLER(I32 num, U32 errcode) {
    (void)errcode;

    // Read status and data
    U8 status = PS2_INB(PS2_STATUSPORT);
    U8 data   = PS2_INB(PS2_DATAPORT);

    // Ensure this is really a mouse byte (status bit 5)
    if(status & 0x20) {
        PUSH_TO_CMD_QUEUE((PS2_PACKET){MOUSE_PACKET, data});
    }

    pic_send_eoi(12); // slave
}

void PS2_KEYBOARD_HANDLER(I32 num, U32 errcode) {
    (void)errcode;

    U8 status = PS2_INB(PS2_STATUSPORT);
    U8 data   = PS2_INB(PS2_DATAPORT);

    if(!(status & 0x20)) { // keyboard byte
        PUSH_TO_CMD_QUEUE((PS2_PACKET){KEYBOARD_PACKET, data});
    }

    pic_send_eoi(1);
}

BOOLEAN PS2_WAIT_FOR_INPUT_CLEAR(VOID) {
    int timeout = 10000;
    while((_inb(PS2_STATUSPORT) & PS2_INPUT_BUFFER_FULL) && --timeout);
    return timeout != 0;
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
    } else if(status != PS2_SPECIAL_ACK) {
        return FALSE;
    }

    return PS2_WAIT_FOR_INPUT_CLEAR();
}

BOOLEAN PS2_SET_SCAN_CODE_SET(U8 scan_code_set) {
    if(!PS2_OUTB(PS2_DATAPORT, PS2_SET_SCANCODE_SET)) return FALSE;
    if(!PS2_OUTB(PS2_DATAPORT, scan_code_set)) return FALSE;
    return TRUE;
}

BOOLEAN PS2_KEYBOARD_RETRY_TRY(VOID) {
    if(!PS2_OUTB(PS2_DATAPORT, PS2_RESET_AND_SELF_TEST)) return FALSE;
    return PS2_INB(PS2_DATAPORT) == PS2_SPECIAL_SELF_TEST_PASSED;
}

BOOLEAN PS2_KEYBOARD_RESET(VOID) {
    for(U8 i = 0; i < 3; i++) {
        PS2_WAIT_FOR_INPUT_CLEAR();
        if(PS2_KEYBOARD_RETRY_TRY()) return TRUE;
    }
    return FALSE;
}

BOOLEAN PS2_EMPTY_KEYBOARD_BUFFER(VOID) {
    while(PS2_INB(PS2_STATUSPORT) & PS2_OUTPUT_BUFFER_FULL) {
        (void)_inb(PS2_DATAPORT);
    }
    return TRUE;
}

BOOLEAN PS2_EnableScanning(VOID) {
    if(!PS2_OUTB(PS2_DATAPORT, PS2_ENABLE_SCANNING)) return FALSE;
    return PS2_ACKNOWLEDGED();
}

BOOLEAN PS2_DisableScanning(VOID) {
    if(!PS2_OUTB(PS2_DATAPORT, PS2_DISABLE_SCANNING)) return FALSE;
    return PS2_ACKNOWLEDGED();
}

U16 PS2_Identify(VOID) {
    if(!PS2_OUTB(PS2_DATAPORT, PS2_IDENTIFY_KEYBOARD)) return 0;

    U8 first = PS2_INB(PS2_DATAPORT);
    if(first == 0xAB) {
        U8 second = PS2_INB(PS2_DATAPORT);
        return (first << 8) | second;
    }
    return first;
}

static void PS2_MOUSE_WRITE(U8 data) {
    _outb(PS2_CMDPORT, 0xD4); // Signal that next byte is for the mouse
    _outb(PS2_DATAPORT, data);
    PS2_ACKNOWLEDGED();
}

BOOLEAN PS2_SetTypematic(U8 delay, U8 rate) {
    /* Configuration Byte Bitmask:
       [7]   Always 0
       [6:5] Delay (00=250ms, 01=500ms, 10=750ms, 11=1000ms)
       [4:0] Rate (00000=30Hz, 11111=2Hz)
    */
    U8 config = ((delay & 0x03) << 5) | (rate & 0x1F);

    // Send the command byte
    if (!PS2_OUTB(PS2_DATAPORT, PS2_SET_TYPEMATIC_RATE_AND_DELAY)) return FALSE;
    
    // Send the configuration byte
    if (!PS2_OUTB(PS2_DATAPORT, config)) return FALSE;

    return TRUE;
}

BOOLEAN PS2_KEYBOARD_INIT(VOID) {
    // Disable scanning first
    if(!PS2_DisableScanning()) return FALSE;

    // Identify device
    U16 type = PS2_Identify();
    ps2_info.type = type;
    ps2_info.dual_channel = FALSE;
    ps2_info.exists = TRUE;

    // Disable both ports during setup
    _outb(PS2_CMDPORT, DISABLE_FIRST_PS2_PORT);
    _outb(PS2_CMDPORT, DISABLE_SECOND_PS2_PORT);

    // Configure controller
    _outb(PS2_CMDPORT, GET_CONTROLLER_CONFIGURATION_BYTE);
    U8 config1 = _inb(PS2_DATAPORT);
    FLAG_UNSET(config1, 0b01010010);
    _outb(PS2_CMDPORT, SET_CONTROLLER_CONFIGURATION_BYTE);
    _outb(PS2_WRITEPORT, config1);

    // Check if second port exists
    _outb(PS2_CMDPORT, ENABLE_SECOND_PS2_PORT);
    _outb(PS2_CMDPORT, GET_CONTROLLER_CONFIGURATION_BYTE);
    U8 config2 = _inb(PS2_DATAPORT);
    if(IS_FLAG_UNSET(config2, 0b00100000)) {
        ps2_info.dual_channel = TRUE;
        FLAG_UNSET(config2, 0b00100010);
        _outb(PS2_CMDPORT, SET_CONTROLLER_CONFIGURATION_BYTE);
        _outb(PS2_WRITEPORT, config2);
    }

    // Test ports
    _outb(PS2_CMDPORT, TEST_FIRST_PS2_PORT);
    ps2_info.port1_check = PS2_INB(PS2_DATAPORT);
    if(ps2_info.dual_channel) {
        _outb(PS2_CMDPORT, TEST_SECOND_PS2_PORT);
        ps2_info.port2_check = PS2_INB(PS2_DATAPORT);
    } else {
        ps2_info.port2_check = 0;
    }

    // Enable keyboard port & interrupt
    _outb(PS2_CMDPORT, ENABLE_FIRST_PS2_PORT);
    FLAG_SET(config1, 0b00000001);
    _outb(PS2_CMDPORT, SET_CONTROLLER_CONFIGURATION_BYTE);
    _outb(PS2_WRITEPORT, config1);

    // Enable mouse port interrupt if exists
    if(ps2_info.dual_channel) {
        FLAG_SET(config2, 0b00000010);
        _outb(PS2_CMDPORT, SET_CONTROLLER_CONFIGURATION_BYTE);
        _outb(PS2_WRITEPORT, config2);
    }

    // Reset keyboard
    if(!PS2_KEYBOARD_RESET()) return FALSE;

    // Set Delay to 250ms (0) and Rate to 30Hz (0)
    PS2_SetTypematic(0, 0);

    // Set scan code set
    if(!PS2_SET_SCAN_CODE_SET(DEFAULT_SCANCODESET)) {
        if(!PS2_SET_SCAN_CODE_SET(SECONDARY_SCANCODESET)) return FALSE;
        ps2_info.scancode_set = SECONDARY_SCANCODESET;
    } else {
        ps2_info.scancode_set = DEFAULT_SCANCODESET;
    }

    // Clear keyboard buffer
    if(!PS2_EMPTY_KEYBOARD_BUFFER()) return FALSE;

    // Register ISR & enable IRQ1
    ISR_REGISTER_HANDLER(PIC_REMAP_OFFSET + 1, PS2_KEYBOARD_HANDLER);
    PIC_Unmask(1);

    // Enable scanning
    if(!PS2_EnableScanning()) return FALSE;

    MEMZERO(&dec, sizeof(PS2_DECODER_STATE));
    CLEAR_CMD_QUEUE();
    return TRUE;
}

BOOLEAN PS2_MOUSE_INIT(VOID) {
    if(!ps2_info.dual_channel) return FALSE;

    // 1. Enable second PS/2 port
    _outb(PS2_CMDPORT, ENABLE_SECOND_PS2_PORT);

    // 2. Set Default Settings
    PS2_MOUSE_WRITE(0xF6);

    // 3. THE MAGIC "KNOCK" SEQUENCE FOR SCROLL WHEEL
    // This sequence (200, 100, 80) tells the mouse to enable the Z-axis
    PS2_MOUSE_WRITE(0xF3); PS2_MOUSE_WRITE(200);
    PS2_MOUSE_WRITE(0xF3); PS2_MOUSE_WRITE(100);
    PS2_MOUSE_WRITE(0xF3); PS2_MOUSE_WRITE(80);

    // 4. Check if the mouse upgraded to ID 3 (IntelliMouse)
    _outb(PS2_CMDPORT, 0xD4);
    _outb(PS2_DATAPORT, 0xF2); // Get ID command
    PS2_ACKNOWLEDGED();
    U8 mouse_id = PS2_INB(PS2_DATAPORT);

    if (mouse_id == 3) {
        ps2_info.has_wheel = TRUE;  // Add this to your PS2_INFO struct!
        KDEBUG_PUTS("[PS/2] Mouse: Scroll wheel enabled.\n");
    } else {
        ps2_info.has_wheel = FALSE;
        KDEBUG_PUTS("[PS/2] Mouse: Standard 3-button mode.\n");
    }

    // 5. Enable streaming mode
    PS2_MOUSE_WRITE(0xF4);

    MEMZERO(&g_input.ms, sizeof(PS2_MOUSE_DATA));
    // 6. Register ISR & enable IRQ12
    ISR_REGISTER_HANDLER(PIC_REMAP_OFFSET + 12, PS2_MOUSE_HANDLER);
    PIC_Unmask(12);

    CLEAR_CMD_QUEUE();
    PS2_EMPTY_KEYBOARD_BUFFER();
    return TRUE;
}

static PS2_EVENT ps2_decode_next(void) {
    PS2_EVENT ev = { PS2_EVT_NONE };

    while (!IS_CMD_QUEUE_EMPTY()) {
        // PEEK at the queue first to check alignment before popping? 
        // Or just pop and discard if invalid. Popping is safer to clear garbage.
        PS2_PACKET p = POP_FROM_CMD_QUEUE();

        if (p.device == KEYBOARD_PACKET) {
            U8 sc = p.data;
            dec.sc1 = (dec.sc1 << 8) | sc;
            dec.sc1_len++;

            if (ps2_info.scancode_set == SCANCODESET2 && (sc == 0xE0 || sc == 0xF0)) {
                continue;
            }

            KEYPRESS k = (ps2_info.scancode_set == SCANCODESET1)
                ? ParsePS2_CS1(dec.sc1, dec.sc2)
                : ParsePS2_CS2(dec.sc1, dec.sc2, dec.sc1_len, dec.sc2_len);

            dec.sc1 = dec.sc2 = dec.sc1_len = dec.sc2_len = 0;

            if (k.keycode != KEY_UNKNOWN) {
                ev.type = PS2_EVT_KEY;
                ev.key = k;
                return ev;
            }
        }
        else if (p.device == MOUSE_PACKET) {
            if (dec.mouse_cycle == 0 && !(p.data & 0x08)) {
                dec.mouse_cycle = 0;
                continue; 
            }
            if (dec.mouse_cycle >= 4) {
                dec.mouse_cycle = 0;
            }

            dec.mouse_bytes[dec.mouse_cycle++] = p.data;
            U8 packet_size = (ps2_info.has_wheel) ? 4 : 3;

            if (dec.mouse_cycle == packet_size) {
                dec.mouse_cycle = 0;
                ev.mouse = g_input.ms.cur;
                
                U8 status = dec.mouse_bytes[0];

                ev.mouse.mouse1 = (status & 0x1) ? TRUE : FALSE; // Left
                ev.mouse.mouse2 = (status & 0x2) ? TRUE : FALSE; // Right
                ev.mouse.mouse3 = (status & 0x4) ? TRUE : FALSE; // Middle

                I32 rel_x = (status & 0x10) ? (I32)((I8)dec.mouse_bytes[1]) : (I32)dec.mouse_bytes[1];
                I32 rel_y = (status & 0x20) ? (I32)((I8)dec.mouse_bytes[2]) : (I32)dec.mouse_bytes[2];

                ev.mouse.x += rel_x;
                ev.mouse.y -= rel_y; 

                if ((I32)ev.mouse.x < 0) ev.mouse.x = 0;
                if ((I32)ev.mouse.y < 0) ev.mouse.y = 0;
                if (ev.mouse.x >= SCREEN_WIDTH) ev.mouse.x = SCREEN_WIDTH - 1;
                if (ev.mouse.y >= SCREEN_HEIGHT) ev.mouse.y = SCREEN_HEIGHT - 1;

                if (ps2_info.has_wheel) {
                    I8 scroll_delta = (I8)(dec.mouse_bytes[3] & 0x0F);
                    if (scroll_delta & 0x08) scroll_delta |= 0xF0;

                    ev.mouse.scroll_wheel += scroll_delta;
                }


                ev.type = PS2_EVT_MOUSE;
                return ev;
            }
        }
    }
    return ev;
}

static void update_modifiers(KEYPRESS *k, MODIFIERS *m) {
    if (k->pressed) {
        switch (k->keycode) {
            case KEY_LSHIFT:
            case KEY_RSHIFT: m->shift = TRUE; break;
            case KEY_LCTRL:
            case KEY_RCTRL:  m->ctrl  = TRUE; break;
            case KEY_LALT:
            case KEY_RALT:   m->alt   = TRUE; break;
            case KEY_CAPSLOCK: m->capslock ^= 1; break;
        }
    } else {
        switch (k->keycode) {
            case KEY_LSHIFT:
            case KEY_RSHIFT: m->shift = FALSE; break;
            case KEY_LCTRL:
            case KEY_RCTRL:  m->ctrl  = FALSE; break;
            case KEY_LALT:
            case KEY_RALT:   m->alt   = FALSE; break;
        }
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
        case SC2_PRESSED_KEYPADMINUS: keypress.keycode = KEYPAD_SUBTRACT; break;
        case SC2_PRESSED_KEYPADASTERISK: keypress.keycode = KEYPAD_MULTIPLY; break;
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
                case SC2_RELEASED_PART2_KEYPADMINUS: keypress.keycode = KEYPAD_SUBTRACT; break;
                case SC2_RELEASED_PART2_KEYPADASTERISK: keypress.keycode = KEYPAD_MULTIPLY; break;
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
                    // case SC2_RELEASED_PART3_MEDIA_LGUI: keypress.keycode = KEY_MEDIA_LGUI; break;
                    case SC2_RELEASED_PART3_MEDIA_VOLUME_DOWN: keypress.keycode = KEY_MEDIA_VOLUME_DOWN; break;
                    case SC2_RELEASED_PART3_MEDIA_MUTE: keypress.keycode = KEY_MEDIA_MUTE; break;
                    // case SC2_RELEASED_PART3_MEDIA_RGUI: keypress.keycode = KEY_MEDIA_RGUI; break;
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
                    case SC2_RELEASED_PART3_KEYPAD_BACKSLASH: keypress.keycode = KEYPAD_DIVIDE; break;
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
                    // case SC2_PRESSED_PART2_MEDIA_LGUI: keypress.keycode = KEY_MEDIA_LGUI; break;
                    case SC2_PRESSED_PART2_MEDIA_VOLUME_DOWN: keypress.keycode = KEY_MEDIA_VOLUME_DOWN; break;
                    case SC2_PRESSED_PART2_MEDIA_MUTE: keypress.keycode = KEY_MEDIA_MUTE; break;
                    // case SC2_PRESSED_PART2_MEDIA_RGUI: keypress.keycode = KEY_MEDIA_RGUI; break;
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
                    case SC2_PRESSED_PART2_KEYPAD_BACKSLASH: keypress.keycode = KEYPAD_DIVIDE; break;
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

static const U8 keymap_lower[KEY_MAX] ATTRIB_RODATA = {
    /* Letters */
    [KEY_A] = 'a', [KEY_B] = 'b', [KEY_C] = 'c', [KEY_D] = 'd',
    [KEY_E] = 'e', [KEY_F] = 'f', [KEY_G] = 'g', [KEY_H] = 'h',
    [KEY_I] = 'i', [KEY_J] = 'j', [KEY_K] = 'k', [KEY_L] = 'l',
    [KEY_M] = 'm', [KEY_N] = 'n', [KEY_O] = 'o', [KEY_P] = 'p',
    [KEY_Q] = 'q', [KEY_R] = 'r', [KEY_S] = 's', [KEY_T] = 't',
    [KEY_U] = 'u', [KEY_V] = 'v', [KEY_W] = 'w', [KEY_X] = 'x',
    [KEY_Y] = 'y', [KEY_Z] = 'z',

    /* Number row */
    [KEY_1] = '1', [KEY_2] = '2', [KEY_3] = '3', [KEY_4] = '4',
    [KEY_5] = '5', [KEY_6] = '6', [KEY_7] = '7', [KEY_8] = '8',
    [KEY_9] = '9', [KEY_0] = '0',

    /* Punctuation */
    [KEY_GRAVE]      = '`',
    [KEY_MINUS]      = '-',
    [KEY_EQUALS]     = '=',
    [KEY_LBRACKET]   = '[',
    [KEY_RBRACKET]   = ']',
    [KEY_BACKSLASH]  = '\\',
    [KEY_SEMICOLON]  = ';',
    [KEY_APOSTROPHE] = '\'',
    [KEY_COMMA]      = ',',
    [KEY_DOT]        = '.',
    [KEY_SLASH]      = '/',

    /* Whitespace */
    [KEY_SPACE] = ' ',
    [KEY_TAB]   = '\t',
    [KEY_ENTER] = '\n',

    /* Keypad */
    [KEYPAD_0] = '0', [KEYPAD_1] = '1', [KEYPAD_2] = '2',
    [KEYPAD_3] = '3', [KEYPAD_4] = '4', [KEYPAD_5] = '5',
    [KEYPAD_6] = '6', [KEYPAD_7] = '7', [KEYPAD_8] = '8',
    [KEYPAD_9] = '9',
    [KEYPAD_DOT] = '.',
};

static const U8 keymap_upper[KEY_MAX] ATTRIB_RODATA = {
    /* Letters */
    [KEY_A] = 'A', [KEY_B] = 'B', [KEY_C] = 'C', [KEY_D] = 'D',
    [KEY_E] = 'E', [KEY_F] = 'F', [KEY_G] = 'G', [KEY_H] = 'H',
    [KEY_I] = 'I', [KEY_J] = 'J', [KEY_K] = 'K', [KEY_L] = 'L',
    [KEY_M] = 'M', [KEY_N] = 'N', [KEY_O] = 'O', [KEY_P] = 'P',
    [KEY_Q] = 'Q', [KEY_R] = 'R', [KEY_S] = 'S', [KEY_T] = 'T',
    [KEY_U] = 'U', [KEY_V] = 'V', [KEY_W] = 'W', [KEY_X] = 'X',
    [KEY_Y] = 'Y', [KEY_Z] = 'Z',

    /* Number row (shifted) */
    [KEY_1] = '!', [KEY_2] = '@', [KEY_3] = '#', [KEY_4] = '$',
    [KEY_5] = '%', [KEY_6] = '^', [KEY_7] = '&', [KEY_8] = '*',
    [KEY_9] = '(', [KEY_0] = ')',

    /* Punctuation (shifted) */
    [KEY_GRAVE]      = '~',
    [KEY_MINUS]      = '_',
    [KEY_EQUALS]     = '+',
    [KEY_LBRACKET]   = '{',
    [KEY_RBRACKET]   = '}',
    [KEY_BACKSLASH]  = '|',
    [KEY_SEMICOLON]  = ':',
    [KEY_APOSTROPHE] = '"',
    [KEY_COMMA]      = '<',
    [KEY_DOT]        = '>',
    [KEY_SLASH]      = '?',
};


U8 KEYPRESS_TO_CHARS(U32 kcode) {
    if (kcode >= KEY_MAX)
        return '\0';

    MODIFIERS *m = &g_input.kb.mods;

    /* Letters: Shift XOR CapsLock */
    if (keymap_lower[kcode] >= 'a' && keymap_lower[kcode] <= 'z') {
        BOOLEAN upper = m->shift ^ m->capslock;
        return upper ? keymap_upper[kcode] : keymap_lower[kcode];
    }

    /* Everything else: Shift selects map */
    if (m->shift && keymap_upper[kcode])
        return keymap_upper[kcode];

    return keymap_lower[kcode];
}
