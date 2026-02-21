typedef struct {
    U32 id;
    PU8 name;
    VBE_COLOUR value;
} COLOUR_ENTRY;

static const COLOUR_ENTRY colour_table[] = {
    {0,  "black",   VBE_BLACK},   {1,  "white",   VBE_WHITE},
    {2,  "red",     VBE_RED},     {3,  "green",   VBE_GREEN},
    {4,  "blue",    VBE_BLUE},    {5,  "yellow",  VBE_YELLOW},
    {6,  "cyan",    VBE_CYAN},    {7,  "magenta", VBE_MAGENTA},
    {8,  "gray",    VBE_GRAY},    {9,  "orange",  VBE_ORANGE},
    {10, "brown",   VBE_BROWN},   {11, "pink",    VBE_PINK},
    {12, "purple",  VBE_PURPLE},  {13, "lime",    VBE_LIME},
    {14, "navy",    VBE_NAVY},    {15, "gold",    VBE_GOLD}
};

#define COLOUR_COUNT (sizeof(colour_table) / sizeof(COLOUR_ENTRY))

// Helper to find color by ID or Name
VBE_COLOUR FIND_VBE_COLOUR(PU8 arg, VBE_COLOUR fallback) {
    if (!arg) return fallback;
    
    for (U32 i = 0; i < COLOUR_COUNT; i++) {
        // Check if argument matches ID (numeric) or Name (string)
        if ((ATOI(arg) == (I32)colour_table[i].id && IS_DIGIT(arg[0])) || 
            STRCMP(arg, colour_table[i].name) == 0) {
            return colour_table[i].value;
        }
    }
    return fallback;
}


// Small helper to print a column with padding since we can't use printf %-12s
VOID PUTS_PADDED(PU8 str, U32 width) {
    PUTS(str);
    I32 len = STRLEN(str);
    for (I32 i = len; i < (I32)width; i++) {
        PUTS(" "); // Or use a single putch(' ') if you have it
    }
}

VOID CMD_COLOUR(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);
    
    SHELL_INSTANCE *inst = GET_SHNDL();
    if (!inst) return;

    ARG_ARRAY arr;
    RAW_LINE_TO_ARG_ARRAY(tmp_line, &arr);

    BOOL8 show_help = FALSE;
    if (arr.argc < 3) {
        show_help = TRUE;
    } else {
        for (U32 i = 0; i < arr.argc; i++) {
            if (STRCMP(arr.argv[i], "-h") == 0 || STRCMP(arr.argv[i], "--help") == 0) {
                show_help = TRUE;
                break;
            }
        }
    }

    if (show_help) {
        PUTS("\nUsage: colour <fg> <bg> [-h]\n");
        PUTS("Example: 'colour red black' or 'colour 2 0'\n\n");
        PUTS("ID   NAME          ID   NAME\n");
        PUTS("----------------------------\n");

        for (U32 i = 0; i < COLOUR_COUNT; i += 2) {
            CHAR id_buf[10];
            
            // Print first column
            ITOA_U(colour_table[i].id, id_buf, 10);
            PUTS_PADDED(id_buf, 5);
            PUTS_PADDED(colour_table[i].name, 14);

            // Print second column (if it exists)
            if (i + 1 < COLOUR_COUNT) {
                ITOA_U(colour_table[i+1].id, id_buf, 10);
                PUTS_PADDED(id_buf, 5);
                PUTS_PADDED(colour_table[i+1].name, 14);
            }
            PUTS("\n");
        }
    } else {
        inst->cursor->fgColor = FIND_VBE_COLOUR(arr.argv[1], inst->cursor->fgColor);
        inst->cursor->bgColor = FIND_VBE_COLOUR(arr.argv[2], inst->cursor->bgColor);
        
        REDRAW_CONSOLE();
        PUTS("\nConsole colours updated.\n");
    }

    DELETE_ARG_ARRAY(&arr);
}