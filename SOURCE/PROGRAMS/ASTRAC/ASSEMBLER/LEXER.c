#include <PROGRAMS/ASTRAC/ASSEMBLER/ASSEMBLER.h>

typedef struct {
    PU8 value;
    U32 enum_val;
} KEYWORD;

static const KEYWORD registers[] ATTRIB_DATA = {
    /* == General Purpose 32-bit == */
    { "eax", REG_EAX }, { "ebx", REG_EBX }, { "ecx", REG_ECX }, { "edx", REG_EDX },
    { "esi", REG_ESI }, { "edi", REG_EDI }, { "ebp", REG_EBP }, { "esp", REG_ESP },

    /* == General Purpose 16-bit == */
    { "ax", REG_AX }, { "bx", REG_BX }, { "cx", REG_CX }, { "dx", REG_DX },
    { "si", REG_SI }, { "di", REG_DI }, { "bp", REG_BP }, { "sp", REG_SP },

    /* == 8-bit high/low == */
    { "ah", REG_AH }, { "al", REG_AL }, { "bh", REG_BH }, { "bl", REG_BL },
    { "ch", REG_CH }, { "cl", REG_CL }, { "dh", REG_DH }, { "dl", REG_DL },

    /* == Segment Registers == */
    { "cs", REG_CS }, { "ds", REG_DS }, { "es", REG_ES },
    { "fs", REG_FS }, { "gs", REG_GS }, { "ss", REG_SS },

    /* == Control Registers == */
    { "cr0", REG_CR0 }, { "cr2", REG_CR2 }, { "cr3", REG_CR3 }, { "cr4", REG_CR4 },

    /* == Debug Registers == */
    { "dr0", REG_DR0 }, { "dr1", REG_DR1 }, { "dr2", REG_DR2 }, { "dr3", REG_DR3 },
    { "dr6", REG_DR6 }, { "dr7", REG_DR7 },

    /* == x87 FPU Registers == */
    { "st0", REG_ST0 }, { "st1", REG_ST1 }, { "st2", REG_ST2 }, { "st3", REG_ST3 },
    { "st4", REG_ST4 }, { "st5", REG_ST5 }, { "st6", REG_ST6 }, { "st7", REG_ST7 },

    /* == MMX Registers == */
    { "mm0", REG_MM0 }, { "mm1", REG_MM1 }, { "mm2", REG_MM2 }, { "mm3", REG_MM3 },
    { "mm4", REG_MM4 }, { "mm5", REG_MM5 }, { "mm6", REG_MM6 }, { "mm7", REG_MM7 },

    /* == SSE Registers (XMM) == */
    { "xmm0", REG_XMM0 }, { "xmm1", REG_XMM1 }, { "xmm2", REG_XMM2 }, { "xmm3", REG_XMM3 },
    { "xmm4", REG_XMM4 }, { "xmm5", REG_XMM5 }, { "xmm6", REG_XMM6 }, { "xmm7", REG_XMM7 },

    /* End marker */
    { NULL, REG_NONE }
};

static const KEYWORD directives[] ATTRIB_DATA = {
    { ".data", DIR_DATA },       // data section
    { ".data", DIR_RODATA },       // data section
    { ".text", DIR_TEXT },       // code section
    { ".bss",  DIR_BSS },        // uninitialized data
    { ".section", DIR_SECTION }, // custom section
    { ".code", DIR_CODE },       // macro end
    { NULL, 0 }
};

BOOL ADD_TOKEN(ASM_TOK_ARRAY* arr, ASM_TOKEN_TYPE type, PU8 txt, U32 _union, U32 line, U32 col) {
    if (arr->len >= MAX_TOKENS) return FALSE;

    PASM_TOK tok = MAlloc(sizeof(ASM_TOK));
    if(!tok) return FALSE;

    tok->type = type;
    tok->txt = STRDUP(txt);
    tok->num = _union;
    tok->line = line;
    tok->col = col;
    
    arr->toks[arr->len++] = tok;
    return TRUE;
}

ASM_TOK_ARRAY *LEX(PASM_INFO info) {
    ASM_TOK_ARRAY *res = MAlloc(sizeof(ASM_TOK_ARRAY));
    if(!res) return NULLPTR;
    
    U8 buf[BUF_SZ] = { 0 };
    U32 ptr = 0;

    for(U32 i = 0; i < info->tmp_file_count; i++) {
        FILE *file;
        if(!FOPEN(file, info->tmp_files[i], MODE_RW | MODE_FAT32)){ // We will overwrite the preprocessed file
            printf("[ASM LEX] Unable to open tmp file '%s'\n", info->tmp_files[i]);
            DESTROY_TOK_ARR(res);
            return NULLPTR;
        } 
        
        while (READ_LOGICAL_LINE(&file, buf, sizeof(buf))) {
            if(IS_EMPTY(buf)) continue;
            // str is trimmed
            CHAR c = buf[ptr];

            ptr++;
        }
        FCLOSE(file);
    }
    return res;
}