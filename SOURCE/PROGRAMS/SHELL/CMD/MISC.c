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

VOID CMD_RESTART(PU8 line){
    (void) line;
    SYS_RESTART();
}
VOID CMD_CLEAR(U8 *line) { (void)line; LE_CLS(); }
VOID CMD_VERSION(U8 *line) { (void)line; PRINTNEWLINE(); PUTS((U8*)"atOS Shell version 1.0.0" LEND); }
VOID CMD_EXIT(U8 *line) { 
    (void)line; 
    PRINTNEWLINE(); 
    PUTS((U8*)"Exiting shell..." LEND);
    EXIT_SHELL();
}

VOID CMD_ECHO(U8 *line) {
    if(!batsh_mode) PRINTNEWLINE(); 
    if (STRLEN(line) > 5) PUTS(line + 5);
    if(!batsh_mode) PRINTNEWLINE(); 
}

VOID CMD_SHELL(U8 *line) {
    PU8 path = 
        "/PROGRAMS/ATOSHELL/ATOSHELL.BIN";
    FILE *f = FOPEN(path, MODE_FR);
    U8 *shell_argv[] = { path , "--legitemate-run", NULLPTR };

    START_PROCESS(
        path,
        f->data,
        f->sz,
        TCB_STATE_ACTIVE | TCB_STATE_INFO_CHILD_PROC_HANDLER,
        0,
        shell_argv,
        2
    );
}

VOID CMD_UNKNOWN(U8 *line) {
    if(!RUN_PROCESS(line)) {
        PRINTNEWLINE();
        PUTS((U8*)"Unknown command or file type: ");
        PUTS(line);
        PRINTNEWLINE();
    }
}