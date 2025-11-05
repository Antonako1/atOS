#include <PROGRAMS/ASTRAC/ASSEMBLER/MNEMONICS.h>
#include <PROGRAMS/ASTRAC/ASSEMBLER/ASSEMBLER.h>
#include <STD/MEM.h>
#include <STD/STRING.h>

PASM_TOK PEEK_TOK(ASM_TOK_ARRAY *toks, U32* index) {
    if(*index + 1 > toks->len) return NULLPTR;
    return toks->toks[*index];
}

BOOLEAN CONSUME_TOK(ASM_AST_ARRAY *arr, PASM_NODE node, U32* index) {
    if(!node) return FALSE;
    if(arr->len + 1 > MAX_NODES) return FALSE;
    arr->toks[arr->len++] = node;
    *index++;
    return TRUE;
}

BOOL ADD_VAR(ASM_VAR *var, ASM_VAR_ARR *vars) {
    if(!var || !vars) return FALSE;
    if(vars->len + 1 > MAX_VARS) return FALSE;
    vars->vars[vars->len++] = var;
    return TRUE;
}

PASM_NODE PARSE_SYMBOL(PASM_TOK tok, U32* index, ASM_TOK_ARRAY *tok_arr, ASM_AST_ARRAY *node_arr) {
    /*
    Here we parse for alot:
    Directives
    Labels
    and more...
    */
    while(*index < tok_arr->len) {
        tok = PEEK_TOK(tok_arr, index);
        switch (tok->type)
        {
        case TOK_SYMBOL:
            break;
        case TOK_IDENTIFIER:
            break;
        default:
            break;
        }
    }
}

PASM_NODE PARSE_IDENTIFIER(PASM_TOK tok, U32* index, ASM_TOK_ARRAY *tok_arr, ASM_AST_ARRAY *node_arr) {
    while(*index < tok_arr->len) {
        tok = PEEK_TOK(tok_arr, index);
        switch (tok->type)
        {
        case TOK_SYMBOL:
            break;
        case TOK_IDENTIFIER:
            break;
        default:
            break;
        }
    }
}
ASM_VAR *PARSE_VAR(PASM_TOK tok, U32* index, ASM_TOK_ARRAY *tok_arr, ASM_AST_ARRAY *node_arr) {

}

PASM_NODE PARSE_RAW_NUMBER(PASM_TOK tok, U32* index, ASM_TOK_ARRAY *tok_arr, ASM_AST_ARRAY *node_arr) {}

/*
Before parsing AST, few notes about lexer

Directives:
    .code
    Parsed as {., code}

Identifiers:
    Parsed as either variable name, mnemonic name or label. 
    If hex, binary or decimal value is found, it will be translated into correct number and left there
    First identifier will be expected as mnemonic if in code section. If value is number, it will be placed as is,
        otherwise it will be treated as mnemonic

    String values are parsed as
    string DB "STRING", <0|LEN>
    To get string value
    len DD $string

Global labels:
    name:
    Parsed as {name, :}

Local labels:
    @@1:, @f, @b
    Parsed as {@, @, 1, :} {@, f} {@, b}
*/
ASM_AST_ARRAY *ASM_AST(ASM_TOK_ARRAY *toks, PASM_INFO info) {
    ASM_AST_ARRAY *arr = MAlloc(sizeof(ASM_AST_ARRAY));
    if(!arr) return NULLPTR;

    ASTRAC_ARGS *args = GET_ARGS();
    
    ASM_VAR_ARR DATA_SECTION;
    ASM_VAR_ARR RODATA_SECTION;
    
    ASM_DIRECTIVE current_section = DIR_NONE;
    ASM_DIRECTIVE code_type = DIR_CODE_TYPE_32; // default 32-bit

    U32 total_tokens = toks->len;
    U32 index = 0;
    PASM_NODE node;
    while(index < total_tokens) {
        PASM_TOK tok = PEEK_TOK(toks, index);
        switch(tok->type) {
            
            // Dec, Hex, Bin or Char. Should have been handler by parsers by this point, 
            // if not its value will be insterted as is
            case TOK_NUMBER: 

                break; 
            case TOK_IDENT_VAR: 
                ASM_VAR *var = PARSE_VAR(tok, &index, toks, arr);
                if(current_section == DIR_DATA) ADD_VAR(var, &DATA_SECTION);
                else if(current_section == DIR_RODATA) ADD_VAR(var, &RODATA_SECTION);
                else {
                    // Should not happen, just place into data
                    ADD_VAR(var, &DATA_SECTION);
                    if(!args->quiet) printf("[ASM_AST] Warning: variable declared outside data regions @ %d:%d\n", tok->col, tok->line);
                }
                break; // Data sections variable creation
            case TOK_IDENTIFIER: 
            node = PARSE_IDENTIFIER(tok, &index, toks, arr);
                break; // Could be almost anything
            case TOK_SYMBOL: 
                node = PARSE_SYMBOL(tok, &index, toks, arr);
                CONSUME_TOK(arr, node, &index);
                break; // Any keyword symbol
                
            // Any register. Should have been handler by parsers by this point.
            case TOK_REGISTER: break; 
            
            // "" symbol. Should have been handler by parsers by this point.
            case TOK_STRING: break; 

            case TOK_LABEL: break; // Not returned as such. Need to construct from symbols and identifiers
            case TOK_LOCAL_LABEL: break; // Not returned as such. Need to construct from symbols and identifiers 
            case TOK_MNEMONIC: break; // Not returned as such. Need to construct from identifier
            case TOK_DIRECTIVE: break; // Not returned as such. Need to construct from symbols and identifiers
            
            case TOK_EOL: CONSUME_TOK(arr, node, &index); continue;
            case TOK_EOF: CONSUME_TOK(arr, node, &index); break;
            default:
            case TOK_NONE: 
                break;
        }
    }

    
    return arr;
}