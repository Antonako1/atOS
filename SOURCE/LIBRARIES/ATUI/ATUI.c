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
       display.back_buffer[i].bg == display.bg) {
        // no change, just move cursor
        atui_cursor_move(y, x + 1);
        return;
    }
    display.back_buffer[i].c  = c;
    display.back_buffer[i].fg = display.fg;
    display.back_buffer[i].bg = display.bg;
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
        display.back_buffer[i].c  = ' ';
        display.back_buffer[i].fg = display.fg;
        display.back_buffer[i].bg = display.bg;
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
    }
    
    ATUI_REFRESH();

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
