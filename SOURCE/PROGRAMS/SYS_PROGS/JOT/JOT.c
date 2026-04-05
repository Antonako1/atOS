/*+++
    JOT.c — Nano-style TUI text editor for atOS

    Uses ATUI for terminal UI and ATRC for config/syntax highlighting.

    Usage:  JOT [filename]

    Keybinds (default, configurable via JOT.CNF):
        ^O  Write Out (save)
        ^X  Exit
        ^K  Cut line
        ^U  Uncut (paste line)
        ^W  Where Is (search)
        ^\\  Replace
        ^G  Help screen
        ^C  Show cursor position
        ^_  Go to line number
        ^A  Select all / Home
        ^E  End of line
        ^L  Refresh screen
        ^T  Go to top
        ^B  Go to bottom
---*/

// #define ATRC_IMPLEMENTATION// defined in ATUI
#include <STD/TYPEDEF.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <STD/FS_DISK.h>
#include <STD/IO.h>
#include <STD/DEBUG.h>
#include <STD/PROC_COM.h>
#include <LIBRARIES/ATUI/ATUI.h>
#include <LIBRARIES/ATUI/ATUI_WIDGETS.h>
#include <LIBRARIES/ATRC/ATRC.h>

/* ====== Configuration ====== */

#define JOT_VERSION       "1.0"
#define MAX_LINES         4096
#define MAX_LINE_LEN      512
#define CUT_BUFFER_SIZE   64
#define SEARCH_MAX        128
#define TAB_SIZE_DEFAULT  4
#define UNDO_MAX          128
#define MAX_SYNTAX_RULES  32
#define MAX_SYNTAX_KWS    64
#define MAX_KW_LEN        32
#define CONFIG_PATH       "/ATOS/CONFS/JOT/JOT.CNF"
#define SYNTAX_BASE_PATH  "/ATOS/CONFS/JOT/"
#define MAX_SYNTAX_EXT_MAP 16
#define MAX_AUTO_PAIRS     8

/* ====== Editor data structures ====== */

typedef struct {
    PU8  data;        /* heap-allocated line content                */
    U32  len;         /* length of content (excluding NUL)          */
    U32  cap;         /* allocated capacity                         */
} LINE_BUF;

/* Undo record */
typedef enum {
    UNDO_INSERT_CHAR,
    UNDO_DELETE_CHAR,
    UNDO_INSERT_LINE,
    UNDO_DELETE_LINE,
    UNDO_JOIN_LINE,
    UNDO_SPLIT_LINE,
} UNDO_TYPE;

typedef struct {
    UNDO_TYPE type;
    U32       row;
    U32       col;
    CHAR      ch;
    PU8       line_data;  /* heap copy for line ops */
} UNDO_RECORD;

/* Syntax highlighting rule */
typedef struct {
    CHAR      keywords[MAX_SYNTAX_KWS][MAX_KW_LEN];
    U32       kw_count;
    VBE_COLOUR colour;
} SYNTAX_RULE;

/* Keybinding actions */
typedef enum {
    JOT_ACT_EXIT = 0,
    JOT_ACT_SAVE,
    JOT_ACT_CUT,
    JOT_ACT_UNCUT,
    JOT_ACT_SEARCH,
    JOT_ACT_REPLACE,
    JOT_ACT_HELP,
    JOT_ACT_UNDO,
    JOT_ACT_TOP,
    JOT_ACT_BOTTOM,
    JOT_ACT_GOTO_LINE,
    JOT_ACT_POSITION,
    JOT_ACT_REFRESH,
    JOT_ACT_READ_FILE,
    JOT_ACT_HOME,
    JOT_ACT_END,
    JOT_ACT_COUNT
} JOT_ACTION;

typedef struct {
    KEYCODES key;
    BOOL     need_ctrl;
    BOOL     need_shift;
} JOT_KEYBIND;

typedef struct {
    CHAR open;
    CHAR close;
} AUTO_PAIR;

typedef struct {
    CHAR ext[16];
    CHAR syntax[16];
} SYNTAX_EXT_MAP;

/* Main editor state */
typedef struct {
    /* Text buffer */
    LINE_BUF  lines[MAX_LINES];
    U32       line_count;

    /* Cursor */
    U32       cur_row;
    U32       cur_col;
    U32       desired_col;  /* sticky column for up/down */

    /* Viewport */
    U32       scroll_y;
    U32       scroll_x;

    /* Screen dimensions (ATUI cells) */
    U32       screen_rows;
    U32       screen_cols;
    U32       edit_rows;    /* rows available for text (screen - 4) */

    /* File info */
    CHAR      filepath[256];
    BOOL      modified;
    BOOL      readonly;
    BOOL      new_file;

    /* Cut buffer */
    PU8       cut_buf[CUT_BUFFER_SIZE];
    U32       cut_count;

    /* Search / replace */
    CHAR      search_str[SEARCH_MAX];
    CHAR      replace_str[SEARCH_MAX];
    BOOL      search_case_sensitive;

    /* Undo stack */
    UNDO_RECORD undo[UNDO_MAX];
    U32       undo_top;

    /* Config */
    PATRC_FD  config;
    U32       tab_size;
    BOOL      auto_indent;
    BOOL      show_line_numbers;
    BOOL      soft_wrap;
    BOOL      syntax_enabled;
    BOOL      tab_as_spaces;

    /* Syntax file config */
    PATRC_FD       syntax_config;
    SYNTAX_EXT_MAP syntax_ext_map[MAX_SYNTAX_EXT_MAP];
    U32            syntax_ext_map_count;

    /* Keybindings */
    JOT_KEYBIND keybinds[JOT_ACT_COUNT];

    /* Autocomplete */
    AUTO_PAIR auto_pairs[MAX_AUTO_PAIRS];
    U32       auto_pair_count;

    /* Colours */
    VBE_COLOUR fg_text;
    VBE_COLOUR bg_text;
    VBE_COLOUR fg_bar;
    VBE_COLOUR bg_bar;
    VBE_COLOUR fg_shortcut_key;
    VBE_COLOUR bg_shortcut;
    VBE_COLOUR fg_shortcut_desc;
    VBE_COLOUR fg_line_number;
    VBE_COLOUR fg_status;

    /* Syntax rules */
    SYNTAX_RULE syntax_rules[MAX_SYNTAX_RULES];
    U32         syntax_rule_count;
    VBE_COLOUR  syntax_comment_colour;
    VBE_COLOUR  syntax_string_colour;
    VBE_COLOUR  syntax_number_colour;
    CHAR        syntax_comment_start[8];
    CHAR        syntax_block_comment_start[8];
    CHAR        syntax_block_comment_end[8];
    VBE_COLOUR  syntax_bracket_colours[4];

    /* Modes */
    BOOL      running;
    BOOL      show_help;

    /* Status message */
    CHAR      status_msg[128];
    U32       status_tick;
} JOT_STATE;

static JOT_STATE ed ATTRIB_DATA = {0};

/* ====== Line buffer operations ====== */

static VOID line_init(LINE_BUF *lb) {
    lb->cap  = MAX_LINE_LEN;
    lb->data = CAlloc(lb->cap, 1);
    lb->len  = 0;
}

static VOID line_free(LINE_BUF *lb) {
    if (lb->data) MFree(lb->data);
    lb->data = NULLPTR;
    lb->len  = 0;
    lb->cap  = 0;
}

static VOID line_ensure(LINE_BUF *lb, U32 needed) {
    if (needed >= lb->cap) {
        U32 newcap = needed + 128;
        PU8 nd = ReAlloc(lb->data, newcap);
        if (nd) {
            lb->data = nd;
            lb->cap  = newcap;
        }
    }
}

static VOID line_set(LINE_BUF *lb, PU8 str, U32 len) {
    line_ensure(lb, len + 1);
    MEMCPY(lb->data, str, len);
    lb->data[len] = NULLT;
    lb->len = len;
}

static VOID line_insert_char(LINE_BUF *lb, U32 pos, CHAR ch) {
    if (pos > lb->len) pos = lb->len;
    line_ensure(lb, lb->len + 2);
    MEMMOVE(lb->data + pos + 1, lb->data + pos, lb->len - pos + 1);
    lb->data[pos] = ch;
    lb->len++;
}

static VOID line_delete_char(LINE_BUF *lb, U32 pos) {
    if (pos >= lb->len) return;
    MEMMOVE(lb->data + pos, lb->data + pos + 1, lb->len - pos);
    lb->len--;
}

static VOID line_append(LINE_BUF *lb, PU8 str, U32 len) {
    line_ensure(lb, lb->len + len + 1);
    MEMCPY(lb->data + lb->len, str, len);
    lb->len += len;
    lb->data[lb->len] = NULLT;
}

/* ====== Undo ====== */

static VOID undo_push(UNDO_TYPE type, U32 row, U32 col, CHAR ch, PU8 line_data) {
    if (ed.undo_top >= UNDO_MAX) {
        /* Shift out oldest */
        if (ed.undo[0].line_data) MFree(ed.undo[0].line_data);
        MEMMOVE(&ed.undo[0], &ed.undo[1], (UNDO_MAX - 1) * sizeof(UNDO_RECORD));
        ed.undo_top = UNDO_MAX - 1;
    }
    UNDO_RECORD *r = &ed.undo[ed.undo_top++];
    r->type = type;
    r->row  = row;
    r->col  = col;
    r->ch   = ch;
    r->line_data = line_data ? STRDUP(line_data) : NULLPTR;
}

static VOID undo_pop(VOID) {
    if (ed.undo_top == 0) return;
    ed.undo_top--;
    UNDO_RECORD *r = &ed.undo[ed.undo_top];
    LINE_BUF *lb;

    switch (r->type) {
    case UNDO_INSERT_CHAR:
        /* Was an insert -> delete it */
        lb = &ed.lines[r->row];
        line_delete_char(lb, r->col);
        ed.cur_row = r->row;
        ed.cur_col = r->col;
        break;
    case UNDO_DELETE_CHAR:
        /* Was a delete -> re-insert it */
        lb = &ed.lines[r->row];
        line_insert_char(lb, r->col, r->ch);
        ed.cur_row = r->row;
        ed.cur_col = r->col;
        break;
    case UNDO_SPLIT_LINE:
        /* Was a split (Enter) -> join lines back */
        if (r->row + 1 < ed.line_count) {
            LINE_BUF *cur = &ed.lines[r->row];
            LINE_BUF *nxt = &ed.lines[r->row + 1];
            line_append(cur, nxt->data, nxt->len);
            line_free(nxt);
            MEMMOVE(&ed.lines[r->row + 1], &ed.lines[r->row + 2],
                    (ed.line_count - r->row - 2) * sizeof(LINE_BUF));
            ed.line_count--;
            ed.cur_row = r->row;
            ed.cur_col = r->col;
        }
        break;
    case UNDO_JOIN_LINE:
        /* Was a join (Backspace at col 0) -> split again */
        if (r->line_data) {
            if (ed.line_count < MAX_LINES) {
                MEMMOVE(&ed.lines[r->row + 1], &ed.lines[r->row],
                        (ed.line_count - r->row) * sizeof(LINE_BUF));
                ed.line_count++;
                line_init(&ed.lines[r->row]);
                line_set(&ed.lines[r->row], r->line_data, STRLEN(r->line_data));
                /* Trim joined portion from next line */
                LINE_BUF *nxt = &ed.lines[r->row + 1];
                U32 old_len = STRLEN(r->line_data);
                if (nxt->len >= old_len) {
                    MEMMOVE(nxt->data, nxt->data + old_len, nxt->len - old_len + 1);
                    nxt->len -= old_len;
                }
                ed.cur_row = r->row;
                ed.cur_col = old_len;
            }
        }
        break;
    case UNDO_DELETE_LINE:
        /* Line was deleted (cut) -> re-insert */
        if (r->line_data && ed.line_count < MAX_LINES) {
            MEMMOVE(&ed.lines[r->row + 1], &ed.lines[r->row],
                    (ed.line_count - r->row) * sizeof(LINE_BUF));
            ed.line_count++;
            line_init(&ed.lines[r->row]);
            line_set(&ed.lines[r->row], r->line_data, STRLEN(r->line_data));
            ed.cur_row = r->row;
            ed.cur_col = 0;
        }
        break;
    case UNDO_INSERT_LINE:
        /* Line was inserted (uncut) -> remove it */
        if (r->row < ed.line_count) {
            line_free(&ed.lines[r->row]);
            MEMMOVE(&ed.lines[r->row], &ed.lines[r->row + 1],
                    (ed.line_count - r->row - 1) * sizeof(LINE_BUF));
            ed.line_count--;
            if (ed.cur_row >= ed.line_count && ed.line_count > 0)
                ed.cur_row = ed.line_count - 1;
            ed.cur_col = 0;
        }
        break;
    }

    if (r->line_data) { MFree(r->line_data); r->line_data = NULLPTR; }
    ed.modified = TRUE;
}

/* ====== Status message ====== */

static VOID set_status(PU8 msg) {
    STRNCPY(ed.status_msg, msg, sizeof(ed.status_msg) - 1);
    ed.status_msg[sizeof(ed.status_msg) - 1] = NULLT;
    ed.status_tick = 100; /* display for ~100 refresh cycles */
}

/* ====== Clamp cursor ====== */

static VOID clamp_cursor(VOID) {
    if (ed.cur_row >= ed.line_count)
        ed.cur_row = ed.line_count > 0 ? ed.line_count - 1 : 0;
    U32 line_len = ed.lines[ed.cur_row].len;
    if (ed.cur_col > line_len)
        ed.cur_col = line_len;
}

/* ====== Scroll viewport to keep cursor visible ====== */

static VOID scroll_to_cursor(VOID) {
    /* Vertical */
    if (ed.cur_row < ed.scroll_y)
        ed.scroll_y = ed.cur_row;
    if (ed.cur_row >= ed.scroll_y + ed.edit_rows)
        ed.scroll_y = ed.cur_row - ed.edit_rows + 1;

    /* Horizontal */
    U32 gutter = ed.show_line_numbers ? 5 : 0;
    U32 text_cols = ed.screen_cols > gutter ? ed.screen_cols - gutter : 1;
    if (ed.cur_col < ed.scroll_x)
        ed.scroll_x = ed.cur_col;
    if (ed.cur_col >= ed.scroll_x + text_cols)
        ed.scroll_x = ed.cur_col - text_cols + 1;
}

/* ====== Colour config loading ====== */

static U32 parse_colour_val(PU8 str) {
    if (!str) return VBE_WHITE;
    /* Expects "R,G,B" format */
    U32 r = 255, g = 255, b = 255;
    PU8 dup = STRDUP(str);
    if (!dup) return VBE_WHITE;
    PU8 saveptr = NULLPTR;
    PU8 tok = STRTOK_R(dup, ",", &saveptr);
    if (tok) r = ATOI(tok);
    tok = STRTOK_R(NULLPTR, ",", &saveptr);
    if (tok) g = ATOI(tok);
    tok = STRTOK_R(NULLPTR, ",", &saveptr);
    if (tok) b = ATOI(tok);
    MFree(dup);
    return VBE_COLOUR(r, g, b);
}

static VOID load_colours(VOID) {
    /* Defaults — dark nano-like theme */
    ed.fg_text          = VBE_WHITE;
    ed.bg_text          = VBE_BLACK;
    ed.fg_bar           = VBE_BLACK;
    ed.bg_bar           = VBE_WHITE;
    ed.fg_shortcut_key  = VBE_WHITE;
    ed.bg_shortcut      = VBE_BLACK;
    ed.fg_shortcut_desc = VBE_LIGHT_GRAY;
    ed.fg_line_number   = VBE_DARK_YELLOW;
    ed.fg_status        = VBE_WHITE;

    /* Syntax defaults */
    ed.syntax_comment_colour = VBE_GRAY;
    ed.syntax_string_colour  = VBE_LIGHT_GREEN;
    ed.syntax_number_colour  = VBE_LIGHT_CYAN;

    /* Bracket colour defaults (rainbow) */
    ed.syntax_bracket_colours[0] = VBE_LIGHT_RED;
    ed.syntax_bracket_colours[1] = VBE_LIGHT_GREEN;
    ed.syntax_bracket_colours[2] = VBE_LIGHT_CYAN;
    ed.syntax_bracket_colours[3] = VBE_LIGHT_MAGENTA;

    if (!ed.config) return;

    PU8 v;
    v = READ_KEY(ed.config, "COLOUR", "FG_TEXT");
    if (v) ed.fg_text = parse_colour_val(v);
    v = READ_KEY(ed.config, "COLOUR", "BG_TEXT");
    if (v) ed.bg_text = parse_colour_val(v);
    v = READ_KEY(ed.config, "COLOUR", "FG_BAR");
    if (v) ed.fg_bar = parse_colour_val(v);
    v = READ_KEY(ed.config, "COLOUR", "BG_BAR");
    if (v) ed.bg_bar = parse_colour_val(v);
    v = READ_KEY(ed.config, "COLOUR", "FG_SHORTCUT_KEY");
    if (v) ed.fg_shortcut_key = parse_colour_val(v);
    v = READ_KEY(ed.config, "COLOUR", "BG_SHORTCUT");
    if (v) ed.bg_shortcut = parse_colour_val(v);
    v = READ_KEY(ed.config, "COLOUR", "FG_SHORTCUT_DESC");
    if (v) ed.fg_shortcut_desc = parse_colour_val(v);
    v = READ_KEY(ed.config, "COLOUR", "FG_LINE_NUMBER");
    if (v) ed.fg_line_number = parse_colour_val(v);
    v = READ_KEY(ed.config, "COLOUR", "FG_STATUS");
    if (v) ed.fg_status = parse_colour_val(v);
    v = READ_KEY(ed.config, "COLOUR", "FG_COMMENT");
    if (v) ed.syntax_comment_colour = parse_colour_val(v);
    v = READ_KEY(ed.config, "COLOUR", "FG_STRING");
    if (v) ed.syntax_string_colour = parse_colour_val(v);
    v = READ_KEY(ed.config, "COLOUR", "FG_NUMBER");
    if (v) ed.syntax_number_colour = parse_colour_val(v);
}

/* ====== Autocomplete helper ====== */

static CHAR get_pair_close(CHAR open) {
    switch (open) {
        case '(': return ')';
        case '{': return '}';
        case '[': return ']';
        case '"': return '"';
        case '\'': return '\'';
        case '<': return '>';
        default: return NULLT;
    }
}

/* ====== Keybinding helpers ====== */

static VOID set_keybind(JOT_ACTION act, KEYCODES key, BOOL ctrl, BOOL shift) {
    ed.keybinds[act].key        = key;
    ed.keybinds[act].need_ctrl  = ctrl;
    ed.keybinds[act].need_shift = shift;
}

static VOID init_default_keybinds(VOID) {
    set_keybind(JOT_ACT_EXIT,      KEY_X,         TRUE, FALSE);
    set_keybind(JOT_ACT_SAVE,      KEY_O,         TRUE, FALSE);
    set_keybind(JOT_ACT_CUT,       KEY_K,         TRUE, FALSE);
    set_keybind(JOT_ACT_UNCUT,     KEY_U,         TRUE, FALSE);
    set_keybind(JOT_ACT_SEARCH,    KEY_W,         TRUE, FALSE);
    set_keybind(JOT_ACT_REPLACE,   KEY_BACKSLASH, TRUE, FALSE);
    set_keybind(JOT_ACT_HELP,      KEY_G,         TRUE, FALSE);
    set_keybind(JOT_ACT_UNDO,      KEY_Z,         TRUE, FALSE);
    set_keybind(JOT_ACT_TOP,       KEY_T,         TRUE, FALSE);
    set_keybind(JOT_ACT_BOTTOM,    KEY_B,         TRUE, FALSE);
    set_keybind(JOT_ACT_GOTO_LINE, KEY_SLASH,     TRUE, TRUE);
    set_keybind(JOT_ACT_POSITION,  KEY_C,         TRUE, FALSE);
    set_keybind(JOT_ACT_REFRESH,   KEY_L,         TRUE, FALSE);
    set_keybind(JOT_ACT_READ_FILE, KEY_R,         TRUE, FALSE);
    set_keybind(JOT_ACT_HOME,      KEY_A,         TRUE, FALSE);
    set_keybind(JOT_ACT_END,       KEY_E,         TRUE, FALSE);
}

static VOID parse_keybind_str(PU8 str, JOT_ACTION action) {
    if (!str || !str[0]) return;
    BOOL ctrl = FALSE, shift = FALSE;
    PU8 p = str;
    if (*p == '^') { ctrl = TRUE; p++; }
    if (!*p) return;
    CHAR ch = *p;
    if (ch == '_') shift = TRUE;
    KEYCODES key = char_to_keycode(ch);
    if (key) set_keybind(action, key, ctrl, shift);
}

static VOID load_keybinds(VOID) {
    init_default_keybinds();
    if (!ed.config) return;
    if (!DOES_EXIST_BLOCK(ed.config, "KEYBINDS")) return;

    PU8 v;
    typedef struct { PU8 name; JOT_ACTION action; } KB_MAP;
    KB_MAP map[] = {
        {"EXIT",      JOT_ACT_EXIT},
        {"SAVE",      JOT_ACT_SAVE},
        {"CUT",       JOT_ACT_CUT},
        {"UNCUT",     JOT_ACT_UNCUT},
        {"SEARCH",    JOT_ACT_SEARCH},
        {"REPLACE",   JOT_ACT_REPLACE},
        {"HELP",      JOT_ACT_HELP},
        {"UNDO",      JOT_ACT_UNDO},
        {"TOP",       JOT_ACT_TOP},
        {"BOTTOM",    JOT_ACT_BOTTOM},
        {"GOTO_LINE", JOT_ACT_GOTO_LINE},
        {"POSITION",  JOT_ACT_POSITION},
        {"REFRESH",   JOT_ACT_REFRESH},
        {"READ_FILE", JOT_ACT_READ_FILE},
        {"HOME",      JOT_ACT_HOME},
        {"END",       JOT_ACT_END},
    };
    for (U32 i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
        v = READ_KEY(ed.config, "KEYBINDS", map[i].name);
        if (v) parse_keybind_str(v, map[i].action);
    }
}

/* ====== Syntax extension map ====== */

static VOID load_syntax_extension_map(VOID) {
    ed.syntax_ext_map_count = 0;
    if (!ed.config) return;
    if (!DOES_EXIST_BLOCK(ed.config, "SYNTAX_FILES")) return;

    /* Iterate the SYNTAX_FILES block keys directly */
    for (U32 b = 0; b < ed.config->blocks.len; b++) {
        PBLOCK block = ed.config->blocks.blocks[b];
        if (!block || STRICMP(block->name, "SYNTAX_FILES") != 0) continue;

        for (U32 k = 0; k < block->keys.len && ed.syntax_ext_map_count < MAX_SYNTAX_EXT_MAP; k++) {
            PKEY_VALUE kv = block->keys.key_value_pairs[k];
            if (!kv || !kv->name || !kv->value) continue;
            if (STRICMP(kv->name, "FILES") == 0) continue;

            SYNTAX_EXT_MAP *m = &ed.syntax_ext_map[ed.syntax_ext_map_count];
            STRNCPY(m->ext, kv->name, sizeof(m->ext) - 1);
            STRNCPY(m->syntax, kv->value, sizeof(m->syntax) - 1);
            ed.syntax_ext_map_count++;
        }
        break;
    }
}

/* ====== Load syntax-specific config for a file extension ====== */

static PU8 get_file_extension(PU8 filepath) {
    if (!filepath) return NULLPTR;
    PU8 dot = NULLPTR;
    for (PU8 p = filepath; *p; p++) {
        if (*p == '.') dot = p + 1;
    }
    return dot;
}

static VOID load_syntax_rules_from_fd(PATRC_FD fd) {
    if (!fd) return;
    PU8 v;
    ed.syntax_rule_count = 0;
    if (!DOES_EXIST_BLOCK(fd, "SYNTAX_KEYWORDS")) return;

    for (U32 i = 0; i < MAX_SYNTAX_RULES; i++) {
        CHAR key[16];
        SPRINTF(key, "RULE_%d", i);
        v = READ_KEY(fd, "SYNTAX_KEYWORDS", key);
        if (!v) break;

        /* Format: "colour_r,g,b:word1,word2,word3" */
        PU8 colon = STRCHR(v, ':');
        if (!colon) continue;

        CHAR colour_str[32];
        U32 cl = colon - v;
        if (cl >= sizeof(colour_str)) cl = sizeof(colour_str) - 1;
        MEMCPY(colour_str, v, cl);
        colour_str[cl] = NULLT;

        SYNTAX_RULE *rule = &ed.syntax_rules[ed.syntax_rule_count];
        rule->colour = parse_colour_val(colour_str);
        rule->kw_count = 0;

        PU8 kw_list = colon + 1;
        PSTR_ARR arr = ATRC_TO_LIST(kw_list, ',');
        if (arr) {
            for (U32 j = 0; j < arr->len && j < MAX_SYNTAX_KWS; j++) {
                STRNCPY(rule->keywords[j], arr->strs[j], MAX_KW_LEN - 1);
                rule->keywords[j][MAX_KW_LEN - 1] = NULLT;
                rule->kw_count++;
            }
            FREE_STR_ARR(arr);
        }
        ed.syntax_rule_count++;
    }
}

static VOID load_syntax_config_for_ext(PU8 ext) {
    if (!ext || !ext[0]) return;

    /* Look up extension in map */
    PU8 syntax_name = NULLPTR;
    for (U32 i = 0; i < ed.syntax_ext_map_count; i++) {
        if (STRICMP(ed.syntax_ext_map[i].ext, ext) == 0) {
            syntax_name = ed.syntax_ext_map[i].syntax;
            break;
        }
    }
    if (!syntax_name) return;

    /* Construct config file path: /ATOS/CONFS/JOT/{NAME}_JOT.CNF */
    CHAR path[256];
    SPRINTF(path, "%s%s_JOT.CNF", SYNTAX_BASE_PATH, syntax_name);

    ed.syntax_config = CREATE_ATRCFD(path, ATRC_READ_WRITE);
    if (!ed.syntax_config) return;

    PU8 v;

    /* Load COMMENT_START from syntax config (overrides JOT.CNF fallback) */
    v = READ_KEY(ed.syntax_config, "SETTINGS", "COMMENT_START");
    if (v) STRNCPY(ed.syntax_comment_start, v, sizeof(ed.syntax_comment_start) - 1);

    /* Load block comment delimiters from syntax config */
    v = READ_KEY(ed.syntax_config, "SETTINGS", "BLOCK_COMMENT_START");
    if (v) STRNCPY(ed.syntax_block_comment_start, v, sizeof(ed.syntax_block_comment_start) - 1);
    v = READ_KEY(ed.syntax_config, "SETTINGS", "BLOCK_COMMENT_END");
    if (v) STRNCPY(ed.syntax_block_comment_end, v, sizeof(ed.syntax_block_comment_end) - 1);

    /* Load TAB_AS_SPACES from syntax config (overrides JOT.CNF) */

    v = READ_KEY(ed.syntax_config, "SETTINGS", "TAB_AS_SPACES");
    if (v) ed.tab_as_spaces = ATRC_TO_BOOL(v);

    /* Load autocomplete pairs */
    ed.auto_pair_count = 0;
    if (DOES_EXIST_BLOCK(ed.syntax_config, "AUTOCOMPLETE")) {
        v = READ_KEY(ed.syntax_config, "AUTOCOMPLETE", "PAIRS");
        if (v) {
            for (U32 i = 0; v[i] && ed.auto_pair_count < MAX_AUTO_PAIRS; i++) {
                CHAR close = get_pair_close(v[i]);
                if (close) {
                    ed.auto_pairs[ed.auto_pair_count].open = v[i];
                    ed.auto_pairs[ed.auto_pair_count].close = close;
                    ed.auto_pair_count++;
                }
            }
        }
    }

    /* Load syntax-specific colours (overrides base theme) */
    v = READ_KEY(ed.syntax_config, "COLOUR", "FG_COMMENT");
    if (v) ed.syntax_comment_colour = parse_colour_val(v);
    v = READ_KEY(ed.syntax_config, "COLOUR", "FG_STRING");
    if (v) ed.syntax_string_colour = parse_colour_val(v);
    v = READ_KEY(ed.syntax_config, "COLOUR", "FG_NUMBER");
    if (v) ed.syntax_number_colour = parse_colour_val(v);

    /* Load syntax keyword rules */
    load_syntax_rules_from_fd(ed.syntax_config);
}

/* ====== Keybind action lookup ====== */

static JOT_ACTION lookup_action(KEYCODES key, BOOL ctrl, BOOL shift) {
    for (U32 i = 0; i < JOT_ACT_COUNT; i++) {
        if (ed.keybinds[i].key == key &&
            ed.keybinds[i].need_ctrl == ctrl &&
            ed.keybinds[i].need_shift == shift)
            return (JOT_ACTION)i;
    }
    return JOT_ACT_COUNT;
}

/* ====== Config loading ====== */

static VOID load_config(VOID) {

    ed.config = CREATE_ATRCFD(CONFIG_PATH, ATRC_READ_WRITE);

    /* Defaults */
    ed.tab_size          = TAB_SIZE_DEFAULT;
    ed.auto_indent       = TRUE;
    ed.show_line_numbers = TRUE;
    ed.soft_wrap         = FALSE;
    ed.syntax_enabled    = TRUE;
    ed.search_case_sensitive = FALSE;
    ed.tab_as_spaces     = TRUE;
    STRCPY(ed.syntax_comment_start, "//");
    STRCPY(ed.syntax_block_comment_start, "/*");
    STRCPY(ed.syntax_block_comment_end, "*/");

    /* Default bracket auto-pairs */
    ed.auto_pairs[0].open = '('; ed.auto_pairs[0].close = ')';
    ed.auto_pairs[1].open = '{'; ed.auto_pairs[1].close = '}';
    ed.auto_pairs[2].open = '['; ed.auto_pairs[2].close = ']';
    ed.auto_pair_count = 3;

    if (!ed.config) {
        load_keybinds();
        load_colours();
        return;
    }
    PU8 v;
    v = READ_KEY(ed.config, "SETTINGS", "TAB_SIZE");
    if (v) ed.tab_size = ATOI(v);
    if (ed.tab_size == 0 || ed.tab_size > 16) ed.tab_size = TAB_SIZE_DEFAULT;

    v = READ_KEY(ed.config, "SETTINGS", "AUTO_INDENT");
    if (v) ed.auto_indent = ATRC_TO_BOOL(v);

    v = READ_KEY(ed.config, "SETTINGS", "LINE_NUMBERS");
    if (v) ed.show_line_numbers = ATRC_TO_BOOL(v);

    v = READ_KEY(ed.config, "SETTINGS", "SOFT_WRAP");
    if (v) ed.soft_wrap = ATRC_TO_BOOL(v);

    v = READ_KEY(ed.config, "SETTINGS", "SYNTAX");
    if (v) ed.syntax_enabled = ATRC_TO_BOOL(v);

    v = READ_KEY(ed.config, "SETTINGS", "CASE_SENSITIVE_SEARCH");
    if (v) ed.search_case_sensitive = ATRC_TO_BOOL(v);

    v = READ_KEY(ed.config, "SETTINGS", "COMMENT_START");
    if (v) STRNCPY(ed.syntax_comment_start, v, sizeof(ed.syntax_comment_start) - 1);

    v = READ_KEY(ed.config, "SETTINGS", "BLOCK_COMMENT_START");
    if (v) STRNCPY(ed.syntax_block_comment_start, v, sizeof(ed.syntax_block_comment_start) - 1);
    v = READ_KEY(ed.config, "SETTINGS", "BLOCK_COMMENT_END");
    if (v) STRNCPY(ed.syntax_block_comment_end, v, sizeof(ed.syntax_block_comment_end) - 1);

    v = READ_KEY(ed.config, "SETTINGS", "TAB_AS_SPACES");
    if (v) ed.tab_as_spaces = ATRC_TO_BOOL(v);

    /* Load syntax extension map from [SYNTAX_FILES] */
    load_syntax_extension_map();

    /* Load keybindings from [KEYBINDS] */
    load_keybinds();

    load_colours();
}

/* ====== Detect file type and load built-in syntax rules ====== */

static BOOL str_endswith(PU8 str, PU8 suffix) {
    U32 slen = STRLEN(str);
    U32 xlen = STRLEN(suffix);
    if (xlen > slen) return FALSE;
    return STRICMP(str + slen - xlen, suffix) == 0;
}

static VOID add_builtin_rule(VBE_COLOUR colour, PU8 csv_keywords) {
    if (ed.syntax_rule_count >= MAX_SYNTAX_RULES) return;
    SYNTAX_RULE *rule = &ed.syntax_rules[ed.syntax_rule_count];
    rule->colour = colour;
    rule->kw_count = 0;
    PSTR_ARR arr = ATRC_TO_LIST(csv_keywords, ',');
    if (arr) {
        for (U32 j = 0; j < arr->len && j < MAX_SYNTAX_KWS; j++) {
            STRNCPY(rule->keywords[j], arr->strs[j], MAX_KW_LEN - 1);
            rule->keywords[j][MAX_KW_LEN - 1] = NULLT;
            rule->kw_count++;
        }
        FREE_STR_ARR(arr);
    }
    ed.syntax_rule_count++;
}

static VOID detect_syntax(VOID) {
    if (!ed.syntax_enabled) return;
    if (!ed.filepath[0]) return;

    /* Try config-based syntax loading first */
    PU8 ext = get_file_extension(ed.filepath);
    if (ext && ed.syntax_ext_map_count > 0) {
        load_syntax_config_for_ext(ext);
        if (ed.syntax_rule_count > 0) return; /* Config rules loaded */
    }

    /* Fall back to built-in rules only if no config rules exist */
    if (ed.syntax_rule_count > 0) return;

    if (str_endswith(ed.filepath, ".AC") || str_endswith(ed.filepath, ".AH") ||
        str_endswith(ed.filepath, ".Ac") || str_endswith(ed.filepath, ".Ah")) {
        STRCPY(ed.syntax_comment_start, "//");
        STRCPY(ed.syntax_block_comment_start, "/*");
        STRCPY(ed.syntax_block_comment_end, "*/");
        /* Types and qualifiers */
        add_builtin_rule(VBE_LIGHT_GREEN,
            "int,char,void,float,double,long,short,unsigned,signed,const,static,"
            "struct,union,enum,typedef,extern,volatile,register,inline,auto,"
            "U8,U16,U32,I8,I16,I32,U0,I0,S0,F32,PU8,PU32,PPU8,PPU32,"
            "VOID,CHAR,BOOL,BOOLEAN,BOOL8,UINT,INT,DWORD,WORD,BYTE,SIZE_T,"
            "VOIDPTR,NULLPTR");
        /* Control flow */
        add_builtin_rule(VBE_LIGHT_MAGENTA,
            "if,else,for,while,do,switch,case,default,break,continue,return,goto,sizeof");
        /* Preprocessor keywords */
        add_builtin_rule(VBE_LIGHT_CYAN,
            "#include,#define,#ifndef,#ifdef,#endif,#if,#else,#elif,#undef,#pragma,"
            "TRUE,FALSE,NULL,NULLT");
    }
    else if (str_endswith(ed.filepath, ".ASM") || str_endswith(ed.filepath, ".INC") ||
             str_endswith(ed.filepath, ".asm") || str_endswith(ed.filepath, ".inc")) {
        STRCPY(ed.syntax_comment_start, ";");
        add_builtin_rule(VBE_LIGHT_GREEN,
            "mov,add,sub,mul,div,inc,dec,and,or,xor,not,shl,shr,push,pop,"
            "call,ret,jmp,je,jne,jz,jnz,jg,jl,jge,jle,cmp,test,lea,nop,"
            "int,iret,cli,sti,hlt,in,out,rep,movs,stos,lods");
        add_builtin_rule(VBE_LIGHT_MAGENTA,
            "section,segment,global,extern,bits,org,db,dw,dd,dq,times,resb,resw,resd,"
            "equ,%macro,%endmacro,%include,%define,%if,%endif");
        add_builtin_rule(VBE_LIGHT_CYAN,
            "eax,ebx,ecx,edx,esi,edi,esp,ebp,ax,bx,cx,dx,si,di,sp,bp,"
            "al,ah,bl,bh,cl,ch,dl,dh,cs,ds,es,fs,gs,ss,cr0,cr3");
    }
    else if (str_endswith(ed.filepath, ".SH")) {
        STRCPY(ed.syntax_comment_start, "#");
        add_builtin_rule(VBE_LIGHT_GREEN,
            "echo,cd,ls,dir,cat,type,rm,mkdir,cp,mv,set,export,exit,clear,cls");
        add_builtin_rule(VBE_LIGHT_MAGENTA,
            "if,else,fi,then,for,while,do,done,case,esac,in,function");
    }
    else if (str_endswith(ed.filepath, ".CNF")) {
        STRCPY(ed.syntax_comment_start, "#");
        add_builtin_rule(VBE_LIGHT_GREEN,
            "TRUE,FALSE,ON,OFF,1,0");
    }
}

/* ====== File I/O ====== */

static VOID strip_cr(PU8 line, U32 *len) {
    while (*len > 0 && (line[*len - 1] == '\r' || line[*len - 1] == '\n')) {
        line[--(*len)] = NULLT;
    }
}

static VOID load_file(PU8 filepath) {
    if (!filepath || !filepath[0]) {
        ed.new_file = TRUE;
        line_init(&ed.lines[0]);
        ed.line_count = 1;
        return;
    }

    STRCPY(ed.filepath, filepath);
    FILE *f = FOPEN(filepath, MODE_FR);
    if (!f) {
        /* New file */
        ed.new_file = TRUE;
        line_init(&ed.lines[0]);
        ed.line_count = 1;
        set_status("[ New File ]");
        return;
    }

    ed.new_file = FALSE;
    U8 line_buf[MAX_LINE_LEN];
    ed.line_count = 0;

    while (FILE_GET_LINE(f, line_buf, sizeof(line_buf)) && ed.line_count < MAX_LINES) {
        U32 len = STRLEN(line_buf);
        strip_cr(line_buf, &len);
        line_init(&ed.lines[ed.line_count]);
        line_set(&ed.lines[ed.line_count], line_buf, len);
        ed.line_count++;
    }

    FCLOSE(f);

    if (ed.line_count == 0) {
        line_init(&ed.lines[0]);
        ed.line_count = 1;
    }

    CHAR msg[128];
    SPRINTF(msg, "Read %d lines", ed.line_count);
    set_status(msg);
}

static BOOL save_file(VOID) {
    if (!ed.filepath[0]) return FALSE;

    if(!FILE_EXISTS(ed.filepath)) {
        if(!FILE_CREATE(ed.filepath)) {
            set_status("[ Error creating file ]");
            return FALSE;
        }
    }
    FILE *f = FOPEN(ed.filepath, MODE_FW);
    if (!f) {
        set_status("[ Error writing file ]");
        return FALSE;
    }

    /* Build full buffer and write at once */
    U32 total_size = 0;
    for (U32 i = 0; i < ed.line_count; i++) {
        total_size += ed.lines[i].len + 1; /* +1 for newline */
    }

    PU8 buf = MAlloc(total_size + 1);
    if (!buf) {
        FCLOSE(f);
        set_status("[ Out of memory ]");
        return FALSE;
    }

    U32 pos = 0;
    for (U32 i = 0; i < ed.line_count; i++) {
        MEMCPY(buf + pos, ed.lines[i].data, ed.lines[i].len);
        pos += ed.lines[i].len;
        if (i < ed.line_count - 1) {
            buf[pos++] = '\n';
        }
    }
    buf[pos] = NULLT;

    FWRITE(f, buf, pos);
    FCLOSE(f);
    MFree(buf);

    ed.modified = FALSE;
    CHAR msg[128];
    SPRINTF(msg, "[ Wrote %d lines ]", ed.line_count);
    set_status(msg);
    return TRUE;
}

/* ====== Syntax highlighting helpers ====== */

static BOOL is_word_char(CHAR c) {
    return ISALNUM(c) || c == '_' || c == '#';
}

static BOOL match_keyword_at(PU8 line, U32 pos, U32 line_len, PU8 keyword) {
    U32 kwlen = STRLEN(keyword);
    if (pos + kwlen > line_len) return FALSE;
    /* Check boundary before */
    if (pos > 0 && is_word_char(line[pos - 1])) return FALSE;
    /* Match characters */
    for (U32 i = 0; i < kwlen; i++) {
        if (line[pos + i] != keyword[i]) return FALSE;
    }
    /* Check boundary after */
    if (pos + kwlen < line_len && is_word_char(line[pos + kwlen])) return FALSE;
    return TRUE;
}

/* Get syntax colour for a character position in a line.
   Returns VBE_SEE_THROUGH if no highlighting applies.
   kw_skip/kw_colour track multi-char keyword highlighting across calls. */
static VBE_COLOUR get_syntax_colour(PU8 line, U32 line_len, U32 pos,
                                     BOOL *in_string, CHAR *string_char,
                                     U32 *kw_skip, VBE_COLOUR *kw_colour) {
    if (!ed.syntax_enabled || !line) return VBE_SEE_THROUGH;

    CHAR c = line[pos];

    /* Continue keyword colour from previous match */
    if (*kw_skip > 0) {
        (*kw_skip)--;
        return *kw_colour;
    }

    /* String handling */
    if (*in_string) {
        if (c == *string_char && (pos == 0 || line[pos - 1] != '\\')) {
            *in_string = FALSE;
        }
        return ed.syntax_string_colour;
    }

    if (c == '"' || c == '\'') {
        *in_string = TRUE;
        *string_char = c;
        return ed.syntax_string_colour;
    }

    /* Number literal */
    if (IS_DIGIT(c) && (pos == 0 || !is_word_char(line[pos - 1]))) {
        return ed.syntax_number_colour;
    }

    /* Keyword matching — highlights the entire keyword, not just the first char */
    for (U32 r = 0; r < ed.syntax_rule_count; r++) {
        SYNTAX_RULE *rule = &ed.syntax_rules[r];
        for (U32 k = 0; k < rule->kw_count; k++) {
            if (match_keyword_at(line, pos, line_len, rule->keywords[k])) {
                U32 kwlen = STRLEN(rule->keywords[k]);
                if (kwlen > 1) {
                    *kw_skip = kwlen - 1;
                    *kw_colour = rule->colour;
                }
                return rule->colour;
            }
        }
    }

    return VBE_SEE_THROUGH;
}

static BOOL is_bracket(CHAR c) {
    return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}';
}

/* Compute syntax state (block comment, bracket depth) at start of a given line
   by scanning all preceding lines.  Needed for correct multi-line highlights. */
static VOID compute_syntax_state_at(U32 target_row,
                                     BOOL *out_block, U32 *out_depth) {
    BOOL in_block  = FALSE;
    BOOL in_str    = FALSE;
    CHAR str_ch    = 0;
    U32  depth     = 0;
    U32  bc_s_len  = STRLEN(ed.syntax_block_comment_start);
    U32  bc_e_len  = STRLEN(ed.syntax_block_comment_end);
    U32  sc_len    = STRLEN(ed.syntax_comment_start);

    for (U32 row = 0; row < target_row && row < ed.line_count; row++) {
        LINE_BUF *lb = &ed.lines[row];
        in_str = FALSE;            /* strings don't span lines */
        for (U32 col = 0; col < lb->len; col++) {
            CHAR c = lb->data[col];
            /* Inside block comment — look only for end */
            if (in_block) {
                if (bc_e_len > 0 && col + bc_e_len <= lb->len) {
                    BOOL m = TRUE;
                    for (U32 i = 0; i < bc_e_len; i++)
                        if (lb->data[col + i] != ed.syntax_block_comment_end[i]) { m = FALSE; break; }
                    if (m) { in_block = FALSE; col += bc_e_len - 1; }
                }
                continue;
            }
            if (in_str) {
                if (c == str_ch && (col == 0 || lb->data[col - 1] != '\\'))
                    in_str = FALSE;
                continue;
            }
            if (c == '"' || c == '\'') { in_str = TRUE; str_ch = c; continue; }
            /* Single-line comment — skip rest of line */
            if (sc_len > 0 && col + sc_len <= lb->len) {
                BOOL m = TRUE;
                for (U32 i = 0; i < sc_len; i++)
                    if (lb->data[col + i] != ed.syntax_comment_start[i]) { m = FALSE; break; }
                if (m) break;
            }
            /* Block comment start */
            if (bc_s_len > 0 && col + bc_s_len <= lb->len) {
                BOOL m = TRUE;
                for (U32 i = 0; i < bc_s_len; i++)
                    if (lb->data[col + i] != ed.syntax_block_comment_start[i]) { m = FALSE; break; }
                if (m) { in_block = TRUE; col += bc_s_len - 1; continue; }
            }
            /* Track brackets */
            if (c == '(' || c == '[' || c == '{') depth++;
            else if (c == ')' || c == ']' || c == '}') { if (depth > 0) depth--; }
        }
    }
    *out_block = in_block;
    *out_depth = depth;
}

/* ====== Drawing ====== */

static VOID draw_title_bar(VOID) {
    ATUI_SET_COLOR(ed.fg_bar, ed.bg_bar);
    ATUI_ATTRON(ATUI_A_REVERSE);
    ATUI_MOVE(0, 0);

    /* Clear line */
    for (U32 i = 0; i < ed.screen_cols; i++) ATUI_ADDCH(' ');

    /* Left: version */
    ATUI_MVPRINTW(0, 2, "JOT %s", JOT_VERSION);

    /* Center: filename */
    PU8 name = ed.filepath[0] ? ed.filepath : "New Buffer";
    U32 namelen = STRLEN(name);
    U32 center = ed.screen_cols / 2;
    if (namelen / 2 < center)
        center -= namelen / 2;
    else
        center = 0;
    ATUI_MVPRINTW(0, center, "%s%s", name, ed.modified ? " (Modified)" : "");

    ATUI_ATTROFF(ATUI_A_REVERSE);
}

static VOID draw_shortcut_bar(VOID) {
    /* Two bottom rows of shortcuts, nano-style */
    typedef struct { PU8 key; PU8 desc; } SHORTCUT;
    SHORTCUT row1[] = {
        {"^G", "Help"},    {"^O", "Write Out"}, {"^W", "Where Is"},
        {"^K", "Cut"},     {"^C", "Cur Pos"},   {"^T", "To Top"},
    };
    SHORTCUT row2[] = {
        {"^X", "Exit"},    {"^R", "Read File"}, {"^\\", "Replace"},
        {"^U", "Uncut"},   {"^_", "Go To Ln"},  {"^Z", "Undo"},
    };

    U32 bar_y1 = ed.screen_rows - 2;
    U32 bar_y2 = ed.screen_rows - 1;
    U32 num1 = sizeof(row1) / sizeof(row1[0]);
    U32 num2 = sizeof(row2) / sizeof(row2[0]);
    U32 slot_w = ed.screen_cols / 6;

    /* Draw row 1 */
    ATUI_MOVE(bar_y1, 0);
    ATUI_SET_COLOR(ed.fg_shortcut_desc, ed.bg_shortcut);
    for (U32 i = 0; i < ed.screen_cols; i++) ATUI_ADDCH(' ');

    for (U32 i = 0; i < num1; i++) {
        U32 x = i * slot_w;
        ATUI_SET_COLOR(ed.fg_shortcut_key, ed.bg_shortcut);
        ATUI_ATTRON(ATUI_A_REVERSE);
        ATUI_MVADDSTR(bar_y1, x, row1[i].key);
        ATUI_ATTROFF(ATUI_A_REVERSE);
        ATUI_SET_COLOR(ed.fg_shortcut_desc, ed.bg_shortcut);
        ATUI_MVADDSTR(bar_y1, x + STRLEN(row1[i].key), " ");
        ATUI_ADDSTR(row1[i].desc);
    }

    /* Draw row 2 */
    ATUI_MOVE(bar_y2, 0);
    ATUI_SET_COLOR(ed.fg_shortcut_desc, ed.bg_shortcut);
    for (U32 i = 0; i < ed.screen_cols; i++) ATUI_ADDCH(' ');

    for (U32 i = 0; i < num2; i++) {
        U32 x = i * slot_w;
        ATUI_SET_COLOR(ed.fg_shortcut_key, ed.bg_shortcut);
        ATUI_ATTRON(ATUI_A_REVERSE);
        ATUI_MVADDSTR(bar_y2, x, row2[i].key);
        ATUI_ATTROFF(ATUI_A_REVERSE);
        ATUI_SET_COLOR(ed.fg_shortcut_desc, ed.bg_shortcut);
        ATUI_MVADDSTR(bar_y2, x + STRLEN(row2[i].key), " ");
        ATUI_ADDSTR(row2[i].desc);
    }
}

static VOID draw_status_bar(VOID) {
    U32 bar_y = ed.screen_rows - 3;

    ATUI_SET_COLOR(ed.fg_status, ed.bg_bar);
    ATUI_ATTRON(ATUI_A_REVERSE);
    ATUI_MOVE(bar_y, 0);
    for (U32 i = 0; i < ed.screen_cols; i++) ATUI_ADDCH(' ');

    if (ed.status_msg[0] && ed.status_tick > 0) {
        ATUI_MVADDSTR(bar_y, 2, ed.status_msg);
        ed.status_tick--;
        if (ed.status_tick == 0) ed.status_msg[0] = NULLT;
    }

    /* Right side: line/col info */
    CHAR info[48];
    SPRINTF(info, "Ln %d/%d  Col %d", ed.cur_row + 1, ed.line_count, ed.cur_col + 1);
    U32 infolen = STRLEN(info);
    if (infolen < ed.screen_cols - 2)
        ATUI_MVADDSTR(bar_y, ed.screen_cols - infolen - 2, info);

    ATUI_ATTROFF(ATUI_A_REVERSE);
}

static VOID draw_text(VOID) {
    U32 gutter = ed.show_line_numbers ? 5 : 0;
    U32 text_cols = ed.screen_cols > gutter ? ed.screen_cols - gutter : 1;

    ATUI_SET_COLOR(ed.fg_text, ed.bg_text);

    /* Pre-compute block-comment and bracket-depth state at first visible line */
    BOOL in_block_comment = FALSE;
    U32  bracket_depth    = 0;
    if (ed.syntax_enabled && ed.scroll_y > 0) {
        compute_syntax_state_at(ed.scroll_y, &in_block_comment, &bracket_depth);
    }

    U32 comment_len  = STRLEN(ed.syntax_comment_start);
    U32 bc_start_len = STRLEN(ed.syntax_block_comment_start);
    U32 bc_end_len   = STRLEN(ed.syntax_block_comment_end);

    for (U32 scr_row = 0; scr_row < ed.edit_rows; scr_row++) {
        U32 file_row = scr_row + ed.scroll_y;
        U32 y = scr_row + 1; /* row 0 = title bar */

        ATUI_MOVE(y, 0);

        if (file_row < ed.line_count) {
            /* Line number gutter */
            if (ed.show_line_numbers) {
                ATUI_SET_COLOR(ed.fg_line_number, ed.bg_text);
                ATUI_MVPRINTW(y, 0, "%4d ", file_row + 1);
                ATUI_SET_COLOR(ed.fg_text, ed.bg_text);
            }

            /* Line content with syntax highlighting */
            LINE_BUF *lb = &ed.lines[file_row];
            BOOL in_string = FALSE;
            CHAR string_char = 0;
            BOOL in_comment = FALSE;
            U32  kw_skip = 0;
            VBE_COLOUR kw_colour = VBE_SEE_THROUGH;
            U32  bc_end_skip = 0;   /* chars remaining of block-comment end token */

            for (U32 scr_col = 0; scr_col < text_cols; scr_col++) {
                U32 file_col = scr_col + ed.scroll_x;
                if (file_col < lb->len) {
                    CHAR c = lb->data[file_col];
                    VBE_COLOUR fg = VBE_SEE_THROUGH;

                    /* --- 1. Inside a block comment --- */
                    if (in_block_comment) {
                        fg = ed.syntax_comment_colour;
                        /* Check for block-comment end */
                        if (bc_end_skip == 0 && bc_end_len > 0 &&
                            file_col + bc_end_len <= lb->len) {
                            BOOL m = TRUE;
                            for (U32 ci = 0; ci < bc_end_len; ci++) {
                                if (lb->data[file_col + ci] != ed.syntax_block_comment_end[ci])
                                    { m = FALSE; break; }
                            }
                            if (m) bc_end_skip = bc_end_len;
                        }
                        if (bc_end_skip > 0) {
                            bc_end_skip--;
                            if (bc_end_skip == 0) in_block_comment = FALSE;
                        }
                    }
                    /* --- 2. Single-line comment (rest of line) --- */
                    else if (in_comment) {
                        fg = ed.syntax_comment_colour;
                    }
                    /* --- 3. Normal text — detect comments, strings, kw, brackets --- */
                    else {
                        /* 3a. Block comment start? (only outside strings) */
                        if (!in_string && ed.syntax_enabled &&
                            bc_start_len > 0 && file_col + bc_start_len <= lb->len) {
                            BOOL m = TRUE;
                            for (U32 ci = 0; ci < bc_start_len; ci++) {
                                if (lb->data[file_col + ci] != ed.syntax_block_comment_start[ci])
                                    { m = FALSE; break; }
                            }
                            if (m) {
                                in_block_comment = TRUE;
                                kw_skip = 0; /* reset keyword state */
                                fg = ed.syntax_comment_colour;
                            }
                        }

                        /* 3b. Single-line comment start? */
                        if (fg == VBE_SEE_THROUGH && !in_string && ed.syntax_enabled &&
                            comment_len > 0 && file_col + comment_len <= lb->len) {
                            BOOL m = TRUE;
                            for (U32 ci = 0; ci < comment_len; ci++) {
                                if (lb->data[file_col + ci] != ed.syntax_comment_start[ci])
                                    { m = FALSE; break; }
                            }
                            if (m) {
                                in_comment = TRUE;
                                kw_skip = 0;
                                fg = ed.syntax_comment_colour;
                            }
                        }

                        /* 3c. Syntax colour (strings, keywords, numbers) */
                        if (fg == VBE_SEE_THROUGH) {
                            fg = get_syntax_colour(lb->data, lb->len, file_col,
                                                   &in_string, &string_char,
                                                   &kw_skip, &kw_colour);
                        }

                        /* 3d. Bracket colouring (only outside strings) */
                        if (fg == VBE_SEE_THROUGH && !in_string &&
                            ed.syntax_enabled && is_bracket(c)) {
                            if (c == '(' || c == '[' || c == '{') {
                                fg = ed.syntax_bracket_colours[bracket_depth % 4];
                                bracket_depth++;
                            } else {
                                if (bracket_depth > 0) bracket_depth--;
                                fg = ed.syntax_bracket_colours[bracket_depth % 4];
                            }
                        }
                    }

                    if (fg != VBE_SEE_THROUGH)
                        ATUI_SET_COLOR(fg, ed.bg_text);
                    else
                        ATUI_SET_COLOR(ed.fg_text, ed.bg_text);

                    /* Tab expansion */
                    if (c == '\t') {
                        U32 spaces = ed.tab_size - (file_col % ed.tab_size);
                        for (U32 s = 0; s < spaces && scr_col < text_cols; s++) {
                            ATUI_ADDCH(' ');
                            if (s < spaces - 1) scr_col++;
                        }
                    } else {
                        ATUI_ADDCH(c);
                    }
                } else {
                    ATUI_SET_COLOR(ed.fg_text, ed.bg_text);
                    ATUI_ADDCH(' ');
                }
            }
        } else {
            /* Empty row — show tilde */
            if (ed.show_line_numbers) {
                ATUI_SET_COLOR(ed.fg_line_number, ed.bg_text);
                ATUI_MVPRINTW(y, 0, "   ~ ");
            } else {
                ATUI_SET_COLOR(ed.fg_line_number, ed.bg_text);
                ATUI_MVADDSTR(y, 0, "~");
            }
            /* Clear rest of row */
            ATUI_SET_COLOR(ed.fg_text, ed.bg_text);
            U32 cur_x, cur_y;
            ATUI_GETYX(&cur_y, &cur_x);
            for (U32 i = cur_x; i < ed.screen_cols; i++) ATUI_ADDCH(' ');
        }
    }
}

static VOID draw_help_screen(VOID) {
    ATUI_CLEAR();
    ATUI_SET_COLOR(ed.fg_bar, ed.bg_bar);
    ATUI_ATTRON(ATUI_A_REVERSE);
    ATUI_MOVE(0, 0);
    for (U32 i = 0; i < ed.screen_cols; i++) ATUI_ADDCH(' ');
    ATUI_MVPRINTW(0, ed.screen_cols / 2 - 10, "JOT Help — Press any key");
    ATUI_ATTROFF(ATUI_A_REVERSE);

    ATUI_SET_COLOR(ed.fg_text, ed.bg_text);

    PU8 help[] = {
        "",
        " JOT is a nano-style text editor for atOS.",
        "",
        " Navigation:",
        "   Arrow Keys       Move cursor",
        "   Home / End       Start / End of line",
        "   Page Up / Down   Scroll half screen",
        "   ^T               Go to top of file",
        "   ^B               Go to bottom of file",
        "   ^_ (Ctrl+_)      Go to line number",
        "",
        " Editing:",
        "   Type             Insert text at cursor",
        "   Enter            Split line / new line",
        "   Backspace        Delete char before cursor",
        "   Delete           Delete char at cursor",
        "   Tab              Insert spaces (tab size)",
        "   ^K               Cut current line",
        "   ^U               Uncut (paste last cut lines)",
        "   ^Z               Undo last action",
        "",
        " File:",
        "   ^O               Write Out (save file)",
        "   ^R               Read file into buffer",
        "   ^X               Exit editor",
        "",
        " Search & Replace:",
        "   ^W               Search (Where Is)",
        "   ^\\               Search and Replace",
        "",
        " Display:",
        "   ^C               Show cursor position",
        "   ^L               Refresh screen",
        "   ^G               This help screen",
        "",
        " Configuration:",
        "   Edit /ATOS/CONFS/JOT/JOT.CNF for settings & keybinds.",
        "   Syntax configs in /ATOS/CONFS/JOT/*_JOT.CNF.",
    };
    U32 hcount = sizeof(help) / sizeof(help[0]);
    for (U32 i = 0; i < hcount && i + 2 < ed.screen_rows; i++) {
        ATUI_MVADDSTR(i + 2, 0, help[i]);
    }

    ATUI_REFRESH();

    /* Wait for any key */
    PS2_KB_DATA *kb;
    do {
        kb = ATUI_GETCH();
    } while (!kb || !kb->cur.pressed);

    ed.show_help = FALSE;
}

static VOID draw_screen(VOID) {
    if (ed.show_help) {
        draw_help_screen();
        return;
    }

    scroll_to_cursor();

    draw_title_bar();
    draw_text();
    draw_status_bar();
    draw_shortcut_bar();

    /* Position cursor */
    U32 gutter = ed.show_line_numbers ? 5 : 0;
    U32 vis_row = ed.cur_row - ed.scroll_y + 1;
    U32 vis_col = ed.cur_col - ed.scroll_x + gutter;
    ATUI_MOVE(vis_row, vis_col);

    ATUI_REFRESH();
}

/* ====== Prompt line ====== */

static U32 prompt_input(PU8 prompt_text, PU8 buf, U32 buflen) {
    U32 bar_y = ed.screen_rows - 3;
    U32 prompt_len = STRLEN(prompt_text);
    U32 input_pos = STRLEN(buf);

    for (;;) {
        /* Draw prompt */
        ATUI_SET_COLOR(ed.fg_status, ed.bg_bar);
        ATUI_ATTRON(ATUI_A_REVERSE);
        ATUI_MOVE(bar_y, 0);
        for (U32 i = 0; i < ed.screen_cols; i++) ATUI_ADDCH(' ');
        ATUI_MVADDSTR(bar_y, 1, prompt_text);
        ATUI_MVADDSTR(bar_y, 1 + prompt_len, buf);
        ATUI_ATTROFF(ATUI_A_REVERSE);
        ATUI_MOVE(bar_y, 1 + prompt_len + input_pos);
        ATUI_REFRESH();

        PS2_KB_DATA *kb = ATUI_GETCH();
        if (!kb || !kb->cur.pressed) continue;

        MODIFIERS *mods = kb_mods();

        switch (kb->cur.keycode) {
        case KEY_ESC:
            return 0; /* Cancelled */
        case KEY_ENTER:
            return input_pos;
        case KEY_BACKSPACE:
            if (input_pos > 0) {
                input_pos--;
                buf[input_pos] = NULLT;
            }
            break;
        default:
            if (kb->cur.ASCII >= 32 && kb->cur.ASCII <= 126 && input_pos < buflen - 1) {
                buf[input_pos++] = kb->cur.ASCII;
                buf[input_pos] = NULLT;
            }
            break;
        }

        /* Allow Ctrl+C to cancel as well */
        if (mods->ctrl && kb->cur.keycode == KEY_C) return 0;
    }
}

/* ====== Editor operations ====== */

static VOID op_insert_char(CHAR ch) {
    LINE_BUF *lb = &ed.lines[ed.cur_row];

    /* Skip over closing pair character if it matches what's under cursor */
    if (ed.auto_pair_count > 0 && ed.cur_col < lb->len && lb->data[ed.cur_col] == ch) {
        for (U32 i = 0; i < ed.auto_pair_count; i++) {
            if (ch == ed.auto_pairs[i].close) {
                ed.cur_col++; /* just skip over */
                return;
            }
        }
    }

    line_insert_char(lb, ed.cur_col, ch);
    undo_push(UNDO_INSERT_CHAR, ed.cur_row, ed.cur_col, ch, NULLPTR);
    ed.cur_col++;
    ed.modified = TRUE;

    /* Auto-insert closing pair */
    if (ed.auto_pair_count > 0) {
        for (U32 i = 0; i < ed.auto_pair_count; i++) {
            if (ch == ed.auto_pairs[i].open) {
                CHAR close = ed.auto_pairs[i].close;
                /* For symmetric pairs (quotes), don't autocomplete inside words */
                if (ch == close) {
                    if (ed.cur_col >= 2 && is_word_char(lb->data[ed.cur_col - 2]))
                        break;
                }
                line_insert_char(lb, ed.cur_col, close);
                undo_push(UNDO_INSERT_CHAR, ed.cur_row, ed.cur_col, close, NULLPTR);
                break;
            }
        }
    }
}

static VOID op_insert_tab(VOID) {
    if (ed.tab_as_spaces) {
        U32 spaces = ed.tab_size - (ed.cur_col % ed.tab_size);
        for (U32 i = 0; i < spaces; i++) {
            op_insert_char(' ');
        }
    } else {
        op_insert_char('\t');
    }
}

static VOID op_delete_char_at(VOID) {
    LINE_BUF *lb = &ed.lines[ed.cur_row];
    if (ed.cur_col < lb->len) {
        CHAR ch = lb->data[ed.cur_col];
        line_delete_char(lb, ed.cur_col);
        undo_push(UNDO_DELETE_CHAR, ed.cur_row, ed.cur_col, ch, NULLPTR);
        ed.modified = TRUE;
    } else if (ed.cur_row + 1 < ed.line_count) {
        /* Join with next line */
        LINE_BUF *nxt = &ed.lines[ed.cur_row + 1];
        undo_push(UNDO_JOIN_LINE, ed.cur_row + 1, 0, 0, nxt->data);
        line_append(lb, nxt->data, nxt->len);
        line_free(nxt);
        MEMMOVE(&ed.lines[ed.cur_row + 1], &ed.lines[ed.cur_row + 2],
                (ed.line_count - ed.cur_row - 2) * sizeof(LINE_BUF));
        ed.line_count--;
        ed.modified = TRUE;
    }
}

static VOID op_backspace(VOID) {
    if (ed.cur_col > 0) {
        ed.cur_col--;
        LINE_BUF *lb = &ed.lines[ed.cur_row];
        CHAR ch = lb->data[ed.cur_col];
        line_delete_char(lb, ed.cur_col);
        undo_push(UNDO_DELETE_CHAR, ed.cur_row, ed.cur_col, ch, NULLPTR);
        ed.modified = TRUE;
    } else if (ed.cur_row > 0) {
        /* Join current line to end of previous */
        LINE_BUF *prev = &ed.lines[ed.cur_row - 1];
        LINE_BUF *cur  = &ed.lines[ed.cur_row];
        U32 join_col = prev->len;
        undo_push(UNDO_JOIN_LINE, ed.cur_row, 0, 0, cur->data);
        line_append(prev, cur->data, cur->len);
        line_free(cur);
        MEMMOVE(&ed.lines[ed.cur_row], &ed.lines[ed.cur_row + 1],
                (ed.line_count - ed.cur_row - 1) * sizeof(LINE_BUF));
        ed.line_count--;
        ed.cur_row--;
        ed.cur_col = join_col;
        ed.modified = TRUE;
    }
}

static VOID op_enter(VOID) {
    if (ed.line_count >= MAX_LINES) {
        set_status("[ Maximum lines reached ]");
        return;
    }

    LINE_BUF *cur = &ed.lines[ed.cur_row];

    /* Calculate auto-indent */
    U32 indent = 0;
    if (ed.auto_indent) {
        while (indent < cur->len && (cur->data[indent] == ' ' || cur->data[indent] == '\t'))
            indent++;
        if (indent > ed.cur_col) indent = ed.cur_col;
    }

    /* Split: text after cursor goes to new line */
    PU8 after = cur->data + ed.cur_col;
    U32 after_len = cur->len - ed.cur_col;

    /* Make room */
    MEMMOVE(&ed.lines[ed.cur_row + 2], &ed.lines[ed.cur_row + 1],
            (ed.line_count - ed.cur_row - 1) * sizeof(LINE_BUF));
    ed.line_count++;

    /* New line */
    line_init(&ed.lines[ed.cur_row + 1]);

    /* Auto-indent: copy leading whitespace */
    if (indent > 0) {
        line_append(&ed.lines[ed.cur_row + 1], cur->data, indent);
    }
    line_append(&ed.lines[ed.cur_row + 1], after, after_len);

    /* Truncate current line at cursor */
    cur->data[ed.cur_col] = NULLT;
    cur->len = ed.cur_col;

    undo_push(UNDO_SPLIT_LINE, ed.cur_row, ed.cur_col, 0, NULLPTR);

    ed.cur_row++;
    ed.cur_col = indent;
    ed.modified = TRUE;
}

static VOID op_cut_line(VOID) {
    if (ed.line_count <= 1 && ed.lines[0].len == 0) return;

    /* Append to cut buffer */
    if (ed.cut_count < CUT_BUFFER_SIZE) {
        ed.cut_buf[ed.cut_count] = STRDUP(ed.lines[ed.cur_row].data);
        ed.cut_count++;
    }

    undo_push(UNDO_DELETE_LINE, ed.cur_row, 0, 0, ed.lines[ed.cur_row].data);

    line_free(&ed.lines[ed.cur_row]);

    if (ed.line_count > 1) {
        MEMMOVE(&ed.lines[ed.cur_row], &ed.lines[ed.cur_row + 1],
                (ed.line_count - ed.cur_row - 1) * sizeof(LINE_BUF));
        ed.line_count--;
    } else {
        /* Last line: just clear it */
        line_init(&ed.lines[0]);
        ed.line_count = 1;
    }

    clamp_cursor();
    ed.cur_col = 0;
    ed.modified = TRUE;
    set_status("[ Cut 1 line ]");
}

static VOID op_uncut(VOID) {
    if (ed.cut_count == 0) {
        set_status("[ Cut buffer empty ]");
        return;
    }

    for (U32 i = 0; i < ed.cut_count; i++) {
        if (ed.line_count >= MAX_LINES) break;

        /* Insert a new line at cur_row */
        MEMMOVE(&ed.lines[ed.cur_row + 1], &ed.lines[ed.cur_row],
                (ed.line_count - ed.cur_row) * sizeof(LINE_BUF));
        ed.line_count++;

        line_init(&ed.lines[ed.cur_row]);
        line_set(&ed.lines[ed.cur_row], ed.cut_buf[i], STRLEN(ed.cut_buf[i]));
        undo_push(UNDO_INSERT_LINE, ed.cur_row, 0, 0, NULLPTR);
        ed.cur_row++;
    }

    ed.cur_col = 0;
    ed.modified = TRUE;

    CHAR msg[48];
    SPRINTF(msg, "[ Pasted %d line(s) ]", ed.cut_count);
    set_status(msg);
}

static VOID op_search(VOID) {
    CHAR prev_search[SEARCH_MAX];
    STRCPY(prev_search, ed.search_str);

    U32 len = prompt_input("Search: ", ed.search_str, SEARCH_MAX);
    if (len == 0) {
        /* If cancelled but there's a previous search, restore it */
        if (prev_search[0]) STRCPY(ed.search_str, prev_search);
        set_status("[ Search cancelled ]");
        return;
    }

    /* Search forward from current position */
    for (U32 i = 0; i < ed.line_count; i++) {
        U32 row = (ed.cur_row + i) % ed.line_count;
        PU8 line = ed.lines[row].data;
        PU8 found;

        if (ed.search_case_sensitive)
            found = STRSTR(line, ed.search_str);
        else
            found = STRISTR(line, ed.search_str);

        if (found) {
            U32 col = (U32)(found - line);
            /* Skip if same position on first iteration */
            if (i == 0 && col <= ed.cur_col) {
                /* Try finding next occurrence */
                PU8 next;
                if (ed.search_case_sensitive)
                    next = STRSTR(found + 1, ed.search_str);
                else
                    next = STRISTR(found + 1, ed.search_str);
                if (next) {
                    col = (U32)(next - line);
                } else {
                    continue;
                }
            }
            ed.cur_row = row;
            ed.cur_col = col;
            set_status("[ Found ]");
            return;
        }
    }

    set_status("[ Not found ]");
}

static VOID op_replace(VOID) {
    U32 slen = prompt_input("Search: ", ed.search_str, SEARCH_MAX);
    if (slen == 0) {
        set_status("[ Replace cancelled ]");
        return;
    }

    ed.replace_str[0] = NULLT;
    U32 rlen = prompt_input("Replace with: ", ed.replace_str, SEARCH_MAX);
    /* Allow empty replacement (deletion) */
    (VOID)rlen;

    U32 count = 0;
    U32 search_len = STRLEN(ed.search_str);
    U32 replace_len = STRLEN(ed.replace_str);

    for (U32 row = 0; row < ed.line_count; row++) {
        LINE_BUF *lb = &ed.lines[row];
        U32 pos = 0;
        while (pos + search_len <= lb->len) {
            PU8 found;
            if (ed.search_case_sensitive)
                found = STRSTR(lb->data + pos, ed.search_str);
            else
                found = STRISTR(lb->data + pos, ed.search_str);

            if (!found) break;

            U32 idx = (U32)(found - lb->data);

            /* Build new line */
            U32 new_len = lb->len - search_len + replace_len;
            PU8 new_data = MAlloc(new_len + 1);
            if (!new_data) break;

            MEMCPY(new_data, lb->data, idx);
            MEMCPY(new_data + idx, ed.replace_str, replace_len);
            MEMCPY(new_data + idx + replace_len, lb->data + idx + search_len,
                   lb->len - idx - search_len);
            new_data[new_len] = NULLT;

            line_set(lb, new_data, new_len);
            MFree(new_data);

            pos = idx + replace_len;
            count++;
        }
    }

    if (count > 0) {
        ed.modified = TRUE;
        CHAR msg[64];
        SPRINTF(msg, "[ Replaced %d occurrence(s) ]", count);
        set_status(msg);
    } else {
        set_status("[ No occurrences found ]");
    }
}

static VOID op_goto_line(VOID) {
    CHAR buf[16] = {0};
    U32 len = prompt_input("Enter line number: ", buf, sizeof(buf));
    if (len == 0) {
        set_status("[ Cancelled ]");
        return;
    }

    U32 line_num = ATOI(buf);
    if (line_num == 0) line_num = 1;
    if (line_num > ed.line_count) line_num = ed.line_count;
    ed.cur_row = line_num - 1;
    ed.cur_col = 0;

    CHAR msg[48];
    SPRINTF(msg, "[ Line %d ]", line_num);
    set_status(msg);
}

static VOID op_show_pos(VOID) {
    CHAR msg[80];
    SPRINTF(msg, "[ Line %d/%d, Col %d/%d, Char %d ]",
            ed.cur_row + 1, ed.line_count,
            ed.cur_col + 1, ed.lines[ed.cur_row].len,
            ed.cur_col < ed.lines[ed.cur_row].len ? ed.lines[ed.cur_row].data[ed.cur_col] : 0);
    set_status(msg);
}

static VOID op_read_file(VOID) {
    CHAR path[256] = {0};
    U32 len = prompt_input("File to read: ", path, sizeof(path));
    if (len == 0) {
        set_status("[ Cancelled ]");
        return;
    }

    FILE *f = FOPEN(path, MODE_FR);
    if (!f) {
        set_status("[ File not found ]");
        return;
    }

    U8 line_buf[MAX_LINE_LEN];
    U32 inserted = 0;

    while (FILE_GET_LINE(f, line_buf, sizeof(line_buf)) && ed.line_count < MAX_LINES) {
        U32 ll = STRLEN(line_buf);
        strip_cr(line_buf, &ll);

        /* Insert after current row */
        U32 insert_row = ed.cur_row + 1 + inserted;
        MEMMOVE(&ed.lines[insert_row + 1], &ed.lines[insert_row],
                (ed.line_count - insert_row) * sizeof(LINE_BUF));
        ed.line_count++;
        line_init(&ed.lines[insert_row]);
        line_set(&ed.lines[insert_row], line_buf, ll);
        inserted++;
    }

    FCLOSE(f);
    ed.modified = TRUE;

    CHAR msg[80];
    SPRINTF(msg, "[ Read %d lines from %s ]", inserted, path);
    set_status(msg);
}

static VOID op_write_out(VOID) {
    /* If no filename or new file, prompt for filename */
    if (!ed.filepath[0] || ed.new_file) {
        CHAR path[256] = {0};
        if (ed.filepath[0]) STRCPY(path, ed.filepath);
        INT result = ATUI_MSGBOX_INPUT("Save File",
            "File Name to Write:", path, sizeof(path));
        if (result == 0 || path[0] == NULLT) {
            set_status("[ Cancelled ]");
            return;
        }
        STRCPY(ed.filepath, path);
        ed.new_file = FALSE;
    }

    save_file();
}

/* ====== Input handling ====== */

static VOID handle_input(VOID) {
    PS2_KB_DATA *kb = ATUI_GETCH();
    if (!kb || !kb->cur.pressed) return;

    MODIFIERS *mods = kb_mods();
    KEYCODES key = kb->cur.keycode;

    /* ---- Ctrl shortcuts (configurable via JOT.CNF [KEYBINDS]) ---- */
    if (mods->ctrl) {
        JOT_ACTION action = lookup_action(key, TRUE, mods->shift);
        switch (action) {
        case JOT_ACT_EXIT:
            if (ed.modified) {
                INT result = ATUI_MSGBOX_YESNO("Unsaved Changes",
                    "Save changes before exiting?");
                if (result == 1) {
                    op_write_out();
                    if (ed.modified) return; /* save failed/cancelled */
                }
            }
            ed.running = FALSE;
            return;

        case JOT_ACT_SAVE:      op_write_out();  return;
        case JOT_ACT_CUT:       op_cut_line();   return;
        case JOT_ACT_UNCUT:     op_uncut();      return;
        case JOT_ACT_SEARCH:    op_search();     return;
        case JOT_ACT_REPLACE:   op_replace();    return;
        case JOT_ACT_HELP:      ed.show_help = TRUE; return;
        case JOT_ACT_UNDO:      undo_pop(); set_status("[ Undo ]"); return;
        case JOT_ACT_POSITION:  op_show_pos();   return;
        case JOT_ACT_READ_FILE: op_read_file();  return;
        case JOT_ACT_GOTO_LINE: op_goto_line();  return;

        case JOT_ACT_TOP:
            ed.cur_row = 0; ed.cur_col = 0;
            set_status("[ Top of file ]"); return;

        case JOT_ACT_BOTTOM:
            ed.cur_row = ed.line_count > 0 ? ed.line_count - 1 : 0;
            ed.cur_col = ed.lines[ed.cur_row].len;
            set_status("[ Bottom of file ]"); return;

        case JOT_ACT_REFRESH:   ATUI_TOUCHSCREEN(); return;
        case JOT_ACT_HOME:      ed.cur_col = 0; return;
        case JOT_ACT_END:       ed.cur_col = ed.lines[ed.cur_row].len; return;
        case JOT_ACT_COUNT:
        default: break;
        }
        return; /* Don't insert ctrl combos as chars */
    }

    /* ---- Navigation keys ---- */
    switch (key) {
    case KEY_ARROW_UP:
        if (ed.cur_row > 0) {
            ed.cur_row--;
            if (ed.cur_col > ed.lines[ed.cur_row].len)
                ed.cur_col = ed.lines[ed.cur_row].len;
        }
        return;

    case KEY_ARROW_DOWN:
        if (ed.cur_row + 1 < ed.line_count) {
            ed.cur_row++;
            if (ed.cur_col > ed.lines[ed.cur_row].len)
                ed.cur_col = ed.lines[ed.cur_row].len;
        }
        return;

    case KEY_ARROW_LEFT:
        if (ed.cur_col > 0) {
            ed.cur_col--;
        } else if (ed.cur_row > 0) {
            ed.cur_row--;
            ed.cur_col = ed.lines[ed.cur_row].len;
        }
        return;

    case KEY_ARROW_RIGHT:
        if (ed.cur_col < ed.lines[ed.cur_row].len) {
            ed.cur_col++;
        } else if (ed.cur_row + 1 < ed.line_count) {
            ed.cur_row++;
            ed.cur_col = 0;
        }
        return;

    case KEY_HOME:
        ed.cur_col = 0;
        return;

    case KEY_END:
        ed.cur_col = ed.lines[ed.cur_row].len;
        return;

    case KEY_PAGEUP: {
        U32 jump = ed.edit_rows / 2;
        if (ed.cur_row >= jump)
            ed.cur_row -= jump;
        else
            ed.cur_row = 0;
        clamp_cursor();
        return;
    }

    case KEY_PAGEDOWN: {
        U32 jump = ed.edit_rows / 2;
        ed.cur_row += jump;
        clamp_cursor();
        return;
    }

    case KEY_ENTER:
        /* Clear cut buffer on enter so next ^K starts fresh */
        for (U32 i = 0; i < ed.cut_count; i++) {
            if (ed.cut_buf[i]) MFree(ed.cut_buf[i]);
            ed.cut_buf[i] = NULLPTR;
        }
        ed.cut_count = 0;
        op_enter();
        return;

    case KEY_BACKSPACE:
        op_backspace();
        return;

    case KEY_DELETE:
        op_delete_char_at();
        return;

    case KEY_TAB:
        op_insert_tab();
        return;

    case KEY_ESC:
        return; /* Ignore escape on its own */

    default:
        break;
    }

    /* ---- Printable characters ---- */
    if (kb->cur.ASCII >= 32 && kb->cur.ASCII <= 126) {
        op_insert_char(kb->cur.ASCII);
    }
}

/* ====== Cleanup ====== */

static VOID cleanup_editor(VOID) {
    /* Free all lines */
    for (U32 i = 0; i < ed.line_count; i++) {
        line_free(&ed.lines[i]);
    }

    /* Free cut buffer */
    for (U32 i = 0; i < ed.cut_count; i++) {
        if (ed.cut_buf[i]) MFree(ed.cut_buf[i]);
    }

    /* Free undo records */
    for (U32 i = 0; i < ed.undo_top; i++) {
        if (ed.undo[i].line_data) MFree(ed.undo[i].line_data);
    }

    /* Free config */
    if (ed.config) DESTROY_ATRCFD(ed.config);
    if (ed.syntax_config) DESTROY_ATRCFD(ed.syntax_config);
}

static VOID exit_jot(VOID) {
    cleanup_editor();
    ATUI_DESTROY();
    ENABLE_SHELL_KEYBOARD();
}

/* ====== Entry point ====== */

CMAIN() {
    MEMZERO(&ed, sizeof(JOT_STATE));

    DISABLE_SHELL_KEYBOARD();
    ON_EXIT(exit_jot);
    ON_EXIT(ENABLE_SHELL_KEYBOARD);
    /* Load configuration */
    load_config();
    
    /* Initialise ATUI */
    ATUI_INITIALIZE(NULLPTR, ATUIC_RAW);
    ATUI_CURSOR_SET(TRUE);
    ATUI_CURS_SET(2); /* Block cursor */

    /* Get screen dimensions */
    ATUI_DISPLAY *disp = GET_DISPLAY();
    ed.screen_rows = disp->rows;
    ed.screen_cols = disp->cols;
    ed.edit_rows   = ed.screen_rows - 4; /* title, status, 2 shortcut rows */

    /* Parse filename from args */
    PU8 filename = NULLPTR;
    if (argc > 1) filename = argv[1];

    /* Load file */
    load_file(filename);

    /* Set up syntax highlighting based on file extension */
    detect_syntax();

    /* Apply text colours */
    ATUI_SET_COLOR(ed.fg_text, ed.bg_text);

    /* Main loop */
    ed.running = TRUE;
    while (ed.running) {
        draw_screen();
        handle_input();
    }

    /* Cleanup happens via ON_EXIT */
    return 0;
}
