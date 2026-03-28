/*
 * ANSI.c — ANSI escape sequence parser for TSHELL
 *
 * Parses SGR (Select Graphic Rendition) sequences to set per-character
 * foreground and background colors.  Also handles a handful of CSI
 * commands: \e[2J (clear), \e[H (home), \e[K (clear to EOL).
 *
 * Usage:
 *   for each byte c:
 *     if (ANSI_FEED(c, tout)) continue;   // consumed by parser
 *     // else: c is a printable char, emit it
 */

#include "TSHELL.h"
#include <STD/STRING.h>
#include <STD/MEM.h>

/* ---- ANSI parser states ---- */
#define ANSI_NORMAL  0
#define ANSI_ESC     1   /* saw \x1B              */
#define ANSI_CSI     2   /* saw \x1B[  — reading params */

/* ---- ANSI → VBE color map ---- */
/* Standard 8 colors (indices 0-7) */
static const VBE_COLOUR ansi_std_colors[8] ATTRIB_RODATA = {
    VBE_BLACK,    /* 0 */
    VBE_RED,      /* 1 */
    VBE_GREEN,    /* 2 */
    VBE_YELLOW,   /* 3 — brown/yellow */
    VBE_BLUE,     /* 4 */
    VBE_MAGENTA,  /* 5 */
    VBE_CYAN,     /* 6 */
    VBE_WHITE,    /* 7 */
};

/* Bright variants */
static const VBE_COLOUR ansi_bright_colors[8] ATTRIB_RODATA = {
    VBE_GRAY,     /* 0 — bright black = gray */
    VBE_PINK,     /* 1 — bright red          */
    VBE_LIME,     /* 2 — bright green        */
    VBE_GOLD,     /* 3 — bright yellow       */
    VBE_NAVY,     /* 4 — bright blue (navy)  */
    VBE_PURPLE,   /* 5 — bright magenta      */
    VBE_CYAN,     /* 6 — bright cyan         */
    VBE_WHITE,    /* 7 — bright white        */
};

/* ---- Reset parser ---- */
VOID ANSI_RESET(TERM_OUTPUT *t) {
    t->ansi_state       = ANSI_NORMAL;
    t->ansi_param_count = 0;
    t->ansi_cur_param   = 0;
    t->ansi_bold        = FALSE;
}

/* ---- Execute an SGR parameter ---- */
static VOID ansi_sgr(TERM_OUTPUT *t, U8 param) {
    if (param == 0) {
        /* Reset all */
        t->fg         = t->default_fg;
        t->bg         = t->default_bg;
        t->ansi_bold  = FALSE;
    } else if (param == 1) {
        t->ansi_bold = TRUE;
    } else if (param == 22) {
        t->ansi_bold = FALSE;
    } else if (param >= 30 && param <= 37) {
        /* Standard foreground */
        U8 idx = param - 30;
        t->fg = t->ansi_bold ? ansi_bright_colors[idx] : ansi_std_colors[idx];
    } else if (param == 39) {
        t->fg = t->default_fg;
    } else if (param >= 40 && param <= 47) {
        /* Standard background */
        t->bg = ansi_std_colors[param - 40];
    } else if (param == 49) {
        t->bg = t->default_bg;
    } else if (param >= 90 && param <= 97) {
        /* Bright foreground */
        t->fg = ansi_bright_colors[param - 90];
    } else if (param >= 100 && param <= 107) {
        /* Bright background */
        t->bg = ansi_bright_colors[param - 100];
    }
}

/* ---- Process a completed CSI sequence ---- */
static VOID ansi_dispatch_csi(TERM_OUTPUT *t, CHAR cmd) {
    /* Finalize the last parameter being accumulated */
    U8 params[16];
    U8 count = t->ansi_param_count;

    MEMCPY(params, t->ansi_params, count);
    /* The current param being built */
    if (count < 16) {
        params[count] = t->ansi_cur_param;
        count++;
    }

    switch (cmd) {
    case 'm':
        /* SGR — Select Graphic Rendition */
        if (count == 0 || (count == 1 && params[0] == 0)) {
            ansi_sgr(t, 0);
        } else {
            for (U8 i = 0; i < count; i++)
                ansi_sgr(t, params[i]);
        }
        break;

    case 'J':
        /* Erase in Display: \e[2J = clear screen */
        if (count >= 1 && params[0] == 2) {
            TPUT_CLS();
        }
        break;

    case 'H':
        /* Cursor Position: \e[H = home */
        /* For terminal shell we just move to top-left of output area */
        t->cursor_col = 0;
        t->cursor_row = 0;
        break;

    case 'K':
        /* Erase in Line: \e[K or \e[0K = clear to EOL */
        /* We fill the rest of the current scrollback row with spaces */
        {
            U32 row = t->scrollback.write_row;
            if (t->scrollback.total_lines > 0 && t->cursor_row < SCROLLBACK_LINES) {
                U32 sb_row = (row == 0) ? SCROLLBACK_LINES - 1 : row - 1;
                for (U32 c = t->cursor_col; c < AMOUNT_OF_COLS; c++) {
                    t->scrollback.cells[sb_row][c].c  = ' ';
                    t->scrollback.cells[sb_row][c].fg = t->fg;
                    t->scrollback.cells[sb_row][c].bg = t->bg;
                }
            }
        }
        break;

    default:
        break; /* ignore unknown */
    }
}

/* ----
 * ANSI_FEED — feed one byte into the ANSI parser.
 * Returns TRUE if the byte was consumed (part of an escape sequence).
 * Returns FALSE if the byte is a normal printable character.
 * ---- */
BOOL ANSI_FEED(CHAR c, TERM_OUTPUT *t) {
    switch (t->ansi_state) {
    case ANSI_NORMAL:
        if ((U8)c == 0x1B) {
            t->ansi_state = ANSI_ESC;
            return TRUE;
        }
        return FALSE;  /* normal char */

    case ANSI_ESC:
        if (c == '[') {
            t->ansi_state       = ANSI_CSI;
            t->ansi_param_count = 0;
            t->ansi_cur_param   = 0;
            return TRUE;
        }
        /* Not a CSI — abort and treat as normal */
        t->ansi_state = ANSI_NORMAL;
        return FALSE;

    case ANSI_CSI:
        if (c >= '0' && c <= '9') {
            t->ansi_cur_param = t->ansi_cur_param * 10 + (c - '0');
            return TRUE;
        }
        if (c == ';') {
            if (t->ansi_param_count < 16) {
                t->ansi_params[t->ansi_param_count++] = t->ansi_cur_param;
            }
            t->ansi_cur_param = 0;
            return TRUE;
        }
        /* Final byte — dispatch command */
        if (c >= 0x40 && c <= 0x7E) {
            ansi_dispatch_csi(t, c);
            t->ansi_state = ANSI_NORMAL;
            return TRUE;
        }
        /* Unknown intermediate — reset */
        t->ansi_state = ANSI_NORMAL;
        return FALSE;

    default:
        t->ansi_state = ANSI_NORMAL;
        return FALSE;
    }
}
