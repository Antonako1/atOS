// ===================================================
// Command Implementations
// ===================================================
VOID CMD_HELP(U8 *line) {
    (void)line;
    PRINTNEWLINE();
    PUTS((U8*)"Available commands:" LEND);
    for (U32 i = 0; i < shell_command_count; i++) {
        PUTS((U8*)shell_commands[i].name);
        PUTS((U8*)" - ");
        PUTS((U8*)shell_commands[i].help);
        PRINTNEWLINE();
    }
}

VOID CMD_CLEAR(U8 *line) { (void)line; LE_CLS(); }
VOID CMD_VERSION(U8 *line) { (void)line; PRINTNEWLINE(); PUTS((U8*)"atOS Shell version 1.0.0" LEND); }
VOID CMD_EXIT(U8 *line) { (void)line; PRINTNEWLINE(); PUTS((U8*)"Exiting shell..." LEND); }

VOID CMD_ECHO(U8 *line) {
    PRINTNEWLINE();
    if (STRLEN(line) > 5) PUTS(line + 5);
    PRINTNEWLINE();
}


VOID CMD_UNKNOWN(U8 *line) {
    if(!RUN_PROCESS(line)) {
        PRINTNEWLINE();
        PUTS((U8*)"Unknown command or file type: ");
        PUTS(line);
        PRINTNEWLINE();
    }
}