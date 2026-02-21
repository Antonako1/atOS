// Directory-related
VOID CMD_CD(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);

    PU8 path_arg = PARSE_CD_RAW_LINE(tmp_line, 2);
    if (STRCMP(path_arg, "-") == 0) CD_PREV();
    else if (STRCMP(path_arg, "~") == 0) CD_INTO((PU8)"/HOME");
    else if (!path_arg || *path_arg == '\0') CD_INTO((PU8)"/");
    else CD_INTO(path_arg);
    SET_VAR("CD", GET_PATH());
}

VOID CMD_CD_BACKWARDS(PU8 line) { 
    (void)line; CD_BACKWARDS_DIR(); PRINTNEWLINE(); 
    SET_VAR("CD", GET_PATH());
}