#define ATUI_INTERNALS
#include <LIBRARIES/ATUI/ATUI_TYPES.h>
#include <LIBRARIES/ATUI/ATUI.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>
#include <STD/PROC_COM.h>
#include <STD/MEM.h>
#include <STD/GRAPHICS.h>
#include <STD/DEBUG.h>
#include <STD/BINARY.h>
#include <STD/IO.h>


static ATUI_DISPLAY display ATTRIB_DATA = { 0 };
static BOOL8 display_initialized ATTRIB_DATA = { 0 };

#define INIT_CHECK_VOID() if(!display_initialized) return;
#define INIT_CHECK_RET(x) if(!display_initialized) return x;

static VOID atui_cursor_move(U32 new_row, U32 new_col) {
    ATUI_DISPLAY *d = &display;

    if (new_col >= d->cols) new_col = d->cols - 1;
    if (new_row >= d->rows) new_row = d->rows - 1;

    // mark old position dirty
    U32 old = d->cursor.row * d->cols + d->cursor.col;
    d->dirty[old] = TRUE;

    d->cursor.prev_col = d->cursor.col;
    d->cursor.prev_row = d->cursor.row;

    d->cursor.col = new_col;
    d->cursor.row = new_row;

    // mark new position dirty
    U32 now = d->cursor.row * d->cols + d->cursor.col;
    d->dirty[now] = TRUE;
}

static VOID atui_shift_left(U32 row, U32 col) {
    ATUI_DISPLAY *d = &display;

    for (U32 x = col; x < d->cols - 1; x++) {
        U32 dst = row * d->cols + x;
        U32 src = row * d->cols + (x + 1);

        d->back_buffer[dst] = d->back_buffer[src];
        d->dirty[dst] = TRUE;
    }

    // clear last char
    U32 last = row * d->cols + (d->cols - 1);
    d->back_buffer[last].c = ' ';
    d->dirty[last] = TRUE;
}

static VOID atui_shift_right(U32 row, U32 col) {
    ATUI_DISPLAY *d = &display;

    // nothing to do at last column
    if (col >= d->cols - 1)
        return;

    for (U32 x = d->cols - 1; x > col; x--) {
        U32 dst = row * d->cols + x;
        U32 src = row * d->cols + (x - 1);

        d->back_buffer[dst] = d->back_buffer[src];
        d->dirty[dst] = TRUE;
    }

    // clear inserted position (important!)
    U32 insert = row * d->cols + col;
    d->back_buffer[insert].c = ' ';
    d->dirty[insert] = TRUE;
}

static INT atui_process_key(PS2_KB_DATA *kp) {
    if (!kp) return -1;

    U32 code = kp->cur.keycode;

    if (IS_FLAG_SET(display.cnf, ATUIC_RAW)) {
        return code;
    }

    switch (code) {
        case KEY_DELETE:
        {
            U32 row = display.cursor.row;
            U32 col = display.cursor.col;

            atui_shift_left(row, col);

            for (U32 i = col; i < display.cols; i++) {
                display.dirty[row * display.cols + i] = TRUE;
            }

            return -1;
        }


        case KEY_ARROW_LEFT:
            if (display.cursor.col > 0)
                atui_cursor_move(display.cursor.row, display.cursor.col - 1);
            return -1;

        case KEY_ARROW_RIGHT:
            if (display.cursor.col < display.cols - 1) {
                atui_cursor_move(display.cursor.row, display.cursor.col + 1);
            } else if (display.cursor.row < display.rows - 1) {
                atui_cursor_move(display.cursor.row + 1, 0);
            }
            return -1;

        case KEY_ARROW_UP:
        {
            if (display.cursor.row > 0) {
                U32 target_col = display.cursor.col;
                atui_cursor_move(display.cursor.row - 1, target_col);
            }
            return -1;
        }

        case KEY_ARROW_DOWN:
        {
            if (display.cursor.row < display.rows - 1) {
                U32 target_col = display.cursor.col;
                atui_cursor_move(display.cursor.row + 1, target_col);
            }
            return -1;
        }

        case KEY_BACKSPACE:
            return '\b';

        case KEY_ENTER:
            return '\n';
        case KEY_INSERT:
            if(IS_FLAG_SET(display.cnf, ATUIC_INSERT_MODE)) {
                FLAG_UNSET(display.cnf, ATUIC_INSERT_MODE);
            } else {
                FLAG_SET(display.cnf, ATUIC_INSERT_MODE);
            }
            // Mark cursor dirty so the block/underline shape changes immediately
            display.dirty[display.cursor.row * display.cols + display.cursor.col] = TRUE;
            return -1;
    }

    CHAR c = keypress_to_char(code);
    return c ? c : -1;
}

ATUI_DISPLAY *GET_DISPLAY() {
    INIT_CHECK_RET(NULLPTR);
    return &display;
}

VOID ATUI_DESTROY() {
    INIT_CHECK_VOID();
    if(display.font.file) {
        FCLOSE(display.font.file);
        display.font.file = NULLPTR;
    }

    if(display.dirty) {
        MFree(display.dirty);
        display.dirty = NULLPTR;
    }
    display.dirty_sz = 0;
    if(display.back_buffer) {
        MFree(display.back_buffer);
        display.back_buffer = NULLPTR;
    }
    display.buffer_sz = 0;

    display_initialized = FALSE;
}

BOOL ATUI_INITIALIZE(PU8 fnt, ATUI_CONF cnf) {
    if(display_initialized) return TRUE;

    BOOL ret_val = FALSE;

    MEMZERO(&display, sizeof(ATUI_DISPLAY));

    // init font
    PU8 font_file = fnt ? fnt : "/HOME/FONTS/SYS_FONT.FNT";

    display.font.char_height = 16;
    display.font.char_width  = 8;

    display.cols = SCREEN_WIDTH  / display.font.char_width;
    display.rows = SCREEN_HEIGHT / display.font.char_height;

    display.font.file = FOPEN(font_file, MODE_FR);
    if (!display.font.file || display.font.file->sz == 0) goto err;

    display.font.glyph_cnt = display.font.file->sz / display.font.char_height;

    // allocate buffers
    display.dirty_sz = display.cols * display.rows;
    display.buffer_sz = display.dirty_sz * sizeof(CHAR_CELL);
    display.back_buffer = MAlloc(display.buffer_sz);
    if (!display.back_buffer) goto err;
    MEMZERO(display.back_buffer, display.buffer_sz);

    display.dirty = MAlloc(display.dirty_sz);
    if(!display.dirty) goto err;
    MEMSET_OPT(display.dirty, TRUE, display.dirty_sz);
    
    // tcb
    display.t = GET_CURRENT_TCB();
    
    // cursor
    display.cursor.col = 0;
    display.cursor.row = 0;

    // conf
    display.cnf = cnf;

    // colours
    display.fg = VBE_WHITE;
    display.bg = VBE_BLACK;
    display.current_attrs = ATUI_A_NORMAL;

    // background cell
    display.bkgd.c = ' ';
    display.bkgd.fg = VBE_WHITE;
    display.bkgd.bg = VBE_BLACK;
    display.bkgd.attrs = ATUI_A_NORMAL;

    // input timeout (default: blocking)
    display.input_timeout = -1;

    ATUI_CLEAR();

    ret_val = TRUE;
    display_initialized = TRUE;

    ATUI_REFRESH(); // refresh to blank screen
err:
    if (!ret_val) {
        ATUI_DESTROY();
        DRAW_8x8_STRING(0,0, "ATUI INITIALIZATION ERROR", VBE_WHITE, VBE_BLACK);
    }
    return ret_val;
}

VOID ATUI_REFRESH() {
    INIT_CHECK_VOID();
    ATUI_COPY_TO_TCB_FRAMEBUFFER();
}

VOID ATUI_MOVE(U32 y, U32 x) {
    INIT_CHECK_VOID();

    if (x >= display.cols) x = display.cols - 1;
    if (y >= display.rows) y = display.rows - 1;

    // mark old cursor dirty
    U32 old = display.cursor.row * display.cols + display.cursor.col;
    display.dirty[old] = TRUE;

    display.cursor.prev_col = display.cursor.col;
    display.cursor.prev_row = display.cursor.row;

    display.cursor.col = x;
    display.cursor.row = y;

    // mark new cursor dirty
    U32 now = y * display.cols + x;
    display.dirty[now] = TRUE;
}

VOID ATUI_GETYX(U32 *y, U32 *x){
    INIT_CHECK_VOID();
    *y = display.cursor.row;
    *x = display.cursor.col;
}

VOID ATUI_CURSOR_SET(BOOL visible) {
    INIT_CHECK_VOID();
    if(visible)
        FLAG_SET(display.cnf, ATUIC_CURSOR_VISIBLE);
    else
        FLAG_UNSET(display.cnf, ATUIC_CURSOR_VISIBLE);
}

VOID ATUI_SET_FG(VBE_COLOUR color) {
    INIT_CHECK_VOID();
    display.fg = color;
}

VOID ATUI_SET_BG(VBE_COLOUR color) {
    INIT_CHECK_VOID();
    display.bg = color;
}

VOID ATUI_SET_COLOR(VBE_COLOUR fg, VBE_COLOUR bg) {
    INIT_CHECK_VOID();
    display.fg = fg;
    display.bg = bg;
}

/* ================= ATTRIBUTES ================= */

VOID ATUI_ATTRON(U8 attrs) {
    INIT_CHECK_VOID();
    display.current_attrs |= attrs;
}

VOID ATUI_ATTROFF(U8 attrs) {
    INIT_CHECK_VOID();
    display.current_attrs &= ~attrs;
}

VOID ATUI_ATTRSET(U8 attrs) {
    INIT_CHECK_VOID();
    display.current_attrs = attrs;
}

U8 ATUI_ATTRGET() {
    INIT_CHECK_RET(0);
    return display.current_attrs;
}

/* ================= BACKGROUND CHARACTER ================= */

VOID ATUI_BKGD(CHAR_CELL cell) {
    INIT_CHECK_VOID();
    display.bkgd = cell;
}

CHAR_CELL ATUI_GETBKGD() {
    CHAR_CELL empty = { 0 };
    INIT_CHECK_RET(empty);
    return display.bkgd;
}

VOID ATUI_ADDCH(CHAR c) {
    INIT_CHECK_VOID();

    U32 x = display.cursor.col;
    U32 y = display.cursor.row;

    // --- NEWLINE ---
    if (c == '\n') {
        if (display.cursor.row < display.rows - 1)
            atui_cursor_move(display.cursor.row + 1, 0);
        else
            atui_cursor_move(display.cursor.row, 0);
        return;
    }

    // --- CARRIAGE RETURN ---
    if (c == '\r') {
        display.cursor.col = 0;
        return;
    }

    // --- BACKSPACE ---
    if (c == '\b') {
        if (x > 0) {
            x--;
        } else if (y > 0) {
            y--;
            x = display.cols - 1;
        }

        atui_shift_left(y, x);

        // mark entire row dirty (safe + simple)
        for (U32 i = 0; i < display.cols; i++) {
            display.dirty[y * display.cols + i] = TRUE;
        }

        atui_cursor_move(y, x);
        return;
    }

    // --- NORMAL CHARACTER ---
    if (IS_FLAG_SET(display.cnf, ATUIC_INSERT_MODE)) {
        if (x < display.cols - 1) {
            atui_shift_right(y, x);

            for (U32 i = x; i < display.cols; i++) {
                display.dirty[y * display.cols + i] = TRUE;
            }
        }
    }

    U32 i = y * display.cols + x;
    if(display.back_buffer[i].c == c &&
       display.back_buffer[i].fg == display.fg &&
       display.back_buffer[i].bg == display.bg &&
       display.back_buffer[i].attrs == display.current_attrs) {
        // no change, just move cursor
        atui_cursor_move(y, x + 1);
        return;
    }
    display.back_buffer[i].c  = c;
    display.back_buffer[i].fg = display.fg;
    display.back_buffer[i].bg = display.bg;
    display.back_buffer[i].attrs = display.current_attrs;
    display.dirty[i] = TRUE;

    // advance cursor
    x++;
    if (x >= display.cols) {
        x = 0;
        if (y < display.rows - 1)
            y++;
    }

    atui_cursor_move(y, x);
}

VOID ATUI_CLEAR() {
    INIT_CHECK_VOID();
    // Initialize colours to buffer
    for (U32 i = 0; i < display.cols * display.rows; i++) {
        display.back_buffer[i].c  = display.bkgd.c ? display.bkgd.c : ' ';
        display.back_buffer[i].fg = display.fg;
        display.back_buffer[i].bg = display.bg;
        display.back_buffer[i].attrs = ATUI_A_NORMAL;
        display.dirty[i] = TRUE;
    }
}

VOID ATUI_CLRTOEOL() {
    INIT_CHECK_VOID();

    U32 row = display.cursor.row;
    U32 col = display.cursor.col;

    U32 pos = row * display.cols + col;

    for (; col < display.cols; col++, pos++) {
        display.back_buffer[pos].c  = ' ';
        display.back_buffer[pos].fg = display.fg;
        display.back_buffer[pos].bg = display.bg;
        display.back_buffer[pos].attrs = ATUI_A_NORMAL;
        display.dirty[pos] = TRUE;
    }
}

VOID ATUI_CLRTOBOT() {
    INIT_CHECK_VOID();

    U32 row = display.cursor.row;
    U32 col = display.cursor.col;

    U32 pos = row * display.cols + col;
    U32 end = display.cols * display.rows;

    for (; pos < end; pos++) {
        display.back_buffer[pos].c  = ' ';
        display.back_buffer[pos].fg = display.fg;
        display.back_buffer[pos].bg = display.bg;
        display.back_buffer[pos].attrs = ATUI_A_NORMAL;
        display.dirty[pos] = TRUE;
    }
}


VOID ATUI_ADDSTR(CHAR *str){
    if(!str) return;
    for(U32 i = 0; i < STRLEN(str); i++) {
        ATUI_ADDCH(str[i]);
    }
}

VOID ATUI_ECHO(BOOL enable) {
    INIT_CHECK_VOID();
    if(enable)
        FLAG_SET(display.cnf, ATUIC_ECHO);
    else
        FLAG_UNSET(display.cnf, ATUIC_ECHO);
}

VOID ATUI_RAW(BOOL enable) {
    INIT_CHECK_VOID();
    if(enable)
        FLAG_SET(display.cnf, ATUIC_RAW);
    else
        FLAG_UNSET(display.cnf, ATUIC_RAW);
}

typedef struct {
    ATUI_DISPLAY *d;
} ATUI_PRINT_CTX;

VOID atui_putch(CHAR c, VOID *ctx) {
    ATUI_PRINT_CTX *p = (ATUI_PRINT_CTX*)ctx;
    ATUI_ADDCH(c);
}

VOID ATUI_PRINTW(CHAR *fmt, ...) {
    INIT_CHECK_VOID();
    if (!fmt) return;

    va_list args;
    va_start(args, fmt);

    ATUI_PRINT_CTX ctx = { .d = &display };
    VFORMAT(atui_putch, &ctx, fmt, args);

    va_end(args);
}

VOID ATUI_MVPRINTW(U32 y, U32 x, CHAR *fmt, ...) {
    INIT_CHECK_VOID();
    if (!fmt) return;

    ATUI_MOVE(y, x);

    va_list args;
    va_start(args, fmt);

    ATUI_PRINT_CTX ctx = { .d = &display };
    VFORMAT(atui_putch, &ctx, fmt, args);

    va_end(args);
}

VOID ATUI_MVADDSTR(U32 y, U32 x, CHAR *str) {
    INIT_CHECK_VOID();
    if (!str) return;

    ATUI_MOVE(y, x);
    ATUI_ADDSTR(str);
}


PS2_KB_DATA* ATUI_GETCH() {
    PS2_KB_DATA *kp;

    I32 timeout = display.input_timeout;

    // Non-blocking mode (timeout == 0)
    if (timeout == 0) {
        return ATUI_GETCH_NB();
    }

    // Timed mode (timeout > 0)
    if (timeout > 0) {
        U32 deadline = GET_PIT_TICKS() + (U32)timeout;
        while (GET_PIT_TICKS() < deadline) {
            kp = kb_poll();
            if (!kp || !kp->cur.pressed) {
                YIELD();
                continue;
            }
            INT result = atui_process_key(kp);
            if (IS_FLAG_SET(display.cnf, ATUIC_ECHO)) {
                if (result >= 0 && result < 256)
                    ATUI_ADDCH(result);
            }
            ATUI_REFRESH();
            return kp;
        }
        return NULLPTR; // timeout expired
    }

    // Blocking mode (timeout < 0, default)
    while (1) {
        kp = kb_poll();
        if (!kp) {
            YIELD();
            continue;
        }

        if (!kp->cur.pressed) continue;

        INT result = atui_process_key(kp);

        if (IS_FLAG_SET(display.cnf, ATUIC_ECHO)) {
            if (result >= 0 && result < 256) {
                ATUI_ADDCH(result);
            }
        }
        
        ATUI_REFRESH();

        return kp;
    }
}

PS2_KB_DATA* ATUI_GETCH_NB() {
    PS2_KB_DATA *kp = kb_poll();
    if (!kp || !kp->cur.pressed) return NULLPTR;
    INT result = atui_process_key(kp);
    
    if (IS_FLAG_SET(display.cnf, ATUIC_ECHO)) {
        if (result >= 0 && result < 256) {
            ATUI_ADDCH(result);
        }
        ATUI_REFRESH();
    }

    return kp;
}

/* ================================================================ */
/*  mouse support                                                   */
/* ================================================================ */

VOID ATUI_MOUSE_ENABLE(BOOL enable) {
    INIT_CHECK_VOID();
    if (enable)
        FLAG_SET(display.cnf, ATUIC_MOUSE_ENABLED);
    else
        FLAG_UNSET(display.cnf, ATUIC_MOUSE_ENABLED);
}

ATUI_MOUSE* ATUI_MOUSE_POLL() {
    INIT_CHECK_RET(NULLPTR);
    if (!IS_FLAG_SET(display.cnf, ATUIC_MOUSE_ENABLED)) return NULLPTR;

    PS2_MOUSE_DATA *ms = mouse_poll();
    if (!ms) return NULLPTR;

    ATUI_MOUSE *m = &display.mouse;

    m->prev_col = m->cell_col;
    m->prev_row = m->cell_row;

    m->px_x = ms->cur.x;
    m->px_y = ms->cur.y;

    // convert pixel coords to character cell coords
    m->cell_col = m->px_x / display.font.char_width;
    m->cell_row = m->px_y / display.font.char_height;

    if (m->cell_col >= display.cols) m->cell_col = display.cols - 1;
    if (m->cell_row >= display.rows) m->cell_row = display.rows - 1;

    // edge-detect clicks (was released, now pressed)
    BOOL prev_btn1 = m->btn1;
    BOOL prev_btn2 = m->btn2;

    m->btn1 = ms->cur.mouse1;
    m->btn2 = ms->cur.mouse2;
    m->btn3 = ms->cur.mouse3;

    m->clicked  = (m->btn1 && !prev_btn1);
    m->rclicked = (m->btn2 && !prev_btn2);

    m->dragging = (m->btn1 && prev_btn1 &&
                   (m->cell_col != m->prev_col || m->cell_row != m->prev_row));

    return m;
}

ATUI_MOUSE* ATUI_MOUSE_GET() {
    INIT_CHECK_RET(NULLPTR);
    return &display.mouse;
}

/* ================================================================ */
/*  inch / instr — read characters from screen                      */
/* ================================================================ */

CHAR_CELL ATUI_INCH() {
    CHAR_CELL empty = { 0 };
    INIT_CHECK_RET(empty);
    U32 idx = display.cursor.row * display.cols + display.cursor.col;
    return display.back_buffer[idx];
}

CHAR_CELL ATUI_MVINCH(U32 y, U32 x) {
    CHAR_CELL empty = { 0 };
    INIT_CHECK_RET(empty);
    if (y >= display.rows || x >= display.cols) return empty;
    return display.back_buffer[y * display.cols + x];
}

U32 ATUI_INSTR(CHAR *buf, U32 maxlen) {
    INIT_CHECK_RET(0);
    if (!buf || maxlen == 0) return 0;
    U32 row = display.cursor.row;
    U32 col = display.cursor.col;
    U32 n = 0;
    for (U32 x = col; x < display.cols && n < maxlen - 1; x++, n++) {
        buf[n] = display.back_buffer[row * display.cols + x].c;
    }
    buf[n] = '\0';
    return n;
}

U32 ATUI_MVINSTR(U32 y, U32 x, CHAR *buf, U32 maxlen) {
    INIT_CHECK_RET(0);
    ATUI_MOVE(y, x);
    return ATUI_INSTR(buf, maxlen);
}

/* ================================================================ */
/*  touch / untouch                                                 */
/* ================================================================ */

VOID ATUI_TOUCHSCREEN() {
    INIT_CHECK_VOID();
    MEMSET_OPT(display.dirty, TRUE, display.dirty_sz);
}

VOID ATUI_UNTOUCHSCREEN() {
    INIT_CHECK_VOID();
    MEMSET_OPT(display.dirty, FALSE, display.dirty_sz);
}

/* ================================================================ */
/*  cursor_set (expanded: 0=invisible, 1=underline, 2=block)        */
/* ================================================================ */

VOID ATUI_CURS_SET(U32 visibility) {
    INIT_CHECK_VOID();
    switch (visibility) {
        case 0: // invisible
            FLAG_UNSET(display.cnf, ATUIC_CURSOR_VISIBLE);
            break;
        case 1: // normal (underline)
            FLAG_SET(display.cnf, ATUIC_CURSOR_VISIBLE);
            FLAG_UNSET(display.cnf, ATUIC_INSERT_MODE);
            break;
        case 2: // very visible (block)
            FLAG_SET(display.cnf, ATUIC_CURSOR_VISIBLE);
            FLAG_SET(display.cnf, ATUIC_INSERT_MODE);
            break;
    }
    // Mark cursor position dirty so shape changes immediately
    display.dirty[display.cursor.row * display.cols + display.cursor.col] = TRUE;
}

/* ================================================================ */
/*  timeout — configurable GETCH timeout                            */
/* ================================================================ */

VOID ATUI_TIMEOUT(I32 ms) {
    INIT_CHECK_VOID();
    display.input_timeout = ms;
}

/* ================================================================ */
/*  getstr — read a line of input                                   */
/* ================================================================ */

U32 ATUI_GETSTR(CHAR *buf, U32 maxlen) {
    INIT_CHECK_RET(0);
    if (!buf || maxlen == 0) return 0;

    U32 len = 0;
    while (len < maxlen - 1) {
        PS2_KB_DATA *kp = ATUI_GETCH();
        if (!kp) continue;

        U32 code = kp->cur.keycode;
        if (code == KEY_ENTER) break;
        if (code == KEY_ESC) { len = 0; break; }

        CHAR c = keypress_to_char(code);
        if (c == '\b') {
            if (len > 0) {
                len--;
                if (IS_FLAG_SET(display.cnf, ATUIC_ECHO)) {
                    ATUI_ADDCH('\b');
                    ATUI_REFRESH();
                }
            }
            continue;
        }
        if (c >= 32 && c < 127) {
            buf[len++] = c;
            if (IS_FLAG_SET(display.cnf, ATUIC_ECHO)) {
                ATUI_ADDCH(c);
                ATUI_REFRESH();
            }
        }
    }
    buf[len] = '\0';
    return len;
}

/* ================================================================ */
/*  keyname — return string name for a keycode                      */
/* ================================================================ */

static CHAR keyname_buf[16] ATTRIB_DATA = { 0 };

CHAR* ATUI_KEYNAME(U32 keycode) {
    switch (keycode) {
        case KEY_ENTER:       return "KEY_ENTER";
        case KEY_ESC:         return "KEY_ESC";
        case KEY_BACKSPACE:   return "KEY_BACKSPACE";
        case KEY_TAB:         return "KEY_TAB";
        case KEY_DELETE:      return "KEY_DELETE";
        case KEY_INSERT:      return "KEY_INSERT";
        case KEY_HOME:        return "KEY_HOME";
        case KEY_END:         return "KEY_END";
        case KEY_PAGEUP:      return "KEY_PGUP";
        case KEY_PAGEDOWN:    return "KEY_PGDN";
        case KEY_ARROW_UP:    return "KEY_UP";
        case KEY_ARROW_DOWN:  return "KEY_DOWN";
        case KEY_ARROW_LEFT:  return "KEY_LEFT";
        case KEY_ARROW_RIGHT: return "KEY_RIGHT";
        case KEY_F1:          return "KEY_F1";
        case KEY_F2:          return "KEY_F2";
        case KEY_F3:          return "KEY_F3";
        case KEY_F4:          return "KEY_F4";
        case KEY_F5:          return "KEY_F5";
        case KEY_F6:          return "KEY_F6";
        case KEY_F7:          return "KEY_F7";
        case KEY_F8:          return "KEY_F8";
        case KEY_F9:          return "KEY_F9";
        case KEY_F10:         return "KEY_F10";
        case KEY_F11:         return "KEY_F11";
        case KEY_F12:         return "KEY_F12";
        default: {
            CHAR c = keypress_to_char(keycode);
            if (c >= 32 && c < 127) {
                keyname_buf[0] = c;
                keyname_buf[1] = '\0';
            } else {
                SPRINTF(keyname_buf, "0x%x", keycode);
            }
            return keyname_buf;
        }
    }
}

/* ================================================================ */
/*  insdelln — insert/delete lines at cursor                        */
/* ================================================================ */

VOID ATUI_INSDELLN(I32 n) {
    INIT_CHECK_VOID();

    U32 row = display.cursor.row;
    U32 cols = display.cols;
    U32 rows = display.rows;

    if (n > 0) {
        // Insert n blank lines above cursor, pushing content down
        for (I32 r = (I32)rows - 1; r >= (I32)row + n; r--) {
            MEMCPY_OPT(&display.back_buffer[r * cols],
                        &display.back_buffer[(r - n) * cols],
                        cols * sizeof(CHAR_CELL));
            for (U32 c = 0; c < cols; c++)
                display.dirty[r * cols + c] = TRUE;
        }
        // Clear the inserted lines
        for (I32 r = row; r < (I32)row + n && r < (I32)rows; r++) {
            for (U32 c = 0; c < cols; c++) {
                U32 idx = r * cols + c;
                display.back_buffer[idx].c = ' ';
                display.back_buffer[idx].fg = display.fg;
                display.back_buffer[idx].bg = display.bg;
                display.back_buffer[idx].attrs = ATUI_A_NORMAL;
                display.dirty[idx] = TRUE;
            }
        }
    } else if (n < 0) {
        I32 del = -n;
        // Delete |n| lines at cursor, pulling content up
        for (U32 r = row; r + del < rows; r++) {
            MEMCPY_OPT(&display.back_buffer[r * cols],
                        &display.back_buffer[(r + del) * cols],
                        cols * sizeof(CHAR_CELL));
            for (U32 c = 0; c < cols; c++)
                display.dirty[r * cols + c] = TRUE;
        }
        // Clear the vacated lines at the bottom
        for (U32 r = rows - del; r < rows; r++) {
            for (U32 c = 0; c < cols; c++) {
                U32 idx = r * cols + c;
                display.back_buffer[idx].c = ' ';
                display.back_buffer[idx].fg = display.fg;
                display.back_buffer[idx].bg = display.bg;
                display.back_buffer[idx].attrs = ATUI_A_NORMAL;
                display.dirty[idx] = TRUE;
            }
        }
    }
}
