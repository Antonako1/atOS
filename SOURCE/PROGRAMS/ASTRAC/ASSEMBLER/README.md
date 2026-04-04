# ASTRAC Assembler

The assembler back-end of the AstraC toolchain. Takes preprocessed i386
assembly source and produces flat binary output through a six-stage pipeline.

## Pipeline

```
Source (.ASM)
    │
    ▼
┌──────────────────┐
│ 1. PREPROCESS    │  Shared/PREPROCESS.c
│    #include       │  Resolve macros, conditionals, includes.
│    #define        │  Output → /TMP/XX.ASM
└──────────────────┘
    │
    ▼
┌──────────────────┐
│ 2. LEX           │  LEXER.c
│    Tokenise       │  Flat token stream (ASM_TOK_ARRAY).
└──────────────────┘
    │
    ▼
┌──────────────────┐
│ 3. AST           │  AST.c
│    Parse          │  Tree of ASM_NODE: instructions, labels,
│                   │  data variables, section switches.
└──────────────────┘
    │
    ▼
┌──────────────────┐
│ 4. VERIFY        │  VERIFY_AST.c  (TODO)
│    Semantic       │  Check operand types, label resolution,
│    checks         │  section constraints.
└──────────────────┘
    │
    ▼
┌──────────────────┐
│ 5. OPTIMIZE      │  OPTIMIZE.c  (TODO)
│    Peephole       │  Constant folding, short-jump relaxation.
└──────────────────┘
    │
    ▼
┌──────────────────┐
│ 6. GEN_BINARY    │  GEN.c  (TODO)
│    Code gen       │  Emit machine code → flat .BIN output.
└──────────────────┘
```

Entry point: `START_ASSEMBLING()` in **AMAIN.c**.

## File Map

| File               | Purpose |
|--------------------|---------|
| `AMAIN.c`          | Pipeline orchestrator — calls each stage in order, handles cleanup on failure. |
| `ASSEMBLER.h`      | Central header — all types, enums, structs, OPS_* macros, API declarations. |
| `LEXER.c`          | Tokeniser. Reads preprocessed temp files line-by-line, emits `ASM_TOK` tokens. |
| `AST.c`            | AST builder. Consumes the token stream via `TOK_CURSOR`, produces `ASM_NODE` array. |
| `MNEMONICS.h`      | X-macro header. Includes `MNEMONIC_LIST.h` twice: once for the `ASM_MNEMONIC` enum, once for the `asm_mnemonics[]` lookup table. |
| `MNEMONIC_LIST.h`  | The actual instruction entries (~180 i386 mnemonics) using stepped shortcut macros. |
| `VERIFY_AST.c`     | Semantic verification pass (stub). |
| `OPTIMIZE.c`       | Optimisation pass (stub). |
| `GEN.c`            | Binary code generation (stub). |
| `KEYWORDS.txt`     | Reference list of all recognised keywords, symbols, and literals. |

## Mnemonic X-Macro System

`MNEMONIC_LIST.h` is included **twice** by `MNEMONICS.h`:

- **Pass 1** — each macro expands to just `id,` to build the `ASM_MNEMONIC` enum.
- **Pass 2** — each macro expands to a full `ASM_MNEMONIC_TABLE` struct literal for the `asm_mnemonics[]` array.

### Stepped Shortcut Macros

Instead of one 24-parameter `MNEMONIC()` call, most entries use a shortcut
that fills in sensible defaults and only needs 3–7 parameters:

| Macro          | Args | Use case | Defaults |
|----------------|------|----------|----------|
| `MNEM_NOOPS`   | 3 | No operands, single-byte opcode (NOP, HLT, RET…) | `ENC_DIRECT`, no flags, no ModRM |
| `MNEM_REG_ENC` | 5 | Register encoded in opcode low bits — +rd (PUSH r32, INC r32…) | `ENC_REG_OPCODE`, `OPN_ONE` |
| `MNEM_IMM`     | 6 | Immediate operand, no ModRM (INT, PUSH imm…) | `ENC_IMM` |
| `MNEM_RM`      | 7 | ModRM encoding, sets arithmetic flags (ADD, SUB, CMP…) | `ENC_MODRM`, `has_modrm=TRUE`, `FLG_ARITH` |
| `MNEM_RM_NF`   | 7 | ModRM encoding, no flag side-effects (MOV, PUSH r/m, LEA…) | `ENC_MODRM`, `has_modrm=TRUE`, all flags `FLG_NONE` |
| `MNEM_REL`     | 4 | PC-relative branch (JMP rel8, Jcc short, LOOP…) | `ENC_IMM`, `tested_flags=FLG_ALL` |
| `MNEM_0F`      | 7 | Two-byte 0F-prefixed, ModRM (MOVZX, BSF, SETcc…) | `PFX_0F`, `PROC_80386`, `EX_PREFIX_0F` |
| `MNEM_0F_REL`  | 4 | Two-byte 0F-prefixed branch, rel32 (near Jcc) | `PFX_0F`, `PROC_80386` |
| `MNEMONIC`     | 24 | Full explicit form — all fields specified manually | — |

### Enum Layout

```c
typedef enum _ASM_MNEMONIC {
    MNEM_NONE = 0,      // sentinel (asm_mnemonics[0])
    MNEM_NOP,            // first real instruction
    ...
    MNEM_COUNT,          // total count for bounds checking
} ASM_MNEMONIC;
```

`asm_mnemonics[MNEM_FOO]` gives O(1) direct indexed lookup.

### Adding a New Instruction

1. Choose the right shortcut macro based on encoding.
2. Add one line to `MNEMONIC_LIST.h`:
   ```c
   MNEM_RM(MNEM_ADC_RM32_R32, "adc", 0x11, OPS_RM32_R32, OPN_TWO, SZ_32BIT, MODRM_NONE)
   ```
3. The enum value and table entry are generated automatically.

If no shortcut fits, use the full `MNEMONIC(...)` form.

## Token Types

The lexer emits these token types (defined in `ASSEMBLER.h`):

| Token | Examples | Meaning |
|-------|----------|---------|
| `TOK_DIRECTIVE`  | `.data`, `.use32` | Section/mode directive |
| `TOK_LABEL`      | `main:` | Global label definition |
| `TOK_LOCAL_LABEL` | `@@1:`, `@F` | Local/anonymous label |
| `TOK_MNEMONIC`   | `mov`, `add`, `jmp` | Instruction mnemonic |
| `TOK_REGISTER`   | `eax`, `al`, `cs` | CPU register |
| `TOK_NUMBER`     | `0xFF`, `42`, `'A'` | Numeric/char literal |
| `TOK_STRING`     | `"Hello"` | String literal |
| `TOK_SYMBOL`     | `,`, `[`, `+` | Punctuation/operator |
| `TOK_IDENT_VAR`  | `DB`, `DWORD`, `PTR` | Data type keyword |
| `TOK_IDENTIFIER` | `my_label`, `COUNT` | Generic name |
| `TOK_EOL`        | *(end of line)* | Line boundary |
| `TOK_EOF`        | *(end of file)* | File boundary |

## AST Node Types

| Node | Produced from | Contained in |
|------|---------------|--------------|
| `NODE_SECTION`     | `.data`, `.code`, `.use32` | Any section |
| `NODE_LABEL`       | `main:` | .code |
| `NODE_LOCAL_LABEL`  | `@@1:` | .code |
| `NODE_INSTRUCTION` | `mov eax, 1` | .code |
| `NODE_DATA_VAR`    | `msg DB "Hi", 0` | .data / .rodata |
| `NODE_RAW_NUM`     | Bare number in .code | .code |

## Key Structs

- **`ASM_MNEMONIC_TABLE`** — 26-field struct describing one instruction
  encoding (opcode, operand types, ModRM, flags, processor, etc.).
- **`ASM_TOK`** — One lexer token with type, source text, line/col, and a
  union for the parsed semantic value.
- **`ASM_NODE`** — One AST node with type, line/col, and a union for
  instruction / label / data / section / raw-number payloads.
- **`TOK_CURSOR`** — Lightweight iterator over `ASM_TOK_ARRAY` used by the
  AST builder. Provides `TOK_PEEK`, `TOK_ADVANCE`, `TOK_MATCH`, `TOK_EXPECT`.

## OPS_* Operand Macros

Pre-defined 4-element `ASM_OPERAND_TYPE` arrays used in `MNEMONIC_LIST.h`:

```
OPS_NONE          — no operands
OPS_REG           — single register
OPS_RM8 / RM16 / RM32    — single r/m operand
OPS_IMM8 / IMM16 / IMM32 — single immediate
OPS_REL8 / REL16 / REL32 — single relative offset
OPS_RM8_R8  / RM32_R32   — r/m destination, reg source
OPS_R8_RM8  / R32_RM32   — reg destination, r/m source
OPS_RM8_IMM8 / RM32_IMM8 / RM32_IMM32 — r/m + immediate
OPS_R8_IMM8  / R32_IMM32              — reg + immediate
OPS_REG_MOFFS8..32 / OPS_MOFFS8..32_REG — memory offset forms
OPS_RM16_SEG / RM32_SEG / SEG_RM16 / SEG_RM32 — segment forms
```
