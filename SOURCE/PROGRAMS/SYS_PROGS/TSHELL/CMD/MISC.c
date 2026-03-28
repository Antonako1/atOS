/* CMD/MISC.c — General commands for TSHELL */

VOID CMD_HELP(U8 *line) {
    (void)line;
    PRINTNEWLINE();
    PUTS("Available commands:" LEND);
    for (U32 i = 0; i < shell_command_count; i++) {
        PUTS(shell_commands[i].name);
        PUTS(" - ");
        PUTS(shell_commands[i].help);
        PRINTNEWLINE();
    }
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
    }
}
