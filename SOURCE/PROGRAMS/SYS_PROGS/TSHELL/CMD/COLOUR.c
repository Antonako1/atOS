/* CMD/COLOUR.c — Color management command for TSHELL (with ANSI support) */

typedef struct {
    U32 id;
    PU8 name;
    VBE_COLOUR value;
    U8 ansi_fg;     /* ANSI SGR foreground code    */
    U8 ansi_bg;     /* ANSI SGR background code    */
} COLOUR_ENTRY;

static const COLOUR_ENTRY colour_table[] = {
    { 0,  "black",   VBE_BLACK,    30, 40 },
    { 1,  "white",   VBE_WHITE,    37, 47 },
    { 2,  "red",     VBE_RED,      31, 41 },
    { 3,  "green",   VBE_GREEN,    32, 42 },
    { 4,  "blue",    VBE_BLUE,     34, 44 },
    { 5,  "yellow",  VBE_YELLOW,   33, 43 },
    { 6,  "cyan",    VBE_CYAN,     36, 46 },
    { 7,  "magenta", VBE_MAGENTA,  35, 45 },
    { 8,  "gray",    VBE_GRAY,     90, 100 },
    { 9,  "orange",  VBE_ORANGE,   33, 43 },
    { 10, "brown",   VBE_BROWN,    33, 43 },
    { 11, "pink",    VBE_PINK,     91, 101 },
    { 12, "purple",  VBE_PURPLE,   95, 105 },
    { 13, "lime",    VBE_LIME,     92, 102 },
    { 14, "navy",    VBE_NAVY,     94, 104 },
    { 15, "gold",    VBE_GOLD,     93, 103 },
};

#define COLOUR_COUNT (sizeof(colour_table) / sizeof(COLOUR_ENTRY))

VBE_COLOUR FIND_VBE_COLOUR(PU8 arg, VBE_COLOUR fallback) {
    if (!arg) return fallback;
    for (U32 i = 0; i < COLOUR_COUNT; i++) {
        if ((ATOI(arg) == (I32)colour_table[i].id && IS_DIGIT(arg[0])) ||
            STRCMP(arg, colour_table[i].name) == 0) {
            return colour_table[i].value;
        }
    }
    return fallback;
}

VOID PUTS_PADDED(PU8 str, U32 width) {
    PUTS(str);
    I32 len = STRLEN(str);
    for (I32 i = len; i < (I32)width; i++) PUTS(" ");
}

VOID CMD_COLOUR(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);

    TSHELL_INSTANCE *inst = GET_SHNDL();
    if (!inst) return;

    ARG_ARRAY arr;
    RAW_LINE_TO_ARG_ARRAY(tmp_line, &arr);

    BOOL8 show_help = FALSE;
    BOOL8 be_quiet = FALSE;
    if (arr.argc <= 1) show_help = TRUE;
    else {
        for (U32 i = 0; i < arr.argc; i++) {
            if (!arr.argv[i]) break;
            if (STRCMP(arr.argv[i], "-h") == 0 || STRCMP(arr.argv[i], "--help") == 0) {
                show_help = TRUE; break;
            }
            if (STRCMP(arr.argv[i], "-q") == 0 || STRCMP(arr.argv[i], "--quiet") == 0) {
                be_quiet = TRUE; break;
            }
        }
    }

    if (show_help) {
        PUTS("\nUsage: colour <fg> <bg> [-h] [-q (q for quiet)]\n");
        PUTS("Example: 'colour red black' or 'colour 2 0'\n\n");
        PUTS("ID   NAME          ID   NAME\n");
        PUTS("----------------------------\n");

        for (U32 i = 0; i < COLOUR_COUNT; i += 2) {
            CHAR line[128];
            CHAR *p = line;
            CHAR id_buf[10];
            U8 esc[16];

            ITOA_U(colour_table[i].id, id_buf, 10);
            I32 len = STRLEN(id_buf);
            MEMCPY(p, id_buf, len); p += len;
            for (I32 j = len; j < 5; j++) *p++ = ' ';

            SPRINTF(esc, "\x1B[%dm", colour_table[i].ansi_fg);
            len = STRLEN(esc); MEMCPY(p, esc, len); p += len;
            len = STRLEN(colour_table[i].name); MEMCPY(p, colour_table[i].name, len); p += len;
            for (I32 j = len; j < 14; j++) *p++ = ' ';
            MEMCPY(p, "\x1B[0m", 4); p += 4;

            if (i + 1 < COLOUR_COUNT) {
                ITOA_U(colour_table[i+1].id, id_buf, 10);
                len = STRLEN(id_buf);
                for (I32 j = 0; j < 5 - len; j++) *p++ = ' ';
                MEMCPY(p, id_buf, len); p += len;

                SPRINTF(esc, "\x1B[%dm", colour_table[i+1].ansi_fg);
                len = STRLEN(esc); MEMCPY(p, esc, len); p += len;
                len = STRLEN(colour_table[i+1].name); MEMCPY(p, colour_table[i+1].name, len); p += len;
                for (I32 j = len; j < 14; j++) *p++ = ' ';
                MEMCPY(p, "\x1B[0m", 4); p += 4;
            }
            *p++ = '\n';
            *p = 0;
            PUTS(line);
        }
    } else {
        VBE_COLOUR new_fg = FIND_VBE_COLOUR(arr.argv[1], inst->tout->fg);
        VBE_COLOUR new_bg = FIND_VBE_COLOUR(arr.argv[2], inst->tout->bg);

        inst->tout->fg         = new_fg;
        inst->tout->bg         = new_bg;
        inst->tout->default_fg = new_fg;
        inst->tout->default_bg = new_bg;

        ATUI_WSET_COLOR(inst->tout->win, new_fg, new_bg);
        REDRAW_CONSOLE();
        if (be_quiet) PUTS("\n");
        else PUTS("\nConsole colours updated.\n");
    }

    DELETE_ARG_ARRAY(&arr);
}
