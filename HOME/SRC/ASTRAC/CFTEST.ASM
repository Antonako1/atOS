; ═══════════════════════════════════════════════════════════════════════════════
;  VBR.ASM — ASTRAC Assembler Comprehensive Feature Test
; ═══════════════════════════════════════════════════════════════════════════════
;  This file exercises every language feature:
;    Preprocessor, directives, labels, data variables, all operand forms,
;    all instruction categories, all register classes, literals, expressions,
;    size prefixes, segment overrides, local labels, and fill directives.
; ═══════════════════════════════════════════════════════════════════════════════


; ─────────────────────────────────────────────────────────────────────────────
;  1. PREPROCESSOR
; ─────────────────────────────────────────────────────────────────────────────

#include <TEST.INC>

#define ORIGIN      0x7C00
#define SCREEN_BASE 0xB8000
#define VGA_ATTR    0x0F
#define SECTOR_SZ   512
#define SIG_BYTE1   0x55
#define SIG_BYTE2   0xAA
#define BOOT_SIG    0xAA55
#define ONE         1
#define ZERO        0

; Conditional compilation
#ifdef ORIGIN
#define HAS_ORIGIN 1
#endif

#ifndef RELEASE
#warning "Building VBR.ASM in debug/test mode"
#endif

#ifdef NONEXISTENT_MACRO
#error "This should never trigger"
#elif
#define FALLTHROUGH_OK 1
#else
#define UNREACHABLE 1
#endif

#undef UNREACHABLE


; ─────────────────────────────────────────────────────────────────────────────
;  2. 16-BIT MODE + ORIGIN DIRECTIVE
; ─────────────────────────────────────────────────────────────────────────────

.use16
.org ORIGIN

; ─────────────────────────────────────────────────────────────────────────────
;  3. 16-BIT CODE — segment overrides, 16-bit registers
; ─────────────────────────────────────────────────────────────────────────────

.code
real_mode_entry:
    CLI
    CLD

    ; Segment override addressing
    JMP [ES:DI]
    MOV AX, [CS:BX]
    MOV [SS:BP], SP
    MOV [DS:SI], AX
    MOV AL, [FS:DI]
    MOV AH, [GS:SI]

    ; 16-bit register pairs
    MOV AX, BX
    MOV CX, DX
    MOV SI, DI
    MOV SP, BP

    ; 8-bit registers
    MOV AL, BL
    MOV AH, BH
    MOV CL, CH
    MOV DL, DH

    STI
    HLT
    JMP _start

; ─────────────────────────────────────────────────────────────────────────────
;  4. 32-BIT MODE
; ─────────────────────────────────────────────────────────────────────────────

.use32


; ─────────────────────────────────────────────────────────────────────────────
;  5. DATA SECTIONS — all variable types and literal forms
; ─────────────────────────────────────────────────────────────────────────────

; --- Mutable data ---
.data

; DB / BYTE — strings with escape sequences
greeting    DB "Hello, world!", 0
escaped     DB "Tab:\there\nNewline\r\0", 0
quoted      DB "She said \"hi\"", 0
backslash   DB "C:\\path\\file", 0
empty_str   DB "", 0

; DB — numeric values and lists
byte_val    DB 42
byte_hex    DB 0xFF
byte_bin    DB 0b10101010
byte_char   DB 'A'
byte_esc    DB '\n'
byte_zero   DB 0
byte_list   DB 1, 2, 3, 4, 5, 0x0A, 0b11, 'Z', 0

; DW / WORD — 16-bit values
word_val    DW 1234
word_hex    DW 0xABCD
word_bin    DW 0b1111000011110000
word_list   DW 0x1000, 0x2000, 0x3000, 0x4000
word_neg    DW -1

; DD / DWORD — 32-bit values
dword_val   DD 123456
dword_hex   DD 0xDEADBEEF
dword_bin   DD 0b11111111000000001111111100000000
dword_addr  DD SCREEN_BASE
dword_list  DD 0x100, 0x200, 0x300

; REAL4 — floating point
pi_val      REAL4 3.14
neg_float   REAL4 -2.718
zero_float  REAL4 0.0

; $VARNAME — variable byte-size references
greet_len   DD $greeting
esc_len     DD $escaped
blist_len   DD $byte_list

; --- Read-only data ---
.rodata
magic       DW 0xAA55
ro_dword    DD 0x12345678
ro_msg      DB "Read-only string", 0
ro_msg_len  DD $ro_msg

NOP
NOP
NOP
NOP
NOP
NOP
NOP
NOP
NOP
NOP

; ─────────────────────────────────────────────────────────────────────────────
;  6. CODE SECTION — instructions by category
; ─────────────────────────────────────────────────────────────────────────────

.code

; ── 6a. Global labels ────────────────────────────────────────────────────────

_start:
main_entry:

; ── 6b. No-operand instructions ──────────────────────────────────────────────

    NOP
    NOP
    CLC
    STC
    CMC
    CLI
    STI
    CLD
    STD
    SAHF
    LAHF
    XLAT
    AAA
    AAS
    DAA
    DAS
    CWDE
    CDQ
    CBW
    WAIT
    PUSHA
    POPA
    PUSHF
    POPF
    INT3
    INTO
    LEAVE

; ── 6c. MOV — every operand form ─────────────────────────────────────────────

    ; reg, reg (all sizes)
    MOV EAX, EBX
    MOV AX, BX
    MOV AL, BL

    ; reg, imm (all sizes + literal formats)
    MOV EAX, 0xDEADBEEF
    MOV EAX, 123456
    MOV EAX, 0b11110000111100001111000011110000
    MOV AX, 0x1234
    MOV AL, 0xFF
    MOV AL, 'X'
    MOV BL, '\t'
    MOV ECX, -1
    MOV EDX, ZERO

    ; reg, mem (base only)
    MOV EAX, [ESI]
    MOV AX, [EDI]
    MOV AL, [EBX]

    ; reg, mem (base + disp8)
    MOV EAX, [ESI + 4]
    MOV EAX, [EBP + 8]
    MOV EAX, [ESP + 12]

    ; reg, mem (base + disp32)
    MOV EAX, [ESI + 0x1000]
    MOV EAX, [EBX + 65536]

    ; reg, mem (base + index)
    MOV EAX, [ESI + EDI]
    MOV EAX, [EBX + ECX]

    ; reg, mem (base + index * scale)
    MOV EAX, [ESI + EDI*1]
    MOV EAX, [ESI + EDI*2]
    MOV EAX, [ESI + EDI*4]
    MOV EAX, [ESI + EDI*8]

    ; reg, mem (base + index * scale + disp)
    MOV EAX, [EBP + ESI*4 + 16]
    MOV EAX, [EBX + ECX*2 + 0x100]
    MOV EAX, [EDI + EAX*8 + 1]

    ; mem, reg
    MOV [ESI], EAX
    MOV [EDI], AX
    MOV [EBX], AL

    ; mem, imm (with size prefix)
    MOV BYTE PTR [EAX], 0
    MOV WORD PTR [EAX], 0
    MOV DWORD PTR [EAX], 0
    MOV BYTE [ECX], 0x55
    MOV DWORD [EDX + 4], SCREEN_BASE

    ; reg, label/symbol
    MOV EAX, _start
    MOV EBX, greeting

    ; mem (symbol name)
    MOV EAX, [greeting]
    MOV EAX, [dword_hex]

; ── 6d. ADD / SUB — arithmetic ───────────────────────────────────────────────

    ; reg, reg
    ADD EAX, EBX
    ADD AX, BX
    ADD AL, BL

    ; reg, imm
    ADD EAX, 100
    ADD AX, 0x10
    ADD AL, 1
    ADD ECX, SECTOR_SZ

    ; mem, reg
    ADD [ESI], EAX
    ADD [EDI], AL

    ; reg, mem
    ADD EAX, [EBX + 4]
    ADD AX, [ESI + EDI*2]

    ; mem, imm (with size prefix)
    ADD BYTE PTR [EAX], 5
    ADD WORD PTR [ESI], 0x100
    ADD DWORD PTR [EDI + 8], 0x1000

    ; SUB — same patterns
    SUB EAX, EBX
    SUB ECX, 1
    SUB BYTE PTR [EAX], 5
    SUB EAX, [ESI + ECX*4 + 100]

; ── 6e. AND / OR / XOR — bitwise logic ───────────────────────────────────────

    AND EAX, 0xFF
    AND AX, 0x00FF
    AND AL, 0x0F
    AND EAX, EBX
    AND [ESI], EAX

    OR EAX, 1
    OR AX, 0x8000
    OR AL, 0x80
    OR EAX, ECX
    OR DWORD PTR [EDI], 0xFF00

    XOR EAX, EAX
    XOR EBX, ECX
    XOR AL, AL
    XOR BYTE PTR [ESI], 0xAA

; ── 6f. CMP / TEST ──────────────────────────────────────────────────────────

    CMP EAX, 0
    CMP EAX, EBX
    CMP AX, BX
    CMP AL, 'A'
    CMP BYTE PTR [ESI], 0
    CMP DWORD PTR [EDI + 4], SCREEN_BASE

    TEST EAX, EAX
    TEST AL, 0x80
    TEST ECX, 0x01
    TEST BYTE PTR [ESI], 0xFF

; ── 6g. INC / DEC / NOT / NEG — unary ────────────────────────────────────────

    ; Register-in-opcode (INC/DEC)
    INC EAX
    INC EBX
    INC ECX
    INC EDX
    INC ESI
    INC EDI
    INC EBP

    DEC EAX
    DEC ECX

    ; ModR/M form (all four, with size prefix)
    INC BYTE PTR [ESI]
    INC WORD PTR [ESI]
    INC DWORD PTR [ESI]

    DEC BYTE PTR [EDI]
    DEC DWORD PTR [EBX + 4]

    NOT EAX
    NOT ECX
    NOT BYTE PTR [ESI]
    NOT DWORD PTR [EDI + EAX*4]

    NEG EAX
    NEG EDX
    NEG BYTE PTR [EBX]
    NEG WORD PTR [ESI + 8]

; ── 6h. MUL / IMUL / DIV / IDIV — multiply & divide ─────────────────────────

    MUL ECX
    MUL BL
    MUL DWORD PTR [ESI]

    IMUL EDX
    IMUL AL
    IMUL BYTE PTR [EDI]

    DIV ECX
    DIV BL
    DIV WORD PTR [EBX]

    IDIV ESI
    IDIV CL
    IDIV DWORD PTR [EBP + 4]

; ── 6i. Shift & rotate ──────────────────────────────────────────────────────

    ; Shift left
    SHL EAX, 1
    SHL EAX, CL
    SHL EAX, 4
    SHL BYTE PTR [ESI], 1
    SHL DWORD PTR [EDI], CL

    ; Shift right (logical)
    SHR EBX, 1
    SHR EBX, CL
    SHR EBX, 8
    SHR WORD PTR [ESI], 1

    ; Shift right (arithmetic)
    SAR ECX, 1
    SAR ECX, CL
    SAR AX, 4
    SAR BYTE PTR [EDI + 2], CL

    ; Rotate
    ROL EDX, 1
    ROL EDX, CL
    ROL EDX, 16
    ROL BYTE PTR [ESI], 1

    ROR EAX, 1
    ROR EAX, CL
    ROR EAX, 8
    ROR DWORD PTR [EBX + ECX*2], CL

; ── 6j. LEA — load effective address ─────────────────────────────────────────

    LEA EAX, [EBX]
    LEA EAX, [ESI + 100]
    LEA EAX, [EBX + ECX*4]
    LEA EAX, [EBX + ECX*4 + 8]
    LEA EAX, [EBP + EDI*8 + 0x200]
    LEA ESI, [EDI + 1]

; ── 6k. XCHG — exchange ─────────────────────────────────────────────────────

    XCHG EAX, EBX
    XCHG EAX, ECX
    XCHG EAX, EDX
    XCHG EAX, ESI
    XCHG [ESI], AL
    XCHG [EDI], EAX

; ── 6l. PUSH / POP — all forms ──────────────────────────────────────────────

    ; Register-in-opcode
    PUSH EAX
    PUSH EBX
    PUSH ECX
    PUSH EDX
    PUSH ESI
    PUSH EDI
    PUSH EBP
    PUSH ESP

    POP EAX
    POP EBX
    POP ECX
    POP EDX
    POP ESI
    POP EDI
    POP EBP
    POP ESP

    ; Immediate
    PUSH 0
    PUSH 0x1234
    PUSH SCREEN_BASE

    ; Memory (with size prefix)
    PUSH DWORD PTR [ESI]
    POP DWORD PTR [EDI]

; ── 6m. INT — software interrupt ─────────────────────────────────────────────

    INT 0x21
    INT 0x10
    INT 0x13
    INT 0x80
    INT3

; ── 6n. Conditional jumps (short, rel8) ──────────────────────────────────────

@@1:
    JO @@1
    JNO @@1
    JB @@1
    JNB @@1
    JZ @@1
    JNZ @@1
    JBE @@1
    JNBE @@1
    JS @@1
    JNS @@1
    JP @@1
    JNP @@1
    JL @@1
    JNL @@1
    JLE @@1
    JNLE @@1

    JO @f
    JNO @f
    JB @f
    JNB @f
    JZ @f
    JNZ @f
    JBE @f
    JNBE @f
    JS @f
    JNS @f
    JP @f
    JNP @f
    JL @f
    JNL @f
    JLE @f
    JNLE @f
    
    JO @b
    JNO @b
    JB @b
    JNB @b
    JZ @b
    JNZ @b
    JBE @b
    JNBE @b
    JS @b
    JNS @b
    JP @b
    JNP @b
    JL @b
    JNL @b
    JLE @b
    JNLE @b
    

; ── 6o. Unconditional JMP / CALL ─────────────────────────────────────────────

    JMP _start
    JMP main_entry
    CALL _start

    ; Indirect (register / memory)
    JMP EAX
    JMP DWORD PTR [EBX]
    JMP DWORD PTR [ESI + EDI*4]
    CALL ECX
    CALL DWORD PTR [EDI + 8]

; ── 6p. RET variants ─────────────────────────────────────────────────────────

    RET
    RETF

; ── 6q. LOOP instructions ───────────────────────────────────────────────────

@@2:
    LOOP @@2
    LOOPE @@2
    LOOPNE @@2
    JCXZ @@2

; ── 6r. Local label forward / backward references ───────────────────────────

@@3:
    DEC ECX
    JNZ @b
    JMP @f

@@4:
    INC EDX
    JZ @b

; ── 6s. MOVZX / MOVSX — zero/sign-extend ────────────────────────────────────

    MOVZX EAX, AL
    MOVZX EAX, BYTE [ESI]
    MOVZX EAX, AX
    MOVZX EAX, WORD [EDI]

    MOVSX EAX, BL
    MOVSX EAX, BYTE [EBX]
    MOVSX ECX, DL
    MOVSX ECX, WORD [ESI + 4]

; ── 6t. BSF / BSR — bit scan ────────────────────────────────────────────────

    BSF EAX, ECX
    BSF EAX, DWORD [ESI]
    BSR EBX, EDX
    BSR EBX, DWORD [EDI]

; ── 6u. SETcc — set byte on condition ────────────────────────────────────────

    CMP EAX, EBX
    SETO AL
    SETNO BL
    SETB CL
    SETNB DL
    SETZ AL
    SETNZ BL
    SETBE CL
    SETNBE DL
    SETS AL
    SETNS BL
    SETP CL
    SETNP DL
    SETL AL
    SETNL BL
    SETLE CL
    SETNLE DL

; ── 6v. System instructions ──────────────────────────────────────────────────

    CPUID
    RDTSC
    LGDT [EDI]
    LIDT [ESI]
    SGDT [EBX]
    SIDT [ECX]

; ── 6w. BCD adjust (with immediate) ─────────────────────────────────────────

    AAM 10
    AAD 10

; ── 6x. Location counter ($) ────────────────────────────────────────────────

    JMP $

; ── 6y. Macro-expanded immediate ─────────────────────────────────────────────

    MOV EAX, ABC
    ADD EAX, DWORD PTR [ABC]
    MOV ECX, ONE
    MOV EDX, SECTOR_SZ
    MOV EBX, VGA_ATTR

done:
    IRET


; ─────────────────────────────────────────────────────────────────────────────
;  7. RAW NUMBERS IN .code
; ─────────────────────────────────────────────────────────────────────────────

0x55
0xAA
0xAA55
0b00001111
0b11110000
111

; ─────────────────────────────────────────────────────────────────────────────
;  8. FILL DIRECTIVE + BOOT SIGNATURE
; ─────────────────────────────────────────────────────────────────────────────

.times 1512-($-$$) db 0
0xAA55
