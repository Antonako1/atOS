#include "../ASSEMBLER/ASSEMBLER.h"

#define MAX_PREPROCESSING_UNITS MAX_FILES

static const PU8 macros[] ATTRIB_RODATA = {
    "include",

    "ifndef",
    "elif",
    "endif",

    "error",
    "warning",
   
    "define",
    "undef",
};
static U8 tmp_str[BUF_SZ] ATTRIB_DATA = { 0 };
static MACRO_ARR glb_macros ATTRIB_DATA = { 0 };

typedef struct {
    FILE file;
    MACRO_ARR macros;
    U32 type;
} PREPROCESSING_UNIT;

static PREPROCESSING_UNIT units[MAX_PREPROCESSING_UNITS] ATTRIB_DATA = { 0 };
static U32 unit_cnt ATTRIB_DATA = 0;

BOOL DEFINE_MACRO(PU8 name, PU8 value, MACRO_ARR *arr) {
    if(arr->len >= MAX_MACROS) return FALSE;

    PMACRO new = MAlloc(sizeof(MACRO));
    if(!new) return FALSE;

    STRNCPY(new->name, name, MAX_MACRO_VALUE);
    STRNCPY(new->value, value, MAX_MACRO_VALUE);

    arr->macros[arr->len++] = new;

    return TRUE;
}

PMACRO GET_MACRO(PU8 name, MACRO_ARR *arr) {
    for(U32 i = 0; i < arr->len; i++) {
        PMACRO macro = arr->macros[i];
        if(STRCMP(macro->name, name) == 0) {
            return macro;
        }
    }
    return NULLPTR;
}

VOID UNDEFINE_MACRO(PU8 name, MACRO_ARR *arr) {
    BOOL found = FALSE;
    for(U32 i = 0; i < arr->len; i++) {
        if(!found && STRCMP(arr->macros[i]->name, name) == 0) {
            MFree(arr->macros[i]);
            found = TRUE;
        }
        if(found && i < arr->len - 1) {
            arr->macros[i] = arr->macros[i + 1];
        }
    }
    if(found) arr->len--;
}

VOID FREE_MACROS(MACRO_ARR *arr) {
    for(U32 i = 0; i < arr->len; i++) {
        PMACRO macro = arr->macros[i];
        if(macro) MFree(macro);
    }
    arr->len = 0;
}



VOID REMOVE_COMMENTS(PU8 line, U32 cmnt_type) {
    U32 len = STRLEN(line);
    BOOL in_str = FALSE;
    if(cmnt_type == ASM_PREPROCESSOR) {
        for(U32 i = 0; i < len; i++) {
            U8 c = line[i];
            if(c == '\\' && in_str) continue;
            if(c == '"') in_str = !in_str;
            if(c == ';') {
                line[i] = '\0';
                break;
            }
        }
    }
}

BOOL IS_EMPTY(PU8 line) {
    STRNCPY(tmp_str, line, BUF_SZ);
    str_trim(tmp_str);
    if(STRLEN(tmp_str) == 0) return TRUE;
    return FALSE;
}

PU8 EXTRACT_NAME(PU8 line) {
    // Skip "#define " (8 chars)
    PU8 p = line + 7;
    while(*p == ' ' || *p == '\t') p++; // skip whitespace

    PU8 start = p;
    while(*p && *p != ' ' && *p != '\t') p++;

    U32 len = p - start;
    PU8 name = MAlloc(len + 1);
    if(!name) return NULLPTR;
    STRNCPY(name, start, len);
    name[len] = '\0';
    str_trim(name);
    return name;
}

PU8 EXTRACT_VALUE(PU8 line) {
    // Skip "#define NAME " (first two tokens)
    PU8 p = line + 7;
    while(*p == ' ' || *p == '\t') p++;

    // Skip macro name
    while(*p && *p != ' ' && *p != '\t') p++;
    while(*p == ' ' || *p == '\t') p++; // skip whitespace to value

    U32 len = STRLEN(p);
    PU8 value = MAlloc(len + 1);
    if(!value) return NULLPTR;
    STRNCPY(value, p, len);
    value[len] = '\0';
    str_trim(value);
    return value;
}

VOID REPLACE_MACROS_IN_LINE(PU8 line) {
    STRNCPY(tmp_str, line, BUF_SZ);
    U8 solved = FALSE;
    ASTRAC_ARGS *args = GET_ARGS();
    for(U32 i = 0; i < args->macros.len; i++) {
        PMACRO mcr = args->macros.macros[i];
        PU8 res = STRSTR(tmp_str, mcr->name);
        if(res) {
            solved = TRUE;
            PU8 str = STR_REPLACE(tmp_str, mcr->name, mcr->value);
            STRNCPY(tmp_str, str, BUF_SZ);
            MFree(str);
        }
    }
    if(!solved) {
        for(U32 i = 0; i < units[unit_cnt].macros.len; i++) {
            PMACRO mcr = units[unit_cnt].macros.macros[i];
            PU8 res = STRSTR(tmp_str, mcr->name);
            if(res) {
                solved = TRUE;
                PU8 str = STR_REPLACE(tmp_str, mcr->name, mcr->value);
                STRNCPY(tmp_str, str, BUF_SZ);
                MFree(str);
            }
        }
    }
    STRNCPY(line, tmp_str, BUF_SZ);
}

BOOL PREPROCESS_FILE(FILE* file, FILE* tmp_file) {
    U8 buf[BUF_SZ] = {0};
    while(FILE_GET_LINE(file, buf, sizeof(buf))) {
        U32 len = STRLEN(buf);
        if(IS_EMPTY(buf)) continue;
        REMOVE_COMMENTS(buf, ASM_PREPROCESSOR);

        if(STRSTR(buf, "#define")) {
            PU8 name = EXTRACT_NAME(buf);
            PU8 value = EXTRACT_VALUE(buf);
            
            if(!DEFINE_MACRO(name, value, &units[unit_cnt].macros)) {
                printf("Failed to define macro #%s=%s\n", name, value);
                MFree(name);
                MFree(value);
                FCLOSE(tmp_file);
                return FALSE;
            }
            MFree(name);
            MFree(value);
            continue;
        }

        // Replace macros in line
        REPLACE_MACROS_IN_LINE(buf);

        DEBUG_PUTS_LN("After REPLACE_MACROS_IN_LINE:");
        DEBUG_PUTS_LN(buf);

        if(len + 1 < BUF_SZ) {
            buf[len] = '\n';
            if(!FWRITE(tmp_file, buf, len)) {
                DEBUG_PUTS_LN("Nothing written");
            }
        }
        MEMORY_DUMP(tmp_file->data, tmp_file->sz);
    }
    DEBUG_PUTS_LN("Out we go");
    FCLOSE(tmp_file);
    return TRUE;
}

PASM_INFO PREPROCESS_ASM() {
    unit_cnt = 0;
    ASTRAC_ARGS *args = GET_ARGS();
    PASM_INFO info = MAlloc(sizeof(ASM_INFO));
    if(!info) return NULLPTR;

    for(U32 i = 0; i < args->input_file_count; i++) {
        DEBUG_PUTS("[ASTRAC PREPROCESSER] processing file: ");
        DEBUG_PUTS_LN(args->input_files[i]);


        U8 buf[32] = "/TMP/";
        SPRINTF(buf, "/TMP/PRE_%02x.ASM", info->tmp_file_count);
        if(!FILE_CREATE(buf)) {
            DEBUG_PUTS_LN("Failed to create tmp preproc file:");
            DEBUG_PUTS_LN(buf);
            // TODO: fail
        }

        FILE tmp;
        if(!FOPEN(&tmp, buf, MODE_A | MODE_FAT32)) {
            // TODO: fail
            DEBUG_PUTS_LN("Failed to open tmp preproc file:");
            DEBUG_PUTS_LN(buf);
            return info;
        }

        info->tmp_files[info->tmp_file_count++] = STRDUP(buf);

        units[unit_cnt].type = ASM_PREPROCESSOR;
        PU8 file_name = args->input_files[i];
        if(!FOPEN(&units[unit_cnt].file, file_name, MODE_R | MODE_FAT32)) {
            DEBUG_PUTS_LN("Failed to open input file:");
            DEBUG_PUTS_LN(file_name);
            // TODO: fail
            return NULLPTR;
        }

        if(!PREPROCESS_FILE(&units[unit_cnt].file, &tmp)) {
            DEBUG_PUTS_LN("PREPROCESS_FILE failed for:");
            DEBUG_PUTS_LN(file_name);
            // TODO: fail
            FCLOSE(&units[unit_cnt].file);
            return info;
        }


        FCLOSE(&tmp);
        FCLOSE(&units[unit_cnt].file);
        unit_cnt++;
    }
    DEBUG_PUTS_LN("Outta processor");
    return info;
}
// todo: free units memory 