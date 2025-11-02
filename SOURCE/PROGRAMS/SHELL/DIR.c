#ifndef __SHELL__
#define __SHELL__
#endif 
#include <PROGRAMS/SHELL/SHELL.h>
#include <STD/FS_DISK.h>
#include <STD/PROC_COM.h>
#include <STD/ASM.h>
#include <STD/MEM.h>
#include <STD/STRING.h>


static U8 tmp_line[CUR_LINE_MAX_LENGTH] ATTRIB_DATA = { 0 };
static U8 tmp_line_out[CUR_LINE_MAX_LENGTH] ATTRIB_DATA = { 0 };
#define MAX_COUNT_DIRS MAX_CHILD_ENTIES
static DIR_ENTRY dirs[MAX_CHILD_ENTIES] ATTRIB_DATA = { 0 };

void INITIALIZE_DIR(SHELL_INSTANCE *shndl) {
    shndl->fat_info.current_cluster = FAT32_GET_ROOT_CLUSTER();
    // Assuming FAT32_GET_ROOT_DIR_ENTRY returns the structure for the root dir
    shndl->fat_info.current_path_dir_entry = FAT32_GET_ROOT_DIR_ENTRY(); 
    STRCPY(shndl->fat_info.path, "/");
    STRCPY(shndl->fat_info.prev_path, "/");
}

PU8 PARSE_CD_RAW_LINE(PU8 line, U32 cut_index) {
    if (!line) return line;

    // Use a temporary copy for str_trim to avoid modifying the static buffer 
    // before the argument shift if 'line' is one of the static buffers.
    // However, since PARSE_CD_RAW_LINE modifies 'line' in place, we will assume 
    // the caller provides a modifiable copy (which CMD_CD/CMD_DIR will do).

    str_trim(line); // remove leading/trailing spaces

    U32 i = cut_index;

    // Skip spaces after command name
    while (line[i] == ' ') i++;

    // Shift the remainder (the argument) to the beginning
    U32 j = 0;
    while (line[i] != '\0') {
        line[j++] = line[i++];
    }
    line[j] = '\0';

    str_trim(line); // clean up any remaining spaces
    return line;
}

STATIC U32 ClassifyPath(PU8 path) {
    if (!path || !*path)
        return 4; // invalid

    if (path[0] == '/') {
        return 0; // absolute path or root "/"
    } 
    else if (path[0] == '.' && path[1] == '\0') {
        return 1; // "."
    } 
    else if (path[0] == '.' && path[1] == '.' && (path[2] == '/' || path[2] == '\0')) {
        return 2; // ".." or "../"
    } 
    else {
        return 1; // relative
    }
}

STATIC VOID NormalizePath(PU8 path) {
    if (!path || path[0] != '/') 
        return; // This function expects an absolute path

    // Array to store pointers to the "clean" path components
    // (This acts as our stack)
    PU8 parts[64]; // Max 64 directory levels
    U32 part_count = 0;

    // Use STRTOK to split the path by '/'. 
    // This will modify the 'path' string in place, which is fine.
    // It also handily skips multiple slashes like "/a//b".
    PU8 token = (PU8)STRTOK((char *)path, "/");
    while (token) {
        if (STRCMP(token, ".") == 0) {
            // Case 1: Token is ".", just ignore it
        } else if (STRCMP(token, "..") == 0) {
            // Case 2: Token is "..", pop one level from our stack
            if (part_count > 0) {
                part_count--;
            }
        } else {
            // Case 3: Normal directory name, push it onto our stack
            if (part_count < 64) { // Avoid stack overflow
                parts[part_count++] = token;
            }
            // Optional: handle "path too deep" error else
        }
        
        token = (PU8)STRTOK(NULL, "/");
    }

    // Now, rebuild the normalized path string in the 'path' buffer
    PU8 p = path;
    *p++ = '/'; // All paths start with root

    if (part_count == 0) {
        // The path was "/", "/..", "/./", etc.
        *p = '\0'; // Just leave the single "/"
        return;
    }

    for (U32 i = 0; i < part_count; i++) {
        STRCPY(p, parts[i]);
        p += STRLEN(parts[i]);

        // Add a separator, but not after the last part
        if (i < part_count - 1) {
            *p++ = '/';
        }
    }

    // Add the final null terminator
    *p = '\0';
}

STATIC VOID StripLeadingDotSlash(PU8 *path_ptr) {
    PU8 path = *path_ptr;
    while (path[0] == '.' && path[1] == '/') {
        path += 2; // skip "./"
    }
    *path_ptr = path;
}

BOOLEAN ResolvePath(PU8 path, PU8 out_buffer, size_t out_buffer_size) {
    if (!path || !*path || !out_buffer || out_buffer_size == 0)
        return FALSE;

    SHELL_INSTANCE *shndl = GET_SHNDL();
    if (!shndl) return FALSE;

    U32 type = ClassifyPath(path);
    if (type > 2) return FALSE;

    // Temporary buffer
    const size_t tmp_max_size = CUR_LINE_MAX_LENGTH;
    tmp_line[0] = '\0';

    switch (type) {
        case 0: // absolute
            STRNCPY(tmp_line, path, tmp_max_size - 1);
            tmp_line[tmp_max_size - 1] = '\0';
            break;

        case 1: // relative
        case 2: // parent
        {
            PU8 rel_path = path;
            if (type == 1) {
                if (STRCMP(rel_path, ".") == 0) {
                    STRNCPY(tmp_line, shndl->fat_info.path, tmp_max_size - 1);
                    tmp_line[tmp_max_size - 1] = '\0';
                    break;
                }
                // strip leading ./ from relative path
                StripLeadingDotSlash(&rel_path);
            }

            // start with current path
            STRNCPY(tmp_line, shndl->fat_info.path, tmp_max_size - 1);
            tmp_line[tmp_max_size - 1] = '\0';

            // add separator if needed
            if (STRCMP(tmp_line, "/") != 0) {
                STRNCAT(tmp_line, "/", tmp_max_size - STRLEN(tmp_line) - 1);
            }

            // append relative component
            STRNCAT(tmp_line, rel_path, tmp_max_size - STRLEN(tmp_line) - 1);
            break;
        }

        default:
            return FALSE;
    }

    // Normalize the path in-place
    NormalizePath(tmp_line);

    // Check buffer size
    size_t final_len = STRLEN(tmp_line);
    if (final_len + 1 > out_buffer_size) return FALSE;

    STRCPY(out_buffer, tmp_line);
    return TRUE;
}



BOOLEAN CD_INTO(PU8 path) {
    if (!path || !*path)
        return FALSE;

    SHELL_INSTANCE *shndl = GET_SHNDL();
    if (!shndl)
        return FALSE;

    // 1. Resolve the path
    // tmp_line_out will contain the CLEAN, ABSOLUTE, NORMALIZED path (e.g., "/home/user")
    if (ResolvePath(path, tmp_line_out, sizeof(tmp_line_out)) == FALSE) {
        PUTS("\ncd: Failed to resolve path.\n");
        return FALSE;
    }

    // 2. Validate the resolved path: must exist and be a directory.
    FAT_LFN_ENTRY entry = { 0 };
    if (FAT32_PATH_RESOLVE_ENTRY(tmp_line_out, &entry) == FALSE) {
        PUTS("\ncd: Path not found.\n");
        return FALSE;
    }
    
    // Check for directory attribute
    if (!(entry.entry.ATTRIB & FAT_ATTRB_DIR)) {
        PUTS("\ncd: Not a directory.\n");
        return FALSE;
    }

    // 3. Update the shell's state
    STRCPY(shndl->fat_info.prev_path, shndl->fat_info.path);
    STRCPY(shndl->fat_info.path, tmp_line_out);

    // 4. Update the current cluster and directory entry
    shndl->fat_info.current_cluster = ((U32)entry.entry.HIGH_CLUSTER_BITS << 16) | entry.entry.LOW_CLUSTER_BITS;
    MEMCPY_OPT(&shndl->fat_info.current_path_dir_entry, &entry, sizeof(DIR_ENTRY));

    // Optional: PUTS(tmp_line_out); // Print new path
    return TRUE;
}


BOOLEAN CD_PREV() {
    SHELL_INSTANCE *shndl = GET_SHNDL();
    return CD_INTO(shndl->fat_info.prev_path);
}

BOOLEAN CD_BACKWARDS_DIR() {
    SHELL_INSTANCE *shndl = GET_SHNDL();
    if (!shndl) return FALSE;

    // If already at root path "/", nothing to go back to.
    if (STRCMP(shndl->fat_info.path, "/") == 0) return TRUE; // already at root

    U32 len = STRLEN(shndl->fat_info.path);
    if (len == 0) return FALSE;

    // allocate temporary buffer for manipulation
    PU8 tmp = MAlloc(len + 2); // +2 for safety (slash + nul)
    if (!tmp) return FALSE;
    MEMZERO(tmp, len + 2);
    STRNCPY(tmp, shndl->fat_info.path, len + 1); // copy exact

    // find last slash
    PU8 slash = STRRCHR(tmp, '/');
    if (!slash) {
        MFree(tmp);
        return FALSE;
    }

    if (slash == tmp) {
        // path like "/something" and last slash is root slash -> result should be root "/"
        tmp[1] = '\0';
    } else {
        // cut off last component
        *slash = '\0';
    }

    // ensure we have at least "/"
    if (STRLEN(tmp) == 0) {
        tmp[0] = '/';
        tmp[1] = '\0';
    }

    BOOLEAN result = CD_INTO(tmp);
    MFree(tmp);
    return result;
}

VOID PRINT_CONTENTS(FAT_LFN_ENTRY *dir) {
    if (!dir) return;

    U32 cluster = ((U32)dir->entry.HIGH_CLUSTER_BITS << 16) | dir->entry.LOW_CLUSTER_BITS;
    U32 count = MAX_COUNT_DIRS;
    U32 res = FAT32_DIR_ENUMERATE(cluster, dirs, &count);
    if (!res) return;

    PUTS("\n");
    PUTS("Type  Size       Name\n");
    PUTS("----  ----------  ------------------------------\n");

    for (U32 i = 0; i < count; i++) {
        DIR_ENTRY *f = &dirs[i];

        // Skip empty or deleted entries
        if (f->FILENAME[0] == 0x00 || f->FILENAME[0] == 0xE5) continue;

        // Type
        if (f->ATTRIB & FAT_ATTRB_DIR)
            PUTS("dir   ");
        else
            PUTS("file  ");

        // Size (fixed width, right-aligned)
        U8 size_buf[12];
        MEMZERO(size_buf, sizeof(size_buf));
        ITOA_U(f->FILE_SIZE, size_buf, 10);

        U32 len = STRLEN(size_buf);
        for (U32 s = 0; s < 10 - len; s++)
            PUTS(" ");

        PUTS(size_buf);
        PUTS("  ");

        // Name
        U8 name_buf[13];
        MEMZERO(name_buf, sizeof(name_buf));
        STRNCPY(name_buf, f->FILENAME, 11);
        name_buf[12] = '\0';
        str_trim(name_buf);

        PUTS(name_buf);
        PUTS("\n");
    }
}

VOID PRINT_CONTENTS_PATH(PU8 path) {
    if (!path || !*path) return;

    if (!ResolvePath(path, tmp_line, sizeof(tmp_line))) {
        PUTS("dir: Failed to resolve path.\n");
        return;
    }

    FAT_LFN_ENTRY ent = { 0 };
    if (!FAT32_PATH_RESOLVE_ENTRY(tmp_line, &ent)) {
        PUTS("dir: Entry not found.\n");
        return;
    }

    // If it's a directory, list contents
    if (ent.entry.ATTRIB & FAT_ATTRB_DIR) {
        PRINT_CONTENTS(&ent);
    } else {
        // It's a file: print just the file info
        PUTS("\nType  Size       Name\n");
        PUTS("----  ----------  ------------------------------\n");

        PUTS("file  ");

        // Size
        U8 size_buf[12];
        MEMZERO(size_buf, sizeof(size_buf));
        ITOA_U(ent.entry.FILE_SIZE, size_buf, 10);

        U32 len = STRLEN(size_buf);
        for (U32 s = 0; s < 10 - len; s++)
            PUTS(" ");

        PUTS(size_buf);
        PUTS("  ");

        // Name
        U8 name_buf[13];
        MEMZERO(name_buf, sizeof(name_buf));
        STRNCPY(name_buf, ent.entry.FILENAME, 11);
        name_buf[12] = '\0';
        str_trim(name_buf);

        PUTS(name_buf);
        PUTS("\n");
    }
}


//==================== MKDIR ====================

BOOLEAN MAKE_DIR(PU8 path) {
    if (!path || !*path) {
        PUTS("\nmkdir: No path specified.\n");
        return FALSE;
    }

    if (!ResolvePath(path, tmp_line, sizeof(tmp_line))) {
        PUTS("\nmkdir: Failed to resolve path.\n");
        return FALSE;
    }

    // Split parent path + new directory name
    PU8 last_slash = STRRCHR(tmp_line, '/');
    if (!last_slash) return FALSE;

    PU8 dir_name = last_slash + 1;
    if (!*dir_name) {
        PUTS("\nmkdir: Invalid directory name.\n");
        return FALSE;
    }

    *last_slash = '\0'; // parent path
    if (!*tmp_line) STRCPY(tmp_line, "/"); // root

    FAT_LFN_ENTRY parent_entry;
    if (!FAT32_PATH_RESOLVE_ENTRY(tmp_line, &parent_entry)) {
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

//==================== RMDIR ====================

BOOLEAN REMOVE_DIR(PU8 path) {
    if (!path || !*path) {
        PUTS("\nrmdir: No path specified.\n");
        return FALSE;
    }

    if (!ResolvePath(path, tmp_line, sizeof(tmp_line))) {
        PUTS("\nrmdir: Failed to resolve path.\n");
        return FALSE;
    }

    FAT_LFN_ENTRY entry;
    if (!FAT32_PATH_RESOLVE_ENTRY(tmp_line, &entry)) {
        PUTS("\nrmdir: Directory not found.\n");
        return FALSE;
    }

    if (!(entry.entry.ATTRIB & FAT_ATTRB_DIR)) {
        PUTS("\nrmdir: Not a directory.\n");
        return FALSE;
    }

    // Root cannot be deleted
    if (STRCMP(tmp_line, "/") == 0) {
        PUTS("\nrmdir: Cannot remove root directory.\n");
        return FALSE;
    }

    // Check directory contents (skip "." and "..")
    U32 cluster = ((U32)entry.entry.HIGH_CLUSTER_BITS << 16) | entry.entry.LOW_CLUSTER_BITS;
    DIR_ENTRY contents[MAX_CHILD_ENTIES];
    U32 count = MAX_CHILD_ENTIES;
    if (!FAT32_DIR_ENUMERATE(cluster, contents, &count)) {
        PUTS("\nrmdir: Failed to read directory.\n");
        return FALSE;
    }

    for (U32 i = 0; i < count; i++) {
        U8 *name = contents[i].FILENAME;
        if (name[0] == 0x00 || name[0] == 0xE5) continue;
        if (STRNCMP(name, ".          ", 11) == 0 || STRNCMP(name, "..         ", 11) == 0) continue;

        PUTS("\nrmdir: Directory is not empty.\n");
        PUTS(name);
        return FALSE;
    }

    // Get parent directory
    PU8 last_slash = STRRCHR(tmp_line, '/');
    if (!last_slash) return FALSE;

    PU8 dir_name = last_slash + 1;
    *last_slash = '\0';
    if (!*tmp_line) STRCPY(tmp_line, "/");

    FAT_LFN_ENTRY parent_entry;
    if (!FAT32_PATH_RESOLVE_ENTRY(tmp_line, &parent_entry)) {
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





PU8 GET_PATH() {
    return GET_SHNDL()->fat_info.path;
}

PU8 GET_PREV_PATH() {
    return GET_SHNDL()->fat_info.prev_path;
}