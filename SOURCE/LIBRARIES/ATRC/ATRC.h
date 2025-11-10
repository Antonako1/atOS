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

// misc
PU8 INSERT_VAR(PU8 line, PPU8 args); // Return must be freed
U32 GET_ENUM_VALUE(PATRC_FD fd, PU8 block_name, PU8 key_name);
BOOL WRITE_COMMENT_TO_TOP(PATRC_FD fd);
BOOL WRITE_COMMENT_TO_BOTTOM(PATRC_FD fd);

// functions to ease life
#define MAX_ATRC_STRS 32
typedef struct _STR_ARR {
    PU8 strs[MAX_ATRC_STRS];
    U32 len;
} STR_ARR, *PSTR_ARR;

PSTR_ARR ATRC_TO_LIST(PU8 val, CHAR separator); // Free with FREE_STR_ARR()
VOID FREE_STR_ARR(PSTR_ARR arr);
BOOL ATRC_TO_BOOL(PU8 val); // Accepts: TRUE, 1, ON, FALSE, 0, 0FF. case-insensitive.

#define ATRC_IMPLEMENTATION
#ifdef ATRC_IMPLEMENTATION
#include <STD/MEM.h>
#include <STD/STRING.h>
#include <STD/FS_DISK.h>

BOOL PARSE_ATRC(PATRC_FD fd) {
    PBLOCK_ARR blocks = &fd->blocks;
    PKEY_VALUE_ARR vars = &fd->vars;
    /*
    parses line by line, looking for
    % - definition or modification of variable
    [ - block start
    # - comment or preprocessor tag
    everything else is parsed as key.
    */
    U8 line[512] = { 0 };
    FILE *file = FOPEN(fd->filepath, MODE_FR);
    while(FILE_GET_LINE(file, line, sizeof(line))) {
        
    }
    return TRUE;
}

PATRC_FD CREATE_ATRCFD(PU8 filepath, READMODES mode) {
    PATRC_FD fd = MAlloc(sizeof(ATRCFD));

    if(!fd) return NULLPTR;
    if(!READ_ATRCFD(fd, filepath, mode)) {
        DESTROY_ATRCFD(fd);
        return NULLPTR;
    }
    return fd;
}
BOOL READ_ATRCFD(PATRC_FD fd, PU8 filepath, READMODES mode) {
    if(!fd) return FALSE;
    if(fd->filepath) MFree(fd->filepath);
    fd->filepath = STRDUP(filepath);
    fd->mode = mode;
    return PARSE_ATRC(fd);
}
VOID DESTROY_ATRCFD(PATRC_FD fd) {
    if(!fd) return;
    for(U32 i = 0; i < fd->vars.len; i++) {
        MFree(fd->vars.key_value_pairs[i]->name);
        MFree(fd->vars.key_value_pairs[i]->value);
    }
    for(U32 i = 0; i < fd->blocks.len; i++) {
        for(U32 j = 0; j < fd->blocks.blocks[i]->keys.len; j++) {
            MFree(fd->blocks.blocks[i]->keys.key_value_pairs[j]->name);
            MFree(fd->blocks.blocks[i]->keys.key_value_pairs[j]->value);
        }
        MFree(fd->blocks.blocks[i]->name);
    }
    if(fd) MFree(fd);
}
VOID SET_WRITECHECK(PATRC_FD fd, BOOL writecheck) {
    fd->writecheck = writecheck;
}
VOID SET_AUTOSAVE(PATRC_FD fd, BOOL autosave) {
    fd->autosave = autosave;
}
PATRC_FD COPY_ATRCFD(PATRC_FD src) {
    return NULLPTR;
}


PSTR_ARR ATRC_TO_LIST(PU8 val, CHAR separator) {
    if(!val || !separator) return NULLPTR;
    
    PSTR_ARR arr = MAlloc(sizeof(STR_ARR)); 
    if(!arr) return NULLPTR;
    arr->len = 0;
    
    PU8 saveptr = NULLPTR;
    PU8 val_dup = STRDUP(val);
    if(!val_dup) {
        MFree(arr);
        return NULLPTR;
    }
    U8 sep_str[2] = { separator, '\0' }; 
    
    if (MAX_ATRC_STRS <= 0) {
        MFree(val_dup);
        MFree(arr);
        return NULLPTR;
    }

    PU8 tok = STRTOK_R(val_dup, sep_str, &saveptr);
    
    while(tok != NULL && arr->len < MAX_ATRC_STRS ){
        arr->strs[arr->len++] = STRDUP(tok);
        tok = STRTOK_R(NULL, sep_str, &saveptr);
    }
    MFree(val_dup);
    return arr;
}

VOID FREE_STR_ARR(PSTR_ARR arr) {
    for(U32 i = 0; i < arr->len; i++) {
        if(arr->strs[i])MFree(arr->strs[i]);
    }
    MFree(arr->strs);
    if(arr) MFree(arr);
}

BOOL ATRC_TO_BOOL(PU8 val) {
    if(
        STRICMP(val, "TRUE") == 0 ||
        STRICMP(val, "ON") == 0 ||
        STRCMP(val, "1") == 0
    ) return TRUE;
    return FALSE;
}
#endif // ATRC_IMPLEMENTATION

#endif // ATRC_H