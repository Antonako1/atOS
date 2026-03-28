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
VOID CMD_SHUTDOWN(PU8 line) {
    (void) line;
    SYS_SHUTDOWN();
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
        "/ATOS/TSHELL.BIN";
    FILE *f = FOPEN(path, MODE_FR);
    if (!f) {
        PRINTNEWLINE();
        PUTS((U8*)"Failed to open shell binary: ");
        PUTS(path);
        PRINTNEWLINE();
        return;
    }
    PU8 shell_argv = MAlloc(sizeof(U8*) * 3);
    if (!shell_argv) {
        FCLOSE(f);
        PRINTNEWLINE();
        PUTS((U8*)"Failed to allocate memory for shell arguments");
        PRINTNEWLINE();
        return;
    }
    shell_argv[0] = STRDUP(path);
    shell_argv[1] = STRDUP("-c");
    shell_argv[2] = NULLPTR;
    // U8 *shell_argv[3] = { STRDUP("/ATOS/TSHELL.BIN"), STRDUP("-c"), NULLPTR };

    START_PROCESS(
        path,
        f->data,
        f->sz,
        TCB_STATE_ACTIVE | TCB_STATE_INFO_CHILD_PROC_HANDLER,
        0,
        shell_argv,
        2
    );
    FCLOSE(f);
}

VOID CMD_UNKNOWN(U8 *line) {
    if(!RUN_PROCESS(line)) {
        PRINTNEWLINE();
        PUTS((U8*)"Unknown command or file type: ");
        PUTS(line);
        PRINTNEWLINE();
    }
}