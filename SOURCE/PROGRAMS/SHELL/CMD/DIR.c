VOID CMD_DIR(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);

    PU8 path_arg = PARSE_CD_RAW_LINE(tmp_line, 3);
    if (STRCMP(path_arg, "-") == 0) path_arg = GET_PREV_PATH();
    else if (STRCMP(path_arg, "~") == 0) path_arg = (PU8)"/HOME";
    else if (!path_arg || *path_arg == '\0') path_arg = (PU8)".";
    PRINT_CONTENTS_PATH(path_arg);
    PRINTNEWLINE();
}