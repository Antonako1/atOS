VOID CMD_TAIL(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);
    PU8 path_arg = PARSE_CD_RAW_LINE(tmp_line, 4);
    if (!path_arg || !*path_arg) {
        PUTS("\ntail: missing file path.\n");
        return;
    }

    FAT_LFN_ENTRY entry;
    if (!FAT32_PATH_RESOLVE_ENTRY(path_arg, &entry)) {
        PUTS("\ntail: file not found.\n");
        return;
    }

    if (entry.entry.ATTRIB & FAT_ATTRB_DIR) {
        PUTS("\ntail: cannot read a directory.\n");
        return;
    }

    U32 file_size = entry.entry.FILE_SIZE;
    if (file_size == 0) {
        PUTS("\n(empty file)\n");
        return;
    }
    U8 *buffer = FAT32_READ_FILE_CONTENTS(&file_size, &entry.entry);
    if (!buffer) {
        PUTS("\ntype: failed to read file.\n");
        MFree(buffer);
        return;
    }

    buffer[file_size] = '\0';

    // Find start of last N lines
    U32 line_count = 0;
    for (U32 i = file_size; i > 0; i--) {
        if (buffer[i - 1] == '\n') {
            line_count++;
            if (line_count > TAIL_LINE_COUNT) {
                buffer[i] = '\0';
                PUTS("\n");
                PUTS(&buffer[i]);
                PUTS("\n");
                MFree(buffer);
                return;
            }
        }
    }

    // If fewer than N lines, print all
    PUTS("\n");
    PUTS(buffer);
    PUTS("\n");

    MFree(buffer);
}
