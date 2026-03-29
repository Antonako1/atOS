#ifndef TSHELL_H
#define TSHELL_H

#include <STD/TYPEDEF.h>
#include <DRIVERS/PS2/KEYBOARD_MOUSE.h>
#include <FAT/FAT.h>
#include <PROC/PROC.h>
#include <LIBRARIES/ATUI/ATUI.h>

/* ---- Forward declares ---- */
struct _BATSH_INSTANCE;
typedef struct _BATSH_INSTANCE BATSH_INSTANCE;

/* ---- Constants ---- */
#define STDOUT_MAX_LENGTH   4096
#define MAX_STDOUT_BUFFS    (MAX_PROC_AMOUNT - 2)

#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT 768
#define CHAR_WIDTH    8
#define CHAR_HEIGHT   16
/* ATUI handles glyph layout with no inter-character gaps, so
   column/row counts must match the ATUI grid (no spacing).       */
#define AMOUNT_OF_COLS  (SCREEN_WIDTH  / CHAR_WIDTH)
#define AMOUNT_OF_ROWS  (SCREEN_HEIGHT / CHAR_HEIGHT)
#define CUR_LINE_MAX_LENGTH (AMOUNT_OF_ROWS * AMOUNT_OF_COLS)
#define CMD_LINE_HISTORY 25

#define PATH_END_CHAR " $ "
#define LEND "\r\n"

#define SCROLLBACK_LINES 512

/* ---- STDOUT buffer (shared with child processes) ---- */
typedef enum {
    TSTDOUT_ACTION_CLEAR_SCREEN,
} TEXTRA_STDOUT_ACTIONS;

typedef struct {
    U8  buf[STDOUT_MAX_LENGTH];
    U32 buf_end;
    U32 proc_seq;
    U32 shell_seq;
    U32 borrowers_pid;
    U32 owner_pid;
    TEXTRA_STDOUT_ACTIONS actions;
} STDOUT_BUF;

/* ---- Key-bind flags ---- */
typedef enum TACTIVE_KEYBINDS {
    TAK_CTRL_C  = 0x0001,
    TAK_END     = 0x0002,
    TAK_DEFAULT = TAK_CTRL_C | TAK_END,
} TACTIVE_KEYBINDS;

/* ---- Shell states ---- */
typedef enum {
    TSTATE_EDIT_LINE = 0,
} TSHELL_STATES;

/* ---- Scrollback cell (per-character color) ---- */
typedef struct {
    VBE_COLOUR fg;
    VBE_COLOUR bg;
    CHAR       c;
} SCROLLBACK_CELL;

/* ---- Scrollback ring buffer ---- */
typedef struct {
    SCROLLBACK_CELL cells[SCROLLBACK_LINES][AMOUNT_OF_COLS];
    SCROLLBACK_CELL last_render[AMOUNT_OF_ROWS][AMOUNT_OF_COLS];
    U32 write_row;       /* next row to write into (mod SCROLLBACK_LINES) */
    U32 total_lines;     /* total lines ever written                      */
    U32 scroll_offset;   /* 0 = bottom, >0 = scrolled up                  */
} SCROLLBACK_BUFFER;

/* ---- Terminal output state ---- */
typedef struct {
    ATUI_WINDOW        *win;
    SCROLLBACK_BUFFER   scrollback;
    VBE_COLOUR          fg;
    VBE_COLOUR          bg;
    VBE_COLOUR          default_fg;
    VBE_COLOUR          default_bg;
    U32                 cursor_col;
    U32                 cursor_row;
    U32                 prompt_length;
    U32                 prompt_row;

    U8  current_line[CUR_LINE_MAX_LENGTH];
    U32 edit_pos;
    U32 selection_start; /* Selection start position (valid if selection_active) */
    BOOL selection_active;

    PU8  line_history[CMD_LINE_HISTORY];
    U32  history_count;
    U32  history_index;

    U8   yank_buffer[CUR_LINE_MAX_LENGTH];

    /* Cursor blink state (overlay, not stored in scrollback) */
    BOOL cursor_visible;
    U32  cursor_blink_tick;

    /* ANSI parser state */
    U8   ansi_state;
    U8   ansi_params[16];
    U8   ansi_param_count;
    U8   ansi_cur_param;
    BOOL ansi_bold;
} TERM_OUTPUT;

/* ---- Main shell instance ---- */
typedef struct {
    U32 type; // 2
    U32             focused_pid;
    U32             previously_focused_pid;
    U32             self_pid;
    TSHELL_STATES   state;
    BOOLEAN         aborted;
    struct {
        U32         current_cluster;
        DIR_ENTRY   current_path_dir_entry;
        U8          path[FAT_MAX_PATH];
        U8          prev_path[FAT_MAX_PATH];
    } fat_info;
    STDOUT_BUF     *stdouts[MAX_STDOUT_BUFFS];
    U32             stdout_count;
    BOOL8           active_kb;
    TACTIVE_KEYBINDS active_keybinds;
    TERM_OUTPUT    *tout;
} TSHELL_INSTANCE;

/* ==== Public API ==== */

/* TSHELL.c */
TSHELL_INSTANCE *GET_SHNDL(VOID);
BOOLEAN CREATE_STDOUT(U32 borrowers_pid);
BOOLEAN DELETE_STDOUT(U32 borrowers_pid);
STDOUT_BUF* GET_STDOUT(U32 borrowers_pid);
VOID EXIT_SHELL(VOID);

/* TOUTPUT.c */
VOID INIT_TOUTPUT(TSHELL_INSTANCE *shndl, ATUI_WINDOW *win);
VOID TPUT_CHAR(CHAR c);
VOID TPUT_STRING(PU8 str);
VOID TPUT_DEC(U32 n);
VOID TPUT_HEX(U32 n);
VOID TPUT_BIN(U32 n);
VOID TPUT_NEWLINE(VOID);
VOID TPUT_REFRESH(VOID);
VOID TPUT_BEGIN(VOID);
VOID TPUT_END(VOID);
VOID TPUT_SCROLL_UP(U32 n);
VOID TPUT_SCROLL_DOWN(U32 n);
VOID TPUT_SCROLL_TO_BOTTOM(VOID);
VOID TPUT_CLS(VOID);
VOID TPUT_REDRAW_CONSOLE(VOID);
VOID TPUT_SET_FG(VBE_COLOUR c);
VOID TPUT_SET_BG(VBE_COLOUR c);
VOID PUT_SHELL_START(VOID);
VOID REDRAW_CURRENT_LINE(VOID);
VOID BLINK_CURSOR(VOID);
U8 *GET_CURRENT_LINE(VOID);

/* Line-edit handlers (TOUTPUT.c) */
VOID HANDLE_LE_ENTER(VOID);
VOID HANDLE_LE_BACKSPACE(VOID);
VOID HANDLE_LE_DELETE(VOID);
VOID HANDLE_LE_ARROW_LEFT(VOID);
VOID HANDLE_LE_ARROW_RIGHT(VOID);
VOID HANDLE_LE_ARROW_UP(VOID);
VOID HANDLE_LE_ARROW_DOWN(VOID);
VOID HANDLE_LE_SHIFT_ARROW_LEFT(VOID);
VOID HANDLE_LE_SHIFT_ARROW_RIGHT(VOID);
VOID HANDLE_LE_DEFAULT(KEYPRESS *kp, MODIFIERS *mod);
VOID HANDLE_LE_HOME(VOID);
VOID HANDLE_LE_END(VOID);
VOID HANDLE_LE_CTRL_LEFT(VOID);
VOID HANDLE_LE_CTRL_RIGHT(VOID);
VOID HANDLE_LE_CTRL_BACKSPACE(VOID);
VOID HANDLE_LE_CTRL_DELETE(VOID);
VOID HANDLE_LE_CTRL_A(VOID);
VOID HANDLE_LE_CTRL_E(VOID);
VOID HANDLE_LE_CTRL_U(VOID);
VOID HANDLE_LE_CTRL_K(VOID);
VOID HANDLE_LE_CTRL_W(VOID);
VOID HANDLE_LE_CTRL_Y(VOID);
VOID HANDLE_LE_CTRL_L(VOID);
VOID HANDLE_CTRL_C(VOID);

/* ANSI.c */
VOID ANSI_RESET(TERM_OUTPUT *t);
BOOL ANSI_FEED(CHAR c, TERM_OUTPUT *t);  /* TRUE = char consumed by ANSI, FALSE = printable */

/* DIR.c */
VOID INITIALIZE_DIR(TSHELL_INSTANCE *shndl);
PU8  GET_PATH(VOID);
PU8  GET_PREV_PATH(VOID);
BOOLEAN CD_PREV(VOID);
BOOLEAN CD_INTO(PU8 path);
BOOLEAN CD_BACKWARDS_DIR(VOID);
PU8  PARSE_CD_RAW_LINE(PU8 line, U32 cut_index);
BOOLEAN ResolvePath(PU8 path, PU8 out_buffer, size_t out_buffer_size);
BOOLEAN MAKE_DIR(PU8 path);
BOOLEAN REMOVE_DIR(PU8 path);
VOID PRINT_CONTENTS(FAT_LFN_ENTRY *dir);
VOID PRINT_CONTENTS_PATH(PU8 path);

/* BATSH.c */
VOID HANDLE_COMMAND(U8 *line);
BOOLEAN RUN_BATSH_FILE(BATSH_INSTANCE *inst);
BOOLEAN RUN_BATSH_SCRIPT(PU8 path, U32 argc, PPU8 argv);
BATSH_INSTANCE *CREATE_BATSH_INSTANCE(PU8 path, U32 argc, PPU8 argv);
VOID DESTROY_BATSH_INSTANCE(BATSH_INSTANCE *inst);
BOOLEAN PARSE_BATSH_INPUT(PU8 line, BATSH_INSTANCE *inst);
VOID SETUP_BATSH_PROCESSER(VOID);
VOID BATSH_SET_MODE(U8 mode);
U8   BATSH_GET_MODE(VOID);
BOOLEAN RUN_PROCESS(PU8 line);
VOID SET_VAR(PU8 name, PU8 value);
PU8  GET_VAR(PU8 name);

typedef struct _ARG_ARRAY {
    U32  argc;
    PPU8 argv;
} ARG_ARRAY;
VOID RAW_LINE_TO_ARG_ARRAY(PU8 raw_line, ARG_ARRAY *out);
VOID DELETE_ARG_ARRAY(ARG_ARRAY *arr);

/* INPUTS.c */
VOID HANDLE_KB_EDIT_LINE(KEYPRESS *kp, MODIFIERS *mod);

/* LOOP helpers (in TSHELL.c) */
VOID EDIT_LINE_LOOP(VOID);

/* process end helper */
VOID END_PROC_SHELL(U32 pid, U32 exit_code, BOOL end_proc);

/* Compat macros for ported code */
#define PUTS(s)          TPUT_STRING((PU8)(s))
#define PUTC(c)          TPUT_CHAR((CHAR)(c))
#define PUT_DEC(n)       TPUT_DEC(n)
#define PUT_HEX(n)       TPUT_HEX(n)
#define PUT_BIN(n)       TPUT_BIN(n)
#define PRINTNEWLINE()   TPUT_NEWLINE()
#define CLS()            TPUT_CLS()
#define REDRAW_CONSOLE() TPUT_REDRAW_CONSOLE()

#endif /* TSHELL_H */
