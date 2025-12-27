/**
 * Define ATRC_IMPLEMENTATION in atleast one source file to use
 * Define ATRC_DEBUG to enable debug prints into log file.
 */
#ifndef ATRC_H
#define ATRC_H
#include <STD/TYPEDEF.h>

typedef enum _READMODES {
    // use if exists
    ATRC_READ_WRITE,

    // create if not exist
    ATRC_CREATE_READ_WRITE,

    // delete and create new file
    ATRC_FORCE_READ_WRITE,
} READMODES;

#ifndef MAX_KEY_VALUE_PAIRS
#   define MAX_KEY_VALUE_PAIRS 32
#endif

#ifndef MAX_BLOCKS
#   define MAX_BLOCKS 8
#endif

typedef struct _KEY_VALUE {
    PU8 name;
    PU8 value;
    U32 enum_val;
} KEY_VALUE, *PKEY_VALUE;
typedef struct _KEY_VALUE_ARR {
    PKEY_VALUE key_value_pairs[MAX_KEY_VALUE_PAIRS];
    U32 len;
} KEY_VALUE_ARR, *PKEY_VALUE_ARR;

typedef struct _BLOCK {
    PU8 name;
    KEY_VALUE_ARR keys;
} BLOCK, *PBLOCK;
typedef struct _BLOCK_ARR {
    PBLOCK blocks[MAX_BLOCKS];
    U32 len;
} BLOCK_ARR, *PBLOCK_ARR;

typedef struct _ATRCFD {
    BLOCK_ARR blocks;
    KEY_VALUE_ARR vars;
    PU8 filepath;
    READMODES mode;
    BOOL autosave;
    BOOL writecheck; // this creates a new key, block or variable if one doesn't exist while modifying or creating
} ATRCFD, *PATRC_FD;

// === Public API ===
//  No need to free anything unless told otherwise

// file handling

// Create a new instance of ATRCFD and parse the file
PATRC_FD CREATE_ATRCFD(PU8 filepath, READMODES mode); // Free with DESTROY_ATRCFD()
BOOL READ_ATRCFD(PATRC_FD fd, PU8 filepath, READMODES mode); // Free with DESTROY_ATRCFD()

VOID DESTROY_ATRCFD(PATRC_FD fd);
VOID SET_WRITECHECK(PATRC_FD fd, BOOL writecheck);
VOID SET_AUTOSAVE(PATRC_FD fd, BOOL autosave);
PATRC_FD COPY_ATRCFD(PATRC_FD src);

// read, write, create and destroy keys, blocks or variables
PU8 READ_VARIABLE(PATRC_FD fd, PU8 variable_name);
PU8 MODIFY_VARIABLE(PATRC_FD fd, PU8 variable_name, PU8 value);
PU8 CREATE_VARIABLE(PATRC_FD fd, PU8 variable_name, PU8 value);
PU8 DELETE_VARIABLE(PATRC_FD fd, PU8 variable_name);

PU8 CREATE_BLOCK(PATRC_FD fd, PU8 block_name);
PU8 DELETE_BLOCK(PATRC_FD fd, PU8 block_name);

PU8 READ_KEY(PATRC_FD fd, PU8 block_name, PU8 key_name);
PU8 MODIFY_KEY(PATRC_FD fd, PU8 block_name, PU8 key_name, PU8 value);
PU8 CREATE_KEY(PATRC_FD fd, PU8 block_name, PU8 key_name, PU8 value);
PU8 DELETE_KEY(PATRC_FD fd, PU8 block_name, PU8 key_name);

// boolean checks
BOOL DOES_EXIST_BLOCK(PATRC_FD fd, PU8 block_name);
BOOL DOES_EXIST_KEY(PATRC_FD fd, PU8 block_name, PU8 key_name);
BOOL DOES_EXIST_VARIABLE(PATRC_FD fd, PU8 variable_name);

#define READ_NKEY(fd, key_name) READ_KEY(fd, " ", key_name)
#define MODIFY_NKEY(fd, key_name, value) MODIFY_KEY(fd, " ", key_name, value)
#define CREATE_NKEY(fd, key_name) CREATE_KEY(fd, " ", key_name)
#define DELETE_NKEY(fd, key_name) DELETE_KEY(fd, " ", key_name)
#define DOES_EXIST_NKEY(fd, key_name) DOES_EXIST_KEY(fd, " ", key_name)

// misc
PU8 INSERT_VAR(PU8 line, PPU8 args, U32 arg_count); // Return must be freed
U32 GET_ENUM_VALUE(PATRC_FD fd, PU8 block_name, PU8 key_name);
// BOOL WRITE_COMMENT_TO_TOP(PATRC_FD fd);
// BOOL WRITE_COMMENT_TO_BOTTOM(PATRC_FD fd);

// functions to ease life
#define MAX_ATRC_STRS 32
typedef struct _STR_ARR {
    PU8 strs[MAX_ATRC_STRS];
    U32 len;
} STR_ARR, *PSTR_ARR;

PSTR_ARR ATRC_TO_LIST(PU8 val, CHAR separator); // Free with FREE_STR_ARR()
VOID FREE_STR_ARR(PSTR_ARR arr);
BOOL ATRC_TO_BOOL(PU8 val); // Accepts: TRUE, 1, ON, FALSE, 0, 0FF. case-insensitive.

#ifdef ATRC_IMPLEMENTATION
#include <STD/MEM.h>
#include <STD/STRING.h>
#include <STD/FS_DISK.h>
#ifdef ATRC_DEBUG
#include <STD/DEBUG.h>
#define DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif
/* --------- Helper: find key_value by name ---------- */
PKEY_VALUE GET_KEY_VALUE(PKEY_VALUE_ARR arr, PU8 name) {
    if(!arr || !name) return NULLPTR;
    for(U32 i = 0; i < arr->len; i++) {
        if(arr->key_value_pairs[i] && arr->key_value_pairs[i]->name &&
           STRCMP(arr->key_value_pairs[i]->name, name) == 0) {
            return arr->key_value_pairs[i];
        }
    }
    return NULLPTR;
}

/* ---------------- PARSE_VALUE -----------------
   - handles escapes with backslash
   - stops at unescaped '#' (comment)
   - replaces %var% by looking up vars array
   - returns a newly allocated string (caller must free)
*/
PU8 PARSE_VALUE(PU8 val, PKEY_VALUE_ARR vars) {
    if(!val) return NULLPTR;

    U32 len = STRLEN(val);
    PU8 out = MAlloc(len + 1 + 128); // some slack
    if(!out) return NULLPTR;
    U32 out_pos = 0;
    BOOL esc = FALSE;

    for(U32 i = 0; i < len; i++) {
        CHAR c = val[i];
        if(esc) {
            out[out_pos++] = c;
            esc = FALSE;
            continue;
        }
        if(c == '\\') {
            esc = TRUE;
            continue;
        }
        if(c == '#') { // comment start
            break;
        }
        if(c == '%') { // variable substitution %name%
            // find closing %
            U32 j = i + 1;
            while(j < len && val[j] != '%') j++;
            if(j >= len) {
                MFree(out);
                return NULLPTR; // unterminated %
            }
            // copy name into temp
            U32 nlen = j - (i + 1);
            PU8 name = MAlloc(nlen + 1);
            if(!name) {
                MFree(out);
                return NULLPTR;
            }

            for(U32 k = 0; k < nlen; k++) name[k] = val[i + 1 + k];
            name[nlen] = '\0';
            
            if(name[0] == '*') {
                PU8 tmp = ReAlloc(out, out_pos + nlen + 3);
                if(!tmp) {
                    MFree(out);
                    MFree(name);
                    return NULLPTR;
                }
                out = tmp;
                out[out_pos++] = '%';
                for(U32 k = 0; k < nlen; k++) {
                    out[out_pos++] = name[k];
                }
                out[out_pos++] = '%';
                MFree(name);
                i = j; // jump past closing %
                continue; // %*[i]%, add and skip
            }
            
            PKEY_VALUE kv = GET_KEY_VALUE(vars, name);
            MFree(name);
            if(!kv || !kv->value) {
                MFree(out);
                return NULLPTR; // missing variable -> parse fail
            }
            // append variable value
            U32 vlen = STRLEN(kv->value);
            // ensure capacity (naive)

            PU8 tmp = ReAlloc(out, out_pos + vlen + 256);
            if(!tmp) {
                MFree(out);
                MFree(name);
                return NULLPTR;
            }
            out = tmp;

            for(U32 k = 0; k < vlen; k++) out[out_pos++] = kv->value[k];
            i = j; // jump past closing %
            continue;
        }
        // normal char
        out[out_pos++] = c;
    }

    out[out_pos] = '\0';
    str_trim(out);
    DEBUG_PRINTF("Parsed value: '%s'\n", out);
    return out;
}

/* ---------------- ADD_KEY_VALUE -----------------
   Adds or updates a key/value in given PKEY_VALUE_ARR.
   Returns TRUE on success.
*/
BOOL ADD_KEY_VALUE(PKEY_VALUE_ARR arr, PU8 name, PU8 value) {
    if(!arr || !name || !value) return FALSE;
    if(arr->len >= MAX_KEY_VALUE_PAIRS) {
        // Check if name exists to allow overwrite
        for(U32 i=0;i<arr->len;i++){
            if(arr->key_value_pairs[i] && STRCMP(arr->key_value_pairs[i]->name, name) == 0) {
                // replace value
                if(arr->key_value_pairs[i]->value) MFree(arr->key_value_pairs[i]->value);
                arr->key_value_pairs[i]->value = STRDUP(value);
                return arr->key_value_pairs[i]->value != NULLPTR;
            }
        }
        return FALSE;
    }

    // search duplicate
    for(U32 i=0;i<arr->len;i++){
        if(arr->key_value_pairs[i] && STRCMP(arr->key_value_pairs[i]->name, name) == 0) {
            // update
            if(arr->key_value_pairs[i]->value) MFree(arr->key_value_pairs[i]->value);
            arr->key_value_pairs[i]->value = STRDUP(value);
            return arr->key_value_pairs[i]->value != NULLPTR;
        }
    }

    // create new
    PKEY_VALUE kv = MAlloc(sizeof(KEY_VALUE));
    if(!kv) return FALSE;
    kv->name = STRDUP(name);
    if(!kv->name) { MFree(kv); return FALSE; }

    kv->value = STRDUP(value);
    if(!kv->value) { MFree(kv->name); MFree(kv); return FALSE; }

    kv->enum_val = arr->len;
    arr->key_value_pairs[arr->len] = kv;
    arr->len++;
    return TRUE;
}

/* ---------------- ADD_BLOCK -----------------
   Safely adds a block by name. Returns TRUE on success.
*/
BOOL ADD_BLOCK(PBLOCK_ARR arr, PU8 name) {
    if(!arr) return FALSE;
    if(arr->len >= MAX_BLOCKS) {
        // avoid duplicates but not create new
        for(U32 i=0;i<arr->len;i++){
            if(arr->blocks[i] && arr->blocks[i]->name && STRCMP(arr->blocks[i]->name, name)==0)
                return TRUE; // already exists, success
        }
        return FALSE;
    }
    // check duplicates
    for(U32 i = 0; i <arr->len; i++) {
        if(arr->blocks[i] && arr->blocks[i]->name && STRCMP(arr->blocks[i]->name, name) == 0) {
            return TRUE;
        }
    }

    PBLOCK b = MAlloc(sizeof(BLOCK));
    if(!b) return FALSE;
    b->keys.len = 0;
    if(name) {
        b->name = STRDUP(name);
        if(!b->name) {
            MFree(b);
            return FALSE;
        }
    } else {
        b->name = NULLPTR;
    }
    arr->blocks[arr->len] = b;
    arr->len++;
    arr->blocks[arr->len - 1]->keys.len = 0;
    
    return TRUE;
}

/* ---------- Preprocessor parsing helpers ---------- */
#define PREPROC_ERROR       0x0000
#define PREPROC_INCLUDE     0x0001
#define PREPROC_RAW_STR_KEY 0x0002
#define PREPROC_RAW_STR_VAR 0x0004
#define PREPROC_RAW_STR_END 0x0008
typedef struct _PREPROC_DATA {
    U32 type;
    union {
        PU8 include_file;
        PU8 tmp;
    };
} PREPROC_DATA;

U32 PARSE_PREPROCESSOR_TAG(PU8 line, U32 line_len, PREPROC_DATA *data_out) {
    data_out->type = PREPROC_ERROR;
    if(!line || line_len == 0) return data_out->type;
    PU8 copy = STRDUP(line);
    if(!copy) return PREPROC_ERROR;
    str_trim(copy);
    if(STRICMP(copy, "#.ER") == 0) {
        data_out->type = PREPROC_RAW_STR_END;
        MFree(copy);
        return data_out->type;
    }
    PU8 space = STRCHR(copy, ' ');
    if(!space) {
        MFree(copy);
        return PREPROC_ERROR;
    }
    *space = '\0';
    if(STRICMP(copy, "#.INCLUDE") == 0) {
        data_out->type = PREPROC_INCLUDE;
        data_out->include_file = STRDUP(space + 1);
    } else if(STRICMP(copy, "#.SR") == 0) {
        if(STRICMP(space + 1, "KEY") == 0) data_out->type = PREPROC_RAW_STR_KEY;
        else if(STRICMP(space + 1, "VAR") == 0) data_out->type = PREPROC_RAW_STR_VAR;
        else data_out->type = PREPROC_ERROR;
    } else {
        data_out->type = PREPROC_ERROR;
    }
    MFree(copy);
    return data_out->type;
}

/* ---------------- PARSE_ATRC -----------------
   Parses the file pointed by fd->filepath into fd->vars and fd->blocks.
*/
BOOL PARSE_ATRC(PATRC_FD fd)
{
    if (!fd || !fd->filepath)
        return FALSE;

    PBLOCK_ARR blocks = &fd->blocks;
    PKEY_VALUE_ARR vars = &fd->vars;
    BOOL result = FALSE;
    FILE *file = NULL;

    blocks->len = 0;
    vars->len = 0;

    /* Create blockless block " " */
    DEBUG_PRINTF("Creating default block\n");
    if (!ADD_BLOCK(blocks, " "))
        return FALSE;

    PBLOCK current_block = blocks->blocks[0];

    U8 line[1024];
    DEBUG_PRINTF("Opening ATRC file: %s\n", fd->filepath);
    file = FOPEN(fd->filepath, MODE_FR);
    if (!file)
        goto fail;

    BOOL in_raw_string = FALSE;
    BOOL raw_into_var = FALSE;            // true => write into vars, false => write into current block keys
    U8 raw_key_name[256] = {0};
    U8 raw_buffer[4096];
    U32 raw_len = 0;
    BOOL have_raw_target = FALSE;         // whether we currently have a %name% target to append into

    U32 ln = 0;
    DEBUG_PRINTF("Starting to parse ATRC file: %s\n", fd->filepath);
    while (FILE_GET_LINE(file, line, sizeof(line)))
    {
        ln++;
        str_trim(line);
        U32 len = STRLEN(line);
        DEBUG_PRINTF("ATRC: Line %d: '%s'\n", ln, line);
        /* First line must be #!ATRC */
        if (ln == 1) {
            if (STRICMP(line, "#!ATRC") != 0)
                goto fail;
            continue;
        }

        if (len == 0) {
            /* If inside raw mode, preserve empty lines as single newline */
            if (in_raw_string && have_raw_target) {
                if (raw_len + 1 >= sizeof(raw_buffer)) goto fail;
                raw_buffer[raw_len++] = '\n';
                raw_buffer[raw_len] = '\0';
            }
            continue;
        }

        /* ============================
           PREPROCESSOR / COMMENTS
        ============================ */
        if (line[0] == '#') {
            if (line[1] == '.') {
                PREPROC_DATA dt = {0};
                U32 ptype = PARSE_PREPROCESSOR_TAG(line, len, &dt);

                if (ptype == PREPROC_INCLUDE) {
                    /* Recursively parse include */
                    PU8 oldpath = fd->filepath;
                    fd->filepath = dt.include_file;
                    BOOL ok = PARSE_ATRC(fd);
                    fd->filepath = oldpath;
                    MFree(dt.include_file);
                    if (!ok) goto fail;
                    continue;
                }
                else if (ptype == PREPROC_RAW_STR_KEY || ptype == PREPROC_RAW_STR_VAR) {
                    /* Begin raw string mode */
                    in_raw_string = TRUE;
                    raw_len = 0;
                    raw_buffer[0] = '\0';
                    have_raw_target = FALSE;
                    raw_into_var = (ptype == PREPROC_RAW_STR_VAR);
                    /* dt does not carry a name in your parser; the first %name%=value inside block provides it. */
                    continue;
                }
                else if (ptype == PREPROC_RAW_STR_END) {
                    if (!in_raw_string) goto fail;

                    /* Finalize current raw target (if any) */
                    if (have_raw_target) {
                        raw_buffer[raw_len] = '\0';
                        if (raw_into_var) {
                            if (!ADD_KEY_VALUE(vars, raw_key_name, raw_buffer)) goto fail;
                        } else {
                            if (!ADD_KEY_VALUE(&current_block->keys, raw_key_name, raw_buffer)) goto fail;
                        }
                        have_raw_target = FALSE;
                    }

                    in_raw_string = FALSE;
                    continue;
                }

                /* other preprocessor tags ignored */
                continue;
            }

            /* Normal comment -> ignore */
            continue;
        }

        /* ============================
           RAW STRING MODE
        ============================ */
        if (in_raw_string) {
            /* If line starts with '%' treat it as %name%=value and switch target */
            if (line[0] == '%') {
                /* If we already had a target, finalize it first */
                if (have_raw_target) {
                    raw_buffer[raw_len] = '\0';
                    if (raw_into_var) {
                        if (!ADD_KEY_VALUE(vars, raw_key_name, raw_buffer)) goto fail;
                    } else {
                        if (!ADD_KEY_VALUE(&current_block->keys, raw_key_name, raw_buffer)) goto fail;
                    }
                    /* Reset raw buffer for new target */
                    raw_len = 0;
                    raw_buffer[0] = '\0';
                    have_raw_target = FALSE;
                }

                /* Parse %name%=value */
                PU8 first_pct = line + 0;
                PU8 second_pct = STRCHR(first_pct + 1, '%');
                if (!second_pct) goto fail;
                *second_pct = '\0';
                PU8 name = first_pct + 1;

                PU8 eq = STRCHR(second_pct + 1, '=');
                if (!eq) goto fail;
                PU8 value_str = eq + 1;

                PU8 parsed = PARSE_VALUE(value_str, vars);
                if (!parsed) goto fail;

                /* Start new raw target with parsed as initial content */
                STRNCPY(raw_key_name, name, sizeof(raw_key_name));
                raw_key_name[sizeof(raw_key_name)-1] = '\0';

                U32 p_len = STRLEN(parsed);
                if (p_len >= sizeof(raw_buffer)) { MFree(parsed); goto fail; }
                MEMCPY(raw_buffer, parsed, p_len);
                raw_len = p_len;
                raw_buffer[raw_len] = '\0';

                have_raw_target = TRUE;
                MFree(parsed);
                continue;
            }

            /* Otherwise append this line + newline to current raw buffer */
            if (!have_raw_target) {
                /* stray text inside raw block before a %name%=value is an error */
                goto fail;
            }

            U32 l = STRLEN(line);
            if (raw_len + l + 2 >= sizeof(raw_buffer)) goto fail;
            /* append newline then the line (preserve separation shown in example) */
            raw_buffer[raw_len++] = '\n';
            MEMCPY(raw_buffer + raw_len, line, l);
            raw_len += l;
            raw_buffer[raw_len] = '\0';
            continue;
        }

        /* ============================
           VARIABLE ASSIGNMENT  %name%=value
           (normal non-raw-mode variables)
        ============================ */
        if (line[0] == '%') {
            DEBUG_PRINTF("Parsing variable assignment line: %s\n", line);
            PU8 first_pct = line + 0;
            PU8 second_pct = STRCHR(first_pct + 1, '%');
            if (!second_pct) goto fail;

            *second_pct = '\0';
            PU8 name = first_pct + 1;

            PU8 eq = STRCHR(second_pct + 1, '=');
            if (!eq) goto fail;

            PU8 value_str = eq + 1;
            PU8 parsed = PARSE_VALUE(value_str, vars);
            if (!parsed) goto fail;

            if (!ADD_KEY_VALUE(vars, name, parsed)) {
                MFree(parsed);
                goto fail;
            }
            MFree(parsed);
            continue;
        }

        /* ============================
           BLOCK HEADER  [block]
        ============================ */
        if (line[0] == '[') {
            DEBUG_PRINTF("Parsing block header line: %s\n", line);
            PU8 end = STRRCHR(line, ']');
            if (!end) goto fail;
            *end = '\0';
            PU8 block_name = line + 1;

            if (!ADD_BLOCK(blocks, block_name)) goto fail;
            current_block = blocks->blocks[blocks->len - 1];
            continue;
        }

        /* ============================
           NORMAL KEY = VALUE
        ============================ */
        {
            DEBUG_PRINTF("Parsing key/value line: %s\n", line);
            PU8 eq = STRCHR(line, '=');
            if (!eq) goto fail;

            *eq = '\0';
            PU8 key = line;
            PU8 value_str = eq + 1;
            PU8 parsed = PARSE_VALUE(value_str, vars);
            if (!parsed) goto fail;
            DEBUG_PRINTF("Adding key '%s' with value '%s' to block '%s'\n", key, parsed, current_block->name);
            if (!ADD_KEY_VALUE(&current_block->keys, key, parsed)) {
                MFree(parsed);
                goto fail;
            }
            MFree(parsed);
            continue;
        }
    } /* end reading file */

    /* If we exited file while still in raw mode, that's an error */
    if (in_raw_string) goto fail;

    result = TRUE;
    DEBUG_PRINTF("Successfully parsed ATRC file: %s\n", fd->filepath);
    goto done;

fail:
    DEBUG_PRINTF("Failed to parse ATRC file: %s\n", fd->filepath);
    result = FALSE;
    DESTROY_ATRCFD(fd);

done:
    if (file) FCLOSE(file);
    file = NULLPTR;
    return result;
}

/* ---------------- CREATE/READ/DESTROY ATC ----------------- */
PATRC_FD CREATE_ATRCFD(PU8 filepath, READMODES mode) {
    DEBUG_PRINTF("Create atrcfd for file: %s\n", filepath);
    PATRC_FD fd = MAlloc(sizeof(ATRCFD));
    if(!fd) return NULLPTR;
    fd->filepath = NULLPTR;
    fd->mode = mode;
    fd->autosave = FALSE;
    fd->writecheck = FALSE;
    fd->blocks.len = 0;
    fd->vars.len = 0;
    if(!READ_ATRCFD(fd, filepath, mode)) {
        DESTROY_ATRCFD(fd);
        return NULLPTR;
    }
    return fd;
}
BOOL READ_ATRCFD(PATRC_FD fd, PU8 filepath, READMODES mode) {
    DEBUG_PRINTF("Reading atrcfd for file: %s\n", filepath);
    if(!filepath) return FALSE;
    if(!fd) CREATE_ATRCFD(filepath, mode);
    if(fd->filepath) { MFree(fd->filepath); fd->filepath = NULLPTR; }
    fd->filepath = STRDUP(filepath);
    fd->mode = mode;
    fd->blocks.len = 0;
    fd->vars.len = 0;
    DEBUG_PRINTF("Parsing ATRC file: %s\n", fd->filepath);
    return PARSE_ATRC(fd);
}
VOID DESTROY_ATRCFD(PATRC_FD fd) {
    if(!fd) return;
    // free vars
    for(U32 i = 0; i < fd->vars.len; i++) {
        PKEY_VALUE kv = fd->vars.key_value_pairs[i];
        if(kv) {
            if(kv->name) MFree(kv->name);
            if(kv->value) MFree(kv->value);
            MFree(kv);
        }
    }
    fd->vars.len = 0;
    // free blocks
    for(U32 i = 0; i < fd->blocks.len; i++) {
        PBLOCK b = fd->blocks.blocks[i];
        if(b) {
            for(U32 j = 0; j < b->keys.len; j++) {
                PKEY_VALUE kv = b->keys.key_value_pairs[j];
                if(kv) {
                    if(kv->name) MFree(kv->name);
                    if(kv->value) MFree(kv->value);
                    MFree(kv);
                }
            }
            if(b->name) MFree(b->name);
            MFree(b);
        }
    }
    fd->blocks.len = 0;
    if(fd->filepath) MFree(fd->filepath);
    MFree(fd);
}
VOID SET_WRITECHECK(PATRC_FD fd, BOOL writecheck) {
    if(!fd) return;
    fd->writecheck = writecheck;
}
VOID SET_AUTOSAVE(PATRC_FD fd, BOOL autosave) {
    if(!fd) return;
    fd->autosave = autosave;
}

/* ---------------- COPY_ATRCFD (deep copy) ----------------- */
PATRC_FD COPY_ATRCFD(PATRC_FD src) {
    if(!src) return NULLPTR;
    PATRC_FD dst = MAlloc(sizeof(ATRCFD));
    if(!dst) return NULLPTR;
    dst->filepath = src->filepath ? STRDUP(src->filepath) : NULLPTR;
    dst->mode = src->mode;
    dst->autosave = src->autosave;
    dst->writecheck = src->writecheck;
    dst->blocks.len = 0;
    dst->vars.len = 0;

    // copy vars
    for(U32 i=0;i<src->vars.len;i++){
        PKEY_VALUE s = src->vars.key_value_pairs[i];
        if(s) {
            ADD_KEY_VALUE(&dst->vars, s->name, s->value);
        }
    }
    // copy blocks and keys
    for(U32 i=0;i<src->blocks.len;i++){
        PBLOCK sb = src->blocks.blocks[i];
        if(sb) {
            ADD_BLOCK(&dst->blocks, sb->name);
            PBLOCK db = dst->blocks.blocks[dst->blocks.len - 1];
            for(U32 j=0;j<sb->keys.len;j++){
                PKEY_VALUE sk = sb->keys.key_value_pairs[j];
                if(sk) ADD_KEY_VALUE(&db->keys, sk->name, sk->value);
            }
        }
    }
    return dst;
}

/* ---------------- Read / Modify / Create / Delete Variables -----------------
   READ_VARIABLE returns pointer owned by fd (do not free). Returns NULLPTR if not found.
   CREATE_VARIABLE and MODIFY_VARIABLE return pointer to stored value (owned by fd).
   DELETE_VARIABLE removes variable and frees memory, returns NULLPTR.
*/
PU8 READ_VARIABLE(PATRC_FD fd, PU8 variable_name) {
    if(!fd || !variable_name) return NULLPTR;
    PKEY_VALUE kv = GET_KEY_VALUE(&fd->vars, variable_name);
    if(!kv) return NULLPTR;
    return kv->value;
}
PU8 CREATE_VARIABLE(PATRC_FD fd, PU8 variable_name, PU8 value) {
    if(!fd || !variable_name || !value) return NULLPTR;
    if(!ADD_KEY_VALUE(&fd->vars, variable_name, value)) return NULLPTR;
    PKEY_VALUE kv = GET_KEY_VALUE(&fd->vars, variable_name);
    return kv ? kv->value : NULLPTR;
}
PU8 MODIFY_VARIABLE(PATRC_FD fd, PU8 variable_name, PU8 value) {
    if(!fd || !variable_name || !value) return NULLPTR;
    PKEY_VALUE kv = GET_KEY_VALUE(&fd->vars, variable_name);
    if(!kv) {
        if(fd->writecheck) {
            if(!ADD_KEY_VALUE(&fd->vars, variable_name, value)) return NULLPTR;
            kv = GET_KEY_VALUE(&fd->vars, variable_name);
            return kv ? kv->value : NULLPTR;
        }
        return NULLPTR;
    }
    if(kv->value) MFree(kv->value);
    kv->value = STRDUP(value);
    return kv->value;
}
PU8 DELETE_VARIABLE(PATRC_FD fd, PU8 variable_name) {
    if(!fd || !variable_name) return NULLPTR;
    PKEY_VALUE kv = GET_KEY_VALUE(&fd->vars, variable_name);
    if(!kv) return NULLPTR;
    // find index
    INT idx = -1;
    for(U32 i=0;i<fd->vars.len;i++){
        if(fd->vars.key_value_pairs[i] == kv) { idx = i; break;}
    }
    if(idx == -1) return NULLPTR;
    // free
    if(kv->name) MFree(kv->name);
    if(kv->value) MFree(kv->value);
    MFree(kv);
    // shift left
    for(U32 i = idx; i+1 < fd->vars.len; i++) {
        fd->vars.key_value_pairs[i] = fd->vars.key_value_pairs[i+1];
    }
    fd->vars.key_value_pairs[fd->vars.len - 1] = NULLPTR;
    fd->vars.len--;
    return NULLPTR;
}

/* ---------------- Blocks: create / delete ----------------- */
PU8 CREATE_BLOCK(PATRC_FD fd, PU8 block_name) {
    if(!fd || !block_name) return NULLPTR;
    if(!ADD_BLOCK(&fd->blocks, block_name)) return NULLPTR;
    PBLOCK b = fd->blocks.blocks[fd->blocks.len - 1];
    return b->name;
}
PU8 DELETE_BLOCK(PATRC_FD fd, PU8 block_name) {
    if(!fd || !block_name) return NULLPTR;
    INT idx = -1;
    for(U32 i=0;i<fd->blocks.len;i++){
        if(fd->blocks.blocks[i] && fd->blocks.blocks[i]->name && STRCMP(fd->blocks.blocks[i]->name, block_name) == 0) {
            idx = i; break;
        }
    }
    if(idx == -1) return NULLPTR;
    PBLOCK b = fd->blocks.blocks[idx];
    // free block contents
    for(U32 j=0;j<b->keys.len;j++){
        PKEY_VALUE kv = b->keys.key_value_pairs[j];
        if(kv) {
            if(kv->name) MFree(kv->name);
            if(kv->value) MFree(kv->value);
            MFree(kv);
        }
    }
    if(b->name) MFree(b->name);
    MFree(b);
    // shift
    for(U32 i = idx; i+1 < fd->blocks.len; i++) {
        fd->blocks.blocks[i] = fd->blocks.blocks[i+1];
    }
    fd->blocks.blocks[fd->blocks.len - 1] = NULLPTR;
    fd->blocks.len--;
    return NULLPTR;
}

/* ---------------- Keys: read / modify / create / delete ----------------- */
PU8 READ_KEY(PATRC_FD fd, PU8 block_name, PU8 key_name) {
    if(!fd || !block_name || !key_name) return NULLPTR;
    for(U32 i=0;i<fd->blocks.len;i++){
        PBLOCK b = fd->blocks.blocks[i];
        if(b && b->name && STRCMP(b->name, block_name) == 0) {
            PKEY_VALUE kv = GET_KEY_VALUE(&b->keys, key_name);
            if(kv) return kv->value;
            return NULLPTR;
        }
    }
    return NULLPTR;
}
PU8 CREATE_KEY(PATRC_FD fd, PU8 block_name, PU8 key_name, PU8 value) {
    if(!fd || !block_name || !key_name || !value) return NULLPTR;
    // find block
    PBLOCK target = NULLPTR;
    for(U32 i=0;i<fd->blocks.len;i++){
        if(fd->blocks.blocks[i] && fd->blocks.blocks[i]->name && STRCMP(fd->blocks.blocks[i]->name, block_name) == 0) {
            target = fd->blocks.blocks[i];
            break;
        }
    }
    if(!target) {
        if(!fd->writecheck) return NULLPTR;
        // create block if writecheck enabled
        if(!ADD_BLOCK(&fd->blocks, block_name)) return NULLPTR;
        target = fd->blocks.blocks[fd->blocks.len - 1];
    }
    if(!ADD_KEY_VALUE(&target->keys, key_name, value)) return NULLPTR;
    PKEY_VALUE kv = GET_KEY_VALUE(&target->keys, key_name);
    return kv ? kv->value : NULLPTR;
}
PU8 MODIFY_KEY(PATRC_FD fd, PU8 block_name, PU8 key_name, PU8 value) {
    if(!fd || !block_name || !key_name || !value) return NULLPTR;
    // find block
    PBLOCK target = NULLPTR;
    for(U32 i=0;i<fd->blocks.len;i++){
        if(fd->blocks.blocks[i] && fd->blocks.blocks[i]->name && STRCMP(fd->blocks.blocks[i]->name, block_name) == 0) {
            target = fd->blocks.blocks[i];
            break;
        }
    }
    if(!target) {
        if(fd->writecheck) {
            if(!ADD_BLOCK(&fd->blocks, block_name)) return NULLPTR;
            target = fd->blocks.blocks[fd->blocks.len - 1];
        } else return NULLPTR;
    }
    PKEY_VALUE kv = GET_KEY_VALUE(&target->keys, key_name);
    if(!kv) {
        if(fd->writecheck) {
            if(!ADD_KEY_VALUE(&target->keys, key_name, value)) return NULLPTR;
            kv = GET_KEY_VALUE(&target->keys, key_name);
            return kv ? kv->value : NULLPTR;
        }
        return NULLPTR;
    }
    if(kv->value) MFree(kv->value);
    kv->value = STRDUP(value);
    return kv->value;
}
PU8 DELETE_KEY(PATRC_FD fd, PU8 block_name, PU8 key_name) {
    if(!fd || !block_name || !key_name) return NULLPTR;
    for(U32 i=0;i<fd->blocks.len;i++){
        PBLOCK b = fd->blocks.blocks[i];
        if(b && b->name && STRCMP(b->name, block_name) == 0) {
            INT idx = -1;
            for(U32 j=0;j<b->keys.len;j++){
                if(b->keys.key_value_pairs[j] && b->keys.key_value_pairs[j]->name && STRCMP(b->keys.key_value_pairs[j]->name, key_name) == 0) {
                    idx = j; break;
                }
            }
            if(idx == -1) return NULLPTR;
            PKEY_VALUE kv = b->keys.key_value_pairs[idx];
            if(kv->name) MFree(kv->name);
            if(kv->value) MFree(kv->value);
            MFree(kv);
            for(U32 j = idx; j + 1 < b->keys.len; j++) {
                b->keys.key_value_pairs[j] = b->keys.key_value_pairs[j+1];
            }
            b->keys.key_value_pairs[b->keys.len - 1] = NULLPTR;
            b->keys.len--;
            return NULLPTR;
        }
    }
    return NULLPTR;
}

/* ---------------- Existence checks ----------------- */
BOOL DOES_EXIST_BLOCK(PATRC_FD fd, PU8 block_name) {
    if(!fd || !block_name) return FALSE;
    for(U32 i=0;i<fd->blocks.len;i++){
        PBLOCK b = fd->blocks.blocks[i];
        if(b && b->name && STRCMP(b->name, block_name) == 0) return TRUE;
    }
    return FALSE;
}
BOOL DOES_EXIST_KEY(PATRC_FD fd, PU8 block_name, PU8 key_name) {
    if(!fd || !block_name || !key_name) return FALSE;
    for(U32 i=0;i<fd->blocks.len;i++){
        PBLOCK b = fd->blocks.blocks[i];
        if(b && b->name && STRCMP(b->name, block_name) == 0) {
            if(GET_KEY_VALUE(&b->keys, key_name)) return TRUE;
            return FALSE;
        }
    }
    return FALSE;
}
BOOL DOES_EXIST_VARIABLE(PATRC_FD fd, PU8 variable_name) {
    if(!fd || !variable_name) return FALSE;
    return GET_KEY_VALUE(&fd->vars, variable_name) != NULLPTR;
}

/* ---------------- GET_ENUM_VALUE ----------------- */
U32 GET_ENUM_VALUE(PATRC_FD fd, PU8 block_name, PU8 key_name) {
    if(!fd || !block_name || !key_name) return (U32)-1;
    for(U32 i=0;i<fd->blocks.len;i++){
        PBLOCK b = fd->blocks.blocks[i];
        if(b && b->name && STRCMP(b->name, block_name) == 0) {
            PKEY_VALUE kv = GET_KEY_VALUE(&b->keys, key_name);
            if(kv) return kv->enum_val;
            return (U32)-1;
        }
    }
    return (U32)-1;
}


PU8 INSERT_VAR(PU8 line, PPU8 args, U32 arg_count)
{
    if (!line || !args || arg_count == 0)
        return NULLPTR;

    PU8 retval = NULLPTR; // start with NULL, will grow with STRAPPEND
    U32 current_arg = 0;
    U32 len = STRLEN(line);

    for (U32 i = 0; i < len; i++)
    {
        CHAR c = line[i];

        if (c == '%' && line[i + 1] == '*')
        {
            CHAR next = line[i + 2];

            // %*% → insert next argument
            if (next == '%')
            {
                if (current_arg < arg_count)
                    retval = STRAPPEND(retval, args[current_arg++]);

                i += 2; // skip '*%'
                continue;
            }

            // %*<digits>% → insert argument at index
            if (IS_DIGIT(next))
            {
                U32 index = 0;
                U32 j = i + 2;
                U32 digits = 0;

                // Read up to 3 digits
                while (digits < 3 && IS_DIGIT(line[j]))
                {
                    index = index * 10 + (line[j] - '0');
                    j++;
                    digits++;
                }

                // Check for ending %
                if (line[j] == '%' && index < arg_count)
                {
                    retval = STRAPPEND(retval, args[index]);
                    i = j; // skip '*<digits>%'
                    continue;
                }

                // Invalid pattern, treat %* literally
                retval = STRAPPEND(retval, "%*");
                i += 1; // skip '*' only
                continue;
            }

            // Not a valid pattern, treat %* literally
            retval = STRAPPEND(retval, "%*");
            i += 1; // skip '*'
            continue;
        }

        // Normal character → append single char
        CHAR tmp[2] = { c, '\0' };
        retval = STRAPPEND(retval, tmp);
    }

    return retval;
}



/* ---------------- Utility: ATRC_TO_LIST / FREE_STR_ARR / ATRC_TO_BOOL ----------------- */
PSTR_ARR ATRC_TO_LIST(PU8 val, CHAR separator) {
    if(!val) return NULLPTR;
    PSTR_ARR arr = MAlloc(sizeof(STR_ARR));
    if(!arr) return NULLPTR;
    arr->len = 0;
    PU8 val_dup = STRDUP(val);
    if(!val_dup) { MFree(arr); return NULLPTR; }
    U8 sep_str[2] = { (U8)separator, '\0' };
    PU8 saveptr = NULLPTR;
    PU8 tok = STRTOK_R(val_dup, sep_str, &saveptr);
    while(tok != NULL && arr->len < MAX_ATRC_STRS) {
        arr->strs[arr->len++] = STRDUP(tok);
        tok = STRTOK_R(NULL, sep_str, &saveptr);
    }
    MFree(val_dup);
    return arr;
}

VOID FREE_STR_ARR(PSTR_ARR arr) {
    if(!arr) return;
    for(U32 i = 0; i < arr->len; i++) {
        if(arr->strs[i]) MFree(arr->strs[i]);
    }
    MFree(arr);
}

BOOL ATRC_TO_BOOL(PU8 val) {
    if(!val) return FALSE;
    if(
        STRICMP(val, "TRUE") == 0 ||
        STRICMP(val, "ON") == 0 ||
        STRCMP(val, "1") == 0
    ) return TRUE;
    return FALSE;
}

#endif // ATRC_IMPLEMENTATION

#endif // ATRC_H
