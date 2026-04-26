#include "AUTOCOMPLETE.h"
#include <STD/FS_DISK.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
#include "BATSH.h"
#include <TSHELL.h>

extern VOID REDRAW_CURRENT_LINE(VOID);
extern VOID PUT_SHELL_START(VOID);
extern VOID TPUT_NEWLINE(VOID);
extern BOOLEAN ResolvePath(PU8 path, PU8 out_buffer, U32 out_buffer_size);

#define AC_MAX_MATCHES 64
#define AC_MAX_NAME    256

/* State for TAB cycling */
static U8 saved_base_input[CUR_LINE_MAX_LENGTH] = {0};
static U8 saved_dir_part[512] = {0};
static BOOLEAN saved_has_slash = FALSE;
static U8 saved_matches[AC_MAX_MATCHES][AC_MAX_NAME] = {0};
static U32 saved_match_count = 0;
static U32 current_match_idx = 0;
static U8 last_completed_line[CUR_LINE_MAX_LENGTH] = {0};

/* Returns pointer into `input` to the last token (after last space), or input if no space */
static PU8 get_partial_token(PU8 input) {
    PU8 last = STRRCHR(input, ' ');
    return last ? last + 1 : input;
}

/* Case-insensitive prefix match */
static BOOL ci_starts_with(PU8 str, PU8 prefix, U32 prefix_len) {
    if (prefix_len == 0) return TRUE;
    if (STRLEN(str) < prefix_len) return FALSE;
    return STRNICMP(str, prefix, prefix_len) == 0;
}

/* Enumerate a FAT32 directory cluster and collect names matching prefix.
   Returns number of matches added. matches must be pre-allocated. */
static U32 collect_matches_in_cluster(U32 cluster, PU8 prefix, U32 prefix_len,
                                       U8 matches[][AC_MAX_NAME], U32 match_count) {
    FAT_LFN_ENTRY entries[128];
    U32 count = 128;
    if (!FAT32_DIR_ENUMERATE_LFN(cluster, entries, &count)) return match_count;

    for (U32 i = 0; i < count && match_count < AC_MAX_MATCHES; i++) {
        PU8 name = (PU8)entries[i].lfn;
        if (STRLEN(name) == 0) {
            /* Fall back to short 8.3 name */
            name = (PU8)entries[i].entry.FILENAME;
        }
        if (STRLEN(name) == 0) continue;
        if (name[0] == '.') continue; /* skip . and .. */
        if (ci_starts_with(name, prefix, prefix_len)) {
            STRNCPY(matches[match_count], name, AC_MAX_NAME - 1);
            match_count++;
        }
    }
    return match_count;
}

/* Resolve a directory path string to its FAT32 cluster. Returns 0 on failure. */
static U32 resolve_dir_cluster(PU8 abs_dir_path) {
    if (!abs_dir_path || !*abs_dir_path) return FAT32_GET_ROOT_CLUSTER();
    if (STRCMP(abs_dir_path, "/") == 0) return FAT32_GET_ROOT_CLUSTER();

    FAT_LFN_ENTRY ent;
    if (!FAT32_PATH_RESOLVE_ENTRY(abs_dir_path, &ent)) return 0;
    if (!FAT32_DIR_ENTRY_IS_DIR(&ent.entry)) return 0;
    return ((U32)ent.entry.HIGH_CLUSTER_BITS << 16) | ent.entry.LOW_CLUSTER_BITS;
}

static VOID handle_tab_internal(BOOLEAN reverse) {
    TSHELL_INSTANCE *shndl = GET_SHNDL();
    if (!shndl) return;
    TERM_OUTPUT *tout = shndl->tout;
    if (!tout) return;
    

    PU8 input = tout->current_line; /* current input buffer */
    if (!input) return;

    /* Check if this is a consecutive TAB press (input exactly matches last completion) */
    if (saved_match_count > 0 && STRCMP(input, last_completed_line) == 0) {
        if (reverse) {
            current_match_idx = (current_match_idx + saved_match_count - 1) % saved_match_count;
        } else {
            current_match_idx = (current_match_idx + 1) % saved_match_count;
        }
        
        U8 completion[512] = {0};
        if (saved_has_slash) {
            STRNCPY(completion, saved_dir_part, sizeof(completion) - 1);
            STRNCAT(completion, saved_matches[current_match_idx], sizeof(completion) - STRLEN(completion) - 1);
        } else {
            STRNCPY(completion, saved_matches[current_match_idx], sizeof(completion) - 1);
        }

        STRNCPY(input, saved_base_input, CUR_LINE_MAX_LENGTH - 1);
        STRNCAT(input, completion, CUR_LINE_MAX_LENGTH - STRLEN(input) - 1);

        tout->edit_pos = STRLEN(input);
        STRNCPY(last_completed_line, input, CUR_LINE_MAX_LENGTH - 1);
        
        REDRAW_CURRENT_LINE();
        return;
    }

    /* Otherwise, start a new completion search */
    saved_match_count = 0;
    current_match_idx = 0;

    PU8 cwd = GET_VAR((PU8)"CD");
    PU8 partial = get_partial_token(input);
    U32 partial_len = STRLEN(partial);
    U32 partial_off = (U32)(partial - input);

    /* Save the base input without the partial token */
    STRNCPY(saved_base_input, input, partial_off);
    saved_base_input[partial_off] = '\0';

    U8 dir_part[512]  = {0};
    U8 file_prefix[AC_MAX_NAME] = {0};

    PU8 last_slash = STRRCHR(partial, '/');
    if (last_slash) {
        saved_has_slash = TRUE;
        U32 dlen = (U32)(last_slash - partial) + 1;
        STRNCPY(dir_part, partial, dlen);
        STRNCPY(saved_dir_part, partial, dlen);
        STRNCPY(file_prefix, last_slash + 1, AC_MAX_NAME - 1);
    } else {
        /* No slash: dir_part = CWD */
        saved_has_slash = FALSE;
        STRNCPY(file_prefix, partial, AC_MAX_NAME - 1);
        STRNCPY(dir_part, cwd, sizeof(dir_part) - 1);
    }

    U32 prefix_len = STRLEN(file_prefix);

    /* Resolve dir_part to absolute */
    U8 abs_dir[512] = {0};
    if (!ResolvePath(dir_part[0] ? dir_part : cwd, abs_dir, sizeof(abs_dir)))
        STRNCPY(abs_dir, cwd, sizeof(abs_dir) - 1);

    U32 match_count = 0;

    /* --- Search in resolved directory --- */
    U32 dir_cluster = resolve_dir_cluster(abs_dir);
    if (dir_cluster)
        match_count = collect_matches_in_cluster(dir_cluster, file_prefix, prefix_len,
                                                  saved_matches, match_count);

    /* --- If no slash in partial (bare command), also search PATH dirs --- */
    if (!last_slash && match_count < AC_MAX_MATCHES) {
        PU8 path_val = GET_VAR((PU8)"PATH");
        if (path_val && *path_val) {
            PU8 path_dup = STRDUP(path_val);
            PU8 saveptr  = NULL;
            PU8 tok = STRTOK_R(path_dup, ";", &saveptr);
            while (tok && match_count < AC_MAX_MATCHES) {
                U8 abs_path_dir[512] = {0};
                if (ResolvePath(tok, abs_path_dir, sizeof(abs_path_dir))) {
                    FAT_LFN_ENTRY ent = {0};
                    FAT32_PATH_RESOLVE_ENTRY(abs_path_dir, &ent.entry);
                    if(!FAT32_DIR_ENTRY_IS_DIR(&ent.entry)) {
                        U32 pc = resolve_dir_cluster(abs_path_dir);
                        if (pc)
                            match_count = collect_matches_in_cluster(pc, file_prefix, prefix_len,
                                                                      saved_matches, match_count);
                    }
                }
                tok = STRTOK_R(NULL, ";", &saveptr);
            }
            MFree(path_dup);
        }
    }

    if (match_count > 0) {
        saved_match_count = match_count;
        current_match_idx = 0;

        U8 completion[512] = {0};
        if (saved_has_slash) {
            STRNCPY(completion, saved_dir_part, sizeof(completion) - 1);
            STRNCAT(completion, saved_matches[0], sizeof(completion) - STRLEN(completion) - 1);
        } else {
            STRNCPY(completion, saved_matches[0], sizeof(completion) - 1);
        }

        /* Overwrite from partial_off onwards */
        input[partial_off] = '\0';
        STRNCAT(input, completion, CUR_LINE_MAX_LENGTH - STRLEN(input) - 1);

        /* Update cursor to end */
        tout->edit_pos = STRLEN(input);
        STRNCPY(last_completed_line, input, CUR_LINE_MAX_LENGTH - 1);
        
        REDRAW_CURRENT_LINE();
    }
}

VOID HANDLE_LE_TAB(VOID) {
    handle_tab_internal(FALSE);
}

VOID HANDLE_LE_SHIFT_TAB(VOID) {
    handle_tab_internal(TRUE);
}
