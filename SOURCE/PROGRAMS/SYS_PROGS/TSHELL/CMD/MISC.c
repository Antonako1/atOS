/* CMD/MISC.c — General commands for TSHELL */

/* Print a string padded to `width` chars with spaces */
static VOID tput_padded(PU8 str, U32 width) {
    U32 len = STRLEN(str);
    PUTS(str);
    for (U32 i = len; i < width; i++) PUTC(' ');
}

VOID CMD_HELP(U8 *line) {
    /* Optional topic: `help batsh` or `help keys` */
    BOOL show_cmds = TRUE, show_keys = TRUE, show_batsh = TRUE;
    if (line && STRLEN(line) > 5) {
        PU8 topic = line + 5;
        while (*topic == ' ') topic++;
        if (STRICMP(topic, "batsh") == 0) { show_cmds = FALSE; show_keys = FALSE; }
        else if (STRICMP(topic, "keys") == 0) { show_cmds = FALSE; show_batsh = FALSE; }
        else if (STRICMP(topic, "cmds") == 0) { show_keys = FALSE; show_batsh = FALSE; }
    }

    TPUT_BEGIN();
    PRINTNEWLINE();

    /* -- Header -- */
    TPUT_SET_FG(VBE_LIGHT_CYAN);
    PUTS("  BATSH v1.0 - atOS Shell" LEND);
    TPUT_SET_FG(VBE_DARK_GRAY);
    PUTS("  Better AT Shell  |  help [cmds|keys|batsh]" LEND LEND);

    /* -- Commands -- */
    if (show_cmds) {
        TPUT_SET_FG(VBE_YELLOW);
        PUTS("  COMMANDS" LEND);
        TPUT_SET_FG(VBE_DARK_GRAY);
        PUTS("  ----------------------------------------------------------------" LEND);
        TPUT_SET_FG(VBE_WHITE);

        U32 half = (shell_command_count + 1) / 2;
        for (U32 i = 0; i < half; i++) {
            /* Left column */
            PUTS("  ");
            TPUT_SET_FG(VBE_LIGHT_GREEN);
            tput_padded((PU8)shell_commands[i].name, 12);
            TPUT_SET_FG(VBE_LIGHT_GRAY);
            tput_padded((PU8)shell_commands[i].help, 31);

            /* Right column (if exists) */
            U32 j = i + half;
            if (j < shell_command_count) {
                PUTS("  ");
                TPUT_SET_FG(VBE_LIGHT_GREEN);
                tput_padded((PU8)shell_commands[j].name, 12);
                TPUT_SET_FG(VBE_LIGHT_GRAY);
                PUTS(shell_commands[j].help);
            }
            PRINTNEWLINE();
        }
        PRINTNEWLINE();
    }

    /* -- Keyboard shortcuts -- */
    if (show_keys) {
        TPUT_SET_FG(VBE_YELLOW);
        PUTS("  KEYBOARD SHORTCUTS" LEND);
        TPUT_SET_FG(VBE_DARK_GRAY);
        PUTS("  ----------------------------------------------------------------" LEND);

        typedef struct { PU8 key; PU8 desc; } KEYDESC;
        static const KEYDESC keys[] = {
            { "Enter",          "Execute current line / confirm" },
            { "Tab",            "Autocomplete file / command name" },
            { "Arrow Up/Down",  "Navigate command history" },
            { "Arrow Left/Right","Move cursor within line" },
            { "Home / End",     "Jump to start / end of line" },
            { "Ctrl+A / E",     "Jump to start / end of line" },
            { "Ctrl+Left/Right","Move word by word" },
            { "Ctrl+W",         "Delete word before cursor" },
            { "Ctrl+U",         "Clear from cursor to start" },
            { "Ctrl+K",         "Clear from cursor to end" },
            { "Ctrl+Y",         "Paste (yank) cleared text" },
            { "Ctrl+L",         "Clear screen (like cls)" },
            { "Ctrl+C",         "Interrupt / cancel current input" },
            { "Backspace",      "Delete char before cursor" },
            { "Delete",         "Delete char at cursor" },
            { "Ctrl+Backspace", "Delete word before cursor" },
            { "Ctrl+Delete",    "Delete word after cursor" },
            { "PgUp / PgDn",    "Scroll terminal output" },
            { "Shift+Tab",      "Cycle autocomplete backwards" },
        };
        U32 key_count = sizeof(keys) / sizeof(keys[0]);
        U32 half_k = (key_count + 1) / 2;

        for (U32 i = 0; i < half_k; i++) {
            PUTS("  ");
            TPUT_SET_FG(VBE_LIGHT_CYAN);
            tput_padded((PU8)keys[i].key, 20);
            TPUT_SET_FG(VBE_LIGHT_GRAY);
            tput_padded((PU8)keys[i].desc, 31);

            U32 j = i + half_k;
            if (j < key_count) {
                PUTS("  ");
                TPUT_SET_FG(VBE_LIGHT_CYAN);
                tput_padded((PU8)keys[j].key, 20);
                TPUT_SET_FG(VBE_LIGHT_GRAY);
                PUTS(keys[j].desc);
            }
            PRINTNEWLINE();
        }
        PRINTNEWLINE();
    }

    /* -- BATSH syntax -- */
    if (show_batsh) {
        TPUT_SET_FG(VBE_YELLOW);
        PUTS("  BATSH SCRIPT SYNTAX" LEND);
        TPUT_SET_FG(VBE_DARK_GRAY);
        PUTS("  ----------------------------------------------------------------" LEND);

        TPUT_SET_FG(VBE_LIGHT_GRAY);
        PUTS("  Comments      ");
        TPUT_SET_FG(VBE_DARK_GRAY);
        PUTS("# comment     rem comment" LEND);

        TPUT_SET_FG(VBE_LIGHT_GRAY);
        PUTS("  Stacking      ");
        TPUT_SET_FG(VBE_DARK_GRAY);
        PUTS("cmd1; cmd2; cmd3" LEND);

        TPUT_SET_FG(VBE_LIGHT_GRAY);
        PUTS("  Local var     ");
        TPUT_SET_FG(VBE_DARK_GRAY);
        PUTS("@name=value    echo @{name}" LEND);

        TPUT_SET_FG(VBE_LIGHT_GRAY);
        PUTS("  Global var    ");
        TPUT_SET_FG(VBE_DARK_GRAY);
        PUTS("@@name=value   echo @{name}" LEND);

        TPUT_SET_FG(VBE_LIGHT_GRAY);
        PUTS("  Arithmetic    ");
        TPUT_SET_FG(VBE_DARK_GRAY);
        PUTS("@count = 5 + 3   (in IF / LOOP / assign)" LEND);

        TPUT_SET_FG(VBE_LIGHT_GRAY);
        PUTS("  Conditional   ");
        TPUT_SET_FG(VBE_DARK_GRAY);
        PUTS("IF a EQU b THEN ... [ELSE ...] FI" LEND);
        PUTS("                ");
        PUTS("ops: EQU NEQ LSS LEQ GTR GEQ EXISTS" LEND);

        TPUT_SET_FG(VBE_LIGHT_GRAY);
        PUTS("  Range loop    ");
        TPUT_SET_FG(VBE_DARK_GRAY);
        PUTS("LOOP 1 TO 10 ... END   (counter: @{I})" LEND);

        TPUT_SET_FG(VBE_LIGHT_GRAY);
        PUTS("  Cond loop     ");
        TPUT_SET_FG(VBE_DARK_GRAY);
        PUTS("LOOP @{x} LSS 10 ... END   BREAK to exit" LEND);

        TPUT_SET_FG(VBE_LIGHT_GRAY);
        PUTS("  Run script    ");
        TPUT_SET_FG(VBE_DARK_GRAY);
        PUTS("scriptname.SH [args...]" LEND);
        PRINTNEWLINE();
    }

    TPUT_SET_FG(VBE_WHITE);
    TPUT_END();
}

VOID CMD_RESTART(PU8 line) { (void)line; SYS_RESTART(); }
VOID CMD_SHUTDOWN(PU8 line) { (void)line; SYS_SHUTDOWN(); }
VOID CMD_CLEAR(U8 *line) { (void)line; HANDLE_LE_CTRL_L(); }

VOID CMD_VERSION(U8 *line) {
    (void)line;
    PRINTNEWLINE();
    PUTS("TSHELL v1.0.0 (ATUI)" LEND);
}

VOID CMD_EXIT(U8 *line) {
    (void)line;
    PRINTNEWLINE();
    PUTS("Exiting shell..." LEND);
    EXIT_SHELL();
}

VOID CMD_ECHO(U8 *line) {
    if (!batsh_mode) PRINTNEWLINE();
    if (STRLEN(line) > 5) PUTS(line + 5);
    if (!batsh_mode) PRINTNEWLINE();
}

VOID CMD_SHELL(U8 *line) {
    (void)line;
    PU8 path = "/PROGRAMS/ATOSHELL/ATOSHELL.BIN";
    FILE *f = FOPEN(path, MODE_FR);
    if (!f) { PUTS("\nFailed to open shell binary.\n"); return; }
    U8 *shell_argv[] = { path, "--legitemate-run", NULLPTR };
    START_PROCESS(
        path, f->data, f->sz,
        TCB_STATE_ACTIVE | TCB_STATE_INFO_CHILD_PROC_HANDLER,
        0, shell_argv, 2
    );
}

VOID CMD_UNKNOWN(U8 *line) {
    if (!RUN_PROCESS(line)) {
        PRINTNEWLINE();
        PUTS("Unknown command or file type: ");
        PUTS(line);
        PRINTNEWLINE();
    } else {
        // TPUT_NEWLINE();
        // PUT_SHELL_START();
    }
}
