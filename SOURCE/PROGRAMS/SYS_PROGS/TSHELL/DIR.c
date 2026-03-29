/*
 * DIR.c — Directory management for TSHELL
 * Ported from PROGRAMS/SHELL/DIR.c with output via TPUT_* API
 */

#include "TSHELL.h"
#include <STD/FS_DISK.h>
#include <STD/PROC_COM.h>
#include <STD/MEM.h>
#include <STD/STRING.h>

static U8 tmp_dir_line[CUR_LINE_MAX_LENGTH] ATTRIB_DATA = { 0 };
static U8 tmp_dir_out[CUR_LINE_MAX_LENGTH] ATTRIB_DATA = { 0 };
#define MAX_COUNT_DIRS MAX_CHILD_ENTIES
static DIR_ENTRY dirs[MAX_CHILD_ENTIES] ATTRIB_DATA = { 0 };

/* ================================================================== */

VOID INITIALIZE_DIR(TSHELL_INSTANCE *sh) {
    sh->fat_info.current_cluster = FAT32_GET_ROOT_CLUSTER();
    sh->fat_info.current_path_dir_entry = FAT32_GET_ROOT_DIR_ENTRY();
    STRCPY(sh->fat_info.path, "/");
    STRCPY(sh->fat_info.prev_path, "/");
}

PU8 PARSE_CD_RAW_LINE(PU8 line, U32 cut_index) {
    if (!line) return line;
    str_trim(line);
    U32 i = cut_index;
    while (line[i] == ' ') i++;
    U32 j = 0;
    while (line[i] != '\0') line[j++] = line[i++];
    line[j] = '\0';
    str_trim(line);
    return line;
}

/* ---- Path classification ---- */
static U32 ClassifyPath(PU8 path) {
    if (!path || !*path) return 4;
    if (path[0] == '/') return 0;
    if (path[0] == '.' && path[1] == '\0') return 1;
    if (path[0] == '.' && path[1] == '.' && (path[2] == '/' || path[2] == '\0')) return 2;
    return 1;
}

static VOID NormalizePath(PU8 path) {
    if (!path || path[0] != '/') return;
    PU8 parts[64];
    U32 part_count = 0;
    PU8 token = (PU8)STRTOK((char *)path, "/");
    while (token) {
        if (STRCMP(token, ".") == 0) { /* skip */ }
        else if (STRCMP(token, "..") == 0) {
            if (part_count > 0) part_count--;
        } else {
            if (part_count < 64) parts[part_count++] = token;
        }
        token = (PU8)STRTOK(NULL, "/");
    }
    PU8 p = path;
    *p++ = '/';
    if (part_count == 0) { *p = '\0'; return; }
    for (U32 i = 0; i < part_count; i++) {
        STRCPY(p, parts[i]);
        p += STRLEN(parts[i]);
        if (i < part_count - 1) *p++ = '/';
    }
    *p = '\0';
}

static VOID StripLeadingDotSlash(PU8 *path_ptr) {
    PU8 path = *path_ptr;
    while (path[0] == '.' && path[1] == '/') path += 2;
    *path_ptr = path;
}

BOOLEAN ResolvePath(PU8 path, PU8 out_buffer, size_t out_buffer_size) {
    if (!path || !*path || !out_buffer || out_buffer_size == 0) return FALSE;
    TSHELL_INSTANCE *sh = GET_SHNDL();
    if (!sh) return FALSE;
    U32 type = ClassifyPath(path);
    if (type > 2) return FALSE;
    const size_t tmp_max = CUR_LINE_MAX_LENGTH;
    tmp_dir_line[0] = '\0';

    switch (type) {
    case 0:
        STRNCPY(tmp_dir_line, path, tmp_max - 1);
        tmp_dir_line[tmp_max - 1] = '\0';
        break;
    case 1:
    case 2: {
        PU8 rel = path;
        if (type == 1) {
            if (STRCMP(rel, ".") == 0) {
                STRNCPY(tmp_dir_line, sh->fat_info.path, tmp_max - 1);
                tmp_dir_line[tmp_max - 1] = '\0';
                break;
            }
            StripLeadingDotSlash(&rel);
        }
        STRNCPY(tmp_dir_line, sh->fat_info.path, tmp_max - 1);
        tmp_dir_line[tmp_max - 1] = '\0';
        if (STRCMP(tmp_dir_line, "/") != 0)
            STRNCAT(tmp_dir_line, "/", tmp_max - STRLEN(tmp_dir_line) - 1);
        STRNCAT(tmp_dir_line, rel, tmp_max - STRLEN(tmp_dir_line) - 1);
        break;
    }
    default: return FALSE;
    }

    NormalizePath(tmp_dir_line);
    size_t final_len = STRLEN(tmp_dir_line);
    if (final_len + 1 > out_buffer_size) return FALSE;
    STRCPY(out_buffer, tmp_dir_line);
    return TRUE;
}

/* ================================================================== */

BOOLEAN CD_INTO(PU8 path) {
    if (!path || !*path) return FALSE;
    TSHELL_INSTANCE *sh = GET_SHNDL();
    if (!sh) return FALSE;

    if (ResolvePath(path, tmp_dir_out, sizeof(tmp_dir_out)) == FALSE) {
        PUTS("\ncd: Failed to resolve path.\n");
        return FALSE;
    }

    FAT_LFN_ENTRY entry = { 0 };
    if (FAT32_PATH_RESOLVE_ENTRY(tmp_dir_out, &entry) == FALSE) {
        PUTS("\ncd: Path not found.\n");
        return FALSE;
    }
    if (!(entry.entry.ATTRIB & FAT_ATTRB_DIR)) {
        PUTS("\ncd: Not a directory.\n");
        return FALSE;
    }

    STRCPY(sh->fat_info.prev_path, sh->fat_info.path);
    STRCPY(sh->fat_info.path, tmp_dir_out);
    sh->fat_info.current_cluster = ((U32)entry.entry.HIGH_CLUSTER_BITS << 16) | entry.entry.LOW_CLUSTER_BITS;
    MEMCPY_OPT(&sh->fat_info.current_path_dir_entry, &entry, sizeof(DIR_ENTRY));
    return TRUE;
}

BOOLEAN CD_PREV(VOID) {
    TSHELL_INSTANCE *sh = GET_SHNDL();
    return CD_INTO(sh->fat_info.prev_path);
}

BOOLEAN CD_BACKWARDS_DIR(VOID) {
    TSHELL_INSTANCE *sh = GET_SHNDL();
    if (!sh) return FALSE;
    if (STRCMP(sh->fat_info.path, "/") == 0) return TRUE;

    U32 len = STRLEN(sh->fat_info.path);
    if (len == 0) return FALSE;

    PU8 tmp = MAlloc(len + 2);
    if (!tmp) return FALSE;
    MEMZERO(tmp, len + 2);
    STRNCPY(tmp, sh->fat_info.path, len + 1);

    PU8 slash = STRRCHR(tmp, '/');
    if (!slash) { MFree(tmp); return FALSE; }

    if (slash == tmp) tmp[1] = '\0';
    else *slash = '\0';

    if (STRLEN(tmp) == 0) { tmp[0] = '/'; tmp[1] = '\0'; }

    BOOLEAN result = CD_INTO(tmp);
    MFree(tmp);
    return result;
}

/* ================================================================== */

VOID PRINT_CONTENTS(FAT_LFN_ENTRY *dir) {
    if (!dir) return;

    U32 cluster = ((U32)dir->entry.HIGH_CLUSTER_BITS << 16) | dir->entry.LOW_CLUSTER_BITS;
    U32 count = MAX_COUNT_DIRS;
    U32 res = FAT32_DIR_ENUMERATE(cluster, dirs, &count);
    if (!res) return;

    TPUT_BEGIN();

    PUTS("\n");
    PUTS("Type  Size       Date       Time     Name\n");
    PUTS("----  ----------  ----------  -------  ------------------------------\n");

    for (U32 i = 0; i < count; i++) {
        DIR_ENTRY *f = &dirs[i];
        if (f->FILENAME[0] == 0x00 || f->FILENAME[0] == 0xE5 ||
            (f->ATTRIB & FAT_ATTRIB_VOL_ID) || (f->ATTRIB == FAT_ATTRIB_LFN)) continue;

        if (f->ATTRIB & FAT_ATTRB_DIR) {
            /* Highlight directories with ANSI blue */
            PUTS("\x1B[34mdir   \x1B[0m");
        } else {
            PUTS("file  ");
        }

        /* Size */
        U8 size_buf[12];
        MEMZERO(size_buf, sizeof(size_buf));
        ITOA_U(f->FILE_SIZE, size_buf, 10);
        U32 len = STRLEN(size_buf);
        for (U32 s = 0; s < 10 - len; s++) PUTS(" ");
        PUTS(size_buf);
        PUTS("  ");

        /* Date */
        U32 year, month, day, hour, minute, second;
        FAT_DECODE_TIME(f->LAST_MOD_TIME, f->LAST_MOD_DATE, &year, &month, &day, &hour, &minute, &second);
        U8 date_buf[16];
        SPRINTF(date_buf, "%04u-%02u-%02u", year, month, day);
        PUTS(date_buf);
        PUTS("  ");

        /* Time */
        U8 time_buf[12];
        SPRINTF(time_buf, "%02u:%02u", hour, minute);
        PUTS(time_buf);
        PUTS("  ");

        /* Name (8.3) */
        U8 name_buf[14], base_name[9], ext_name[4];
        MEMCPY(base_name, f->FILENAME, 8);
        base_name[8] = '\0';
        str_trim(base_name);
        MEMCPY(ext_name, f->FILENAME + 8, 3);
        ext_name[3] = '\0';
        str_trim(ext_name);
        MEMZERO(name_buf, sizeof(name_buf));
        STRCPY(name_buf, base_name);
        if (STRLEN(ext_name) > 0) {
            STRCAT(name_buf, ".");
            STRCAT(name_buf, ext_name);
        }
        if (f->ATTRIB & FAT_ATTRB_DIR) {
            PUTS("\x1B[34m");
            PUTS(name_buf);
            PUTS("\x1B[0m");
        } else {
            PUTS(name_buf);
        }
        PUTS("\n");
    }

    TPUT_END();
}

VOID PRINT_CONTENTS_PATH(PU8 path) {
    if (!path || !*path) return;

    if (!ResolvePath(path, tmp_dir_line, sizeof(tmp_dir_line))) {
        PUTS("dir: Failed to resolve path.\n");
        return;
    }

    FAT_LFN_ENTRY ent = { 0 };
    if (!FAT32_PATH_RESOLVE_ENTRY(tmp_dir_line, &ent)) {
        PUTS("dir: Entry not found.\n");
        return;
    }

    if (ent.entry.ATTRIB & FAT_ATTRB_DIR) {
        PRINT_CONTENTS(&ent);
    } else {
        TPUT_BEGIN();
        PUTS("\nType  Size       Date       Time     Name\n");
        PUTS("----  ----------  ----------  -------  ------------------------------\n");
        PUTS("file  ");

        U8 size_buf[12];
        MEMZERO(size_buf, sizeof(size_buf));
        ITOA_U(ent.entry.FILE_SIZE, size_buf, 10);
        U32 len = STRLEN(size_buf);
        for (U32 s = 0; s < 10 - len; s++) PUTS(" ");
        PUTS(size_buf);
        PUTS("  ");

        U32 year, month, day, hour, minute, second;
        FAT_DECODE_TIME(ent.entry.LAST_MOD_TIME, ent.entry.LAST_MOD_DATE, &year, &month, &day, &hour, &minute, &second);
        U8 date_buf[16];
        SPRINTF(date_buf, "%04u-%02u-%02u", year, month, day);
        PUTS(date_buf);
        PUTS("  ");

        U8 time_buf[12];
        SPRINTF(time_buf, "%02u:%02u", hour, minute);
        PUTS(time_buf);
        PUTS("  ");

        U8 name_buf[14], base_name[9], ext_name[4];
        MEMCPY(base_name, ent.entry.FILENAME, 8);
        base_name[8] = '\0';
        str_trim(base_name);
        MEMCPY(ext_name, ent.entry.FILENAME + 8, 3);
        ext_name[3] = '\0';
        str_trim(ext_name);
        MEMZERO(name_buf, sizeof(name_buf));
        STRCPY(name_buf, base_name);
        if (STRLEN(ext_name) > 0) {
            STRCAT(name_buf, ".");
            STRCAT(name_buf, ext_name);
        }
        PUTS(name_buf);
        PUTS("\n");
        TPUT_END();
    }
}

/* ================================================================== */
/*  MKDIR / RMDIR                                                      */
/* ================================================================== */

BOOLEAN MAKE_DIR(PU8 path) {
    if (!path || !*path) {
        PUTS("\nmkdir: No path specified.\n");
        return FALSE;
    }
    if (!ResolvePath(path, tmp_dir_line, sizeof(tmp_dir_line))) {
        PUTS("\nmkdir: Failed to resolve path.\n");
        return FALSE;
    }

    PU8 last_slash = STRRCHR(tmp_dir_line, '/');
    if (!last_slash) return FALSE;
    PU8 dir_name = last_slash + 1;
    if (!*dir_name) { PUTS("\nmkdir: Invalid directory name.\n"); return FALSE; }
    *last_slash = '\0';
    if (!*tmp_dir_line) STRCPY(tmp_dir_line, "/");

    FAT_LFN_ENTRY parent_entry;
    if (!FAT32_PATH_RESOLVE_ENTRY(tmp_dir_line, &parent_entry)) {
        PUTS("\nmkdir: Parent directory not found.\n");
        return FALSE;
    }
    if (!(parent_entry.entry.ATTRIB & FAT_ATTRB_DIR)) {
        PUTS("\nmkdir: Parent is not a directory.\n");
        return FALSE;
    }

    U32 parent_cluster = ((U32)parent_entry.entry.HIGH_CLUSTER_BITS << 16) | parent_entry.entry.LOW_CLUSTER_BITS;
    if (FAT32_FIND_DIR_BY_NAME_AND_PARENT(parent_cluster, dir_name) != 0) {
        PUTS("\nmkdir: Directory already exists.\n");
        return FALSE;
    }

    U32 new_cluster;
    if (!FAT32_CREATE_CHILD_DIR(parent_cluster, dir_name, FAT_ATTRB_DIR, &new_cluster)) {
        PUTS("\nmkdir: Failed to create directory.\n");
        return FALSE;
    }
    return TRUE;
}

BOOLEAN REMOVE_DIR(PU8 path) {
    if (!path || !*path) {
        PUTS("\nrmdir: No path specified.\n");
        return FALSE;
    }
    if (!ResolvePath(path, tmp_dir_line, sizeof(tmp_dir_line))) {
        PUTS("\nrmdir: Failed to resolve path.\n");
        return FALSE;
    }

    FAT_LFN_ENTRY entry;
    if (!FAT32_PATH_RESOLVE_ENTRY(tmp_dir_line, &entry)) {
        PUTS("\nrmdir: Directory not found.\n");
        return FALSE;
    }
    if (!(entry.entry.ATTRIB & FAT_ATTRB_DIR)) {
        PUTS("\nrmdir: Not a directory.\n");
        return FALSE;
    }
    if (STRCMP(tmp_dir_line, "/") == 0) {
        PUTS("\nrmdir: Cannot remove root directory.\n");
        return FALSE;
    }

    U32 cluster = ((U32)entry.entry.HIGH_CLUSTER_BITS << 16) | entry.entry.LOW_CLUSTER_BITS;
    DIR_ENTRY contents[MAX_CHILD_ENTIES];
    U32 cnt = MAX_CHILD_ENTIES;
    if (!FAT32_DIR_ENUMERATE(cluster, contents, &cnt)) {
        PUTS("\nrmdir: Failed to read directory.\n");
        return FALSE;
    }
    for (U32 i = 0; i < cnt; i++) {
        U8 *name = contents[i].FILENAME;
        if (name[0] == 0x00 || name[0] == 0xE5) continue;
        if (STRNCMP(name, ".          ", 11) == 0 || STRNCMP(name, "..         ", 11) == 0) continue;
        PUTS("\nrmdir: Directory is not empty.\n");
        PUTS(name);
        return FALSE;
    }

    PU8 last_slash = STRRCHR(tmp_dir_line, '/');
    if (!last_slash) return FALSE;
    PU8 dir_name = last_slash + 1;
    *last_slash = '\0';
    if (!*tmp_dir_line) STRCPY(tmp_dir_line, "/");

    FAT_LFN_ENTRY parent_entry;
    if (!FAT32_PATH_RESOLVE_ENTRY(tmp_dir_line, &parent_entry)) {
        PUTS("\nrmdir: Parent directory not found.\n");
        return FALSE;
    }

    U32 parent_cluster = ((U32)parent_entry.entry.HIGH_CLUSTER_BITS << 16) | parent_entry.entry.LOW_CLUSTER_BITS;
    DIR_ENTRY dir_entry;
    if (!FAT32_FIND_DIR_ENTRY_BY_NAME_AND_PARENT(&dir_entry, parent_cluster, dir_name)) {
        PUTS("\nrmdir: Directory entry not found.\n");
        return FALSE;
    }
    if (!FAT32_DIR_REMOVE_ENTRY(&dir_entry, dir_name)) {
        PUTS("\nrmdir: Failed to remove directory.\n");
        return FALSE;
    }
    return TRUE;
}

/* ================================================================== */
/*  Path accessors                                                      */
/* ================================================================== */

PU8 GET_PATH(VOID) {
    return GET_SHNDL()->fat_info.path;
}

PU8 GET_PREV_PATH(VOID) {
    return GET_SHNDL()->fat_info.prev_path;
}
