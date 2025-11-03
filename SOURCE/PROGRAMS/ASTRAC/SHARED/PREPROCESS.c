#include "../ASSEMBLER/ASSEMBLER.h"

#define MAX_PREPROCESSING_UNITS MAX_FILES

typedef enum {
    ident_include,

    ident_ifdef,
    ident_ifndef,
    ident_elif,
    ident_else,
    ident_endif,

    ident_error,
    ident_warning,

    ident_define,
    ident_undef,

    ident_max,
} MACRO_IDENT;
typedef struct {
    PU8 str;
    MACRO_IDENT ident;
} MACRO_KW;
static const MACRO_KW macros_kw[] ATTRIB_RODATA = {
    {"#include", ident_include},

    {"#ifdef", ident_ifdef},
    {"#ifndef", ident_ifndef},
    {"#elif", ident_elif},
    {"#else", ident_else},
    {"#endif", ident_endif},

    {"#error", ident_error},
    {"#warning", ident_warning},
    
    {"#define", ident_define},
    {"#undef", ident_undef},
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

typedef struct {
    BOOL active;   // Is this block active? (e.g., #ifdef true)
    BOOL was_true; // Did any branch already succeed (#elif, #else control)
} IF_STATE;

#define IF_STACK_MAX 32
static IF_STATE if_stack[IF_STACK_MAX] ATTRIB_DATA = { 0 };
static U32 if_stack_top ATTRIB_DATA = 0;

STATIC VOID IF_STACK_PUSH_IF(PU8 name, MACRO_ARR *arr, BOOL is_not) {
    if (if_stack_top >= IF_STACK_MAX) return;

    IF_STATE s;
    s.active = is_not ? (GET_MACRO(name, arr) == NULL)
                      : (GET_MACRO(name, arr) != NULL);
    s.was_true = s.active;
    if_stack[if_stack_top++] = s;
}


STATIC VOID IF_STACK_ELIF(PU8 name, MACRO_ARR *arr) {
    if (if_stack_top == 0) return;
    IF_STATE *s = &if_stack[if_stack_top - 1];

    if (s->was_true) {
        // A previous branch was true → skip this one
        s->active = FALSE;
    } else {
        s->active = (GET_MACRO(name, arr) != NULL);
        s->was_true = s->active;
    }
}

STATIC VOID IF_STACK_ELSE() {
    if (if_stack_top == 0) return;
    IF_STATE *s = &if_stack[if_stack_top - 1];

    // If a branch was already true, this one is false
    s->active = !s->was_true;
    s->was_true = TRUE; // after else, considered satisfied
}

STATIC VOID IF_STACK_POP() {
    if (if_stack_top > 0) if_stack_top--;
}

STATIC BOOL IF_STACK_IS_ACTIVE() {
    for (U32 i = 0; i < if_stack_top; i++) {
        if (!if_stack[i].active)
            return FALSE;
    }
    return TRUE;
}



VOID FREE_PREPROCESSING_UNITS();

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

PU8 EXTRACT_NAME(PU8 line, MACRO_IDENT type) {
    U32 add = STRLEN(macros_kw[type].str);
    PU8 p = line + add;
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

PU8 EXTRACT_VALUE(PU8 line, MACRO_IDENT type) {
    U32 add = STRLEN(macros_kw[type].str);
    PU8 p = line + add;
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
    while (FILE_GET_LINE(file, buf, sizeof(buf))) {
        REMOVE_COMMENTS(buf, ASM_PREPROCESSOR);
        if(IS_EMPTY(buf)) continue;
        U32 len = STRLEN(buf);

        // Detect preprocessor directive
        U32 matched_kw = (U32)-1;
        for (U32 i = 0; i < ident_max; i++) {
            DEBUG_PUTS(buf); DEBUG_PUTC(' ');
            if (STRISTR(buf, macros_kw[i].str)) { // must be at start of line
                matched_kw = i; 
                DEBUG_PUTS_LN("Matched");
                break;
            }
            DEBUG_PUTS_LN(macros_kw[i].str);
        }

        // Check if this section of code should be active
        BOOL active = IF_STACK_IS_ACTIVE();
        if (matched_kw != (U32)-1) {
            const MACRO_KW kw = macros_kw[matched_kw];
            PU8 name = EXTRACT_NAME(buf, kw.ident);
            PU8 value = EXTRACT_VALUE(buf, kw.ident);
            MACRO_ARR *mcr = &units[unit_cnt].macros;

            
            switch (kw.ident) {
                case ident_ifdef:  IF_STACK_PUSH_IF(name, mcr, FALSE); break;
                case ident_ifndef: IF_STACK_PUSH_IF(name, mcr, TRUE);  break;
                case ident_elif:   IF_STACK_ELIF(name, mcr);           break;
                case ident_else:   IF_STACK_ELSE();                    break;
                case ident_endif:  IF_STACK_POP();                     break;

                default: {
                    // Skip all other directives if inactive
                    if (!active) break;

                    switch (kw.ident) {
                        case ident_define: {
                            if (!DEFINE_MACRO(name, value, mcr)) {
                                printf("Failed to define macro #%s=%s\n", name, value);
                                MFree(name);
                                MFree(value);
                                FCLOSE(tmp_file);
                                return FALSE;
                            }
                        } break;
                        case ident_undef: {
                            UNDEFINE_MACRO(name, mcr);
                        } break;
                        case ident_include:
                            // (future implementation)
                            break;
                        case ident_error: {
                            printf("ASM PREPROCESSOR ERROR: %s\n", value);
                            MFree(name);
                            MFree(value);
                            FCLOSE(tmp_file);
                            return FALSE;
                        } break;
                        case ident_warning: {
                            printf("ASM PREPROCESSOR WARNING: %s\n", value);
                        } break;
                    }
                } break;
            }

            MFree(name);
            MFree(value);
            continue; // never write directive lines
        }

        // If not a preprocessor line, skip writing if inactive
        if (!active)
            continue;

        // Process and write active code line
        REPLACE_MACROS_IN_LINE(buf);
        if (len + 1 < BUF_SZ) {
            buf[len] = '\n';
            if (!FWRITE(tmp_file, buf, len + 1)) {
                FCLOSE(tmp_file);
                return FALSE;
            }
        }
    }

    FCLOSE(tmp_file);
    return TRUE;
}


PASM_INFO PREPROCESS_ASM() {
    unit_cnt = 0;
    ASTRAC_ARGS *args = GET_ARGS();
    PASM_INFO info = MAlloc(sizeof(ASM_INFO));
    if(!info) return NULLPTR;
    if_stack_top = 0;

    for(U32 i = 0; i < args->input_file_count; i++) {
        U8 buf[32] = "/TMP/";
        SPRINTF(buf, "/TMP/%02x.ASM", info->tmp_file_count);
        if(!FILE_CREATE(buf)) {
            FREE_PREPROCESSING_UNITS();
        }

        FILE tmp;
        if(!FOPEN(&tmp, buf, MODE_RA | MODE_FAT32)) {
            FREE_PREPROCESSING_UNITS();
            return info;
        }

        info->tmp_files[info->tmp_file_count++] = STRDUP(buf);

        units[unit_cnt].type = ASM_PREPROCESSOR;
        PU8 file_name = args->input_files[i];
        if(!FOPEN(&units[unit_cnt].file, file_name, MODE_R | MODE_FAT32)) {
            FREE_PREPROCESSING_UNITS();
            return NULLPTR;
        }

        if(!PREPROCESS_FILE(&units[unit_cnt].file, &tmp)) {
            FREE_PREPROCESSING_UNITS();
            return info;
        }


        FCLOSE(&tmp);
        unit_cnt++;
    }
    FREE_PREPROCESSING_UNITS();
    return info;
}

VOID FREE_PREPROCESSING_UNITS() {
    for (U32 i = 0; i < unit_cnt; i++) {
        PREPROCESSING_UNIT *unit = &units[i];

        // Close file if still open
        FCLOSE(&unit->file);

        // Free macros in this unit
        FREE_MACROS(&unit->macros);

        // Reset structure fields
        MEMSET(&unit->file, 0, sizeof(FILE));
        MEMSET(&unit->macros, 0, sizeof(MACRO_ARR));
        unit->type = 0;
    }

    // Reset global counters
    unit_cnt = 0;

    // Also clear global macros if you want a full reset
    FREE_MACROS(&glb_macros);
}
