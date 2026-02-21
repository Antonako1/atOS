VOID CMD_RMDIR(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);
    PU8 path_arg = PARSE_CD_RAW_LINE(tmp_line, 5);
    if (!REMOVE_DIR(path_arg)) return;
}