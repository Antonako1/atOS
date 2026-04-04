/*+++
    SOURCE/PROGRAMS/ASTRAC/DISSASEMBLER/DISMAIN.c - Disassembler entry point

    Part of AstraC

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#include "DISSASEMBLER.h"
#include <ASSEMBLER/MNEMONICS.h>
#include <STD/STRING.h> 
#include <STD/IO.h>
#include <STD/MEM.h>

BOOL DISASSEMBLE_FILE(FILE *input, FILE *output, ASTRAC_ARGS *cfg);

ASTRAC_RESULT START_DISSASEMBLER() {
    ASTRAC_ARGS *cfg = GET_ARGS();
    if (!cfg) {
        printf("[DISASM] Internal error: ASTRAC_ARGS is null.\n");
        return ASTRAC_ERR_INTERNAL;
    }

    // Prepare output filename
    U32 strlen = STRLEN(cfg->outfile);
    if (strlen == 0) {
        printf("[DISASM] Internal error: output filename is empty.\n");
        return ASTRAC_ERR_INTERNAL;
    }
    PU8 outfile = MAlloc(strlen + 1);
    if (!outfile) {
        printf("[DISASM] Internal error: failed to allocate memory for output filename.\n");
        return ASTRAC_ERR_INTERNAL;
    }

    PU8 dot = STRRCHR(cfg->outfile, '.');
    if (dot) {
        U32 base_len = dot - cfg->outfile;
        MEMCPY(outfile, cfg->outfile, base_len);
        outfile[base_len] = '\0';
    } else {
        STRCPY(outfile, cfg->outfile);
    }
    STRCAT(outfile, ".DSM");


    // Open output file for writing
    if(FILE_EXISTS(outfile)) {
        if(!FILE_DELETE(outfile)) {
            printf("[DISASM] Internal error: failed to delete existing output file: %s\n", outfile);
            MFree(outfile);
            return ASTRAC_ERR_INTERNAL;
        }
    }
    if(!FILE_CREATE(outfile)) {
        printf("[DISASM] Internal error: failed to create output file: %s\n", outfile);
        MFree(outfile);
        return ASTRAC_ERR_INTERNAL;
    }
    FILE *f = FOPEN(outfile, MODE_FA);
    if (!f) {
        printf("[DISASM] Internal error: failed to open output file: %s\n", outfile);
        MFree(outfile);
        return ASTRAC_ERR_INTERNAL;
    }
    if(!cfg->got_outfile) {
        MFree(cfg->outfile);
    }
    cfg->outfile = STRDUP(outfile);
    MFree(outfile);

    // Open input file for reading
    FILE *input = FOPEN(cfg->input_files[0], MODE_FA);
    if (!input) {
        printf("[DISASM] Error: failed to open input file: %s\n", cfg->input_files[0]);
        FCLOSE(f);
        return ASTRAC_ERR_DISASSEMBLE;
    }

    if (cfg->verbose) {
        printf("[DISASM] Disassembling %s -> %s\n", cfg->input_files[0], cfg->outfile);
    }

    // Perform disassembly
    BOOL success = DISASSEMBLE_FILE(input, f, cfg);
    FCLOSE(input);
    FCLOSE(f);

    if (!success) {
        printf("[DISASM] Error: disassembly failed.\n");
        return ASTRAC_ERR_DISASSEMBLE;
    }

    if (cfg->verbose) {
        printf("[DISASM] Disassembly completed successfully.\n");
    }

    RETURN ASTRAC_OK;
}


/* ── x86 disassembler helpers ────────────────────────────────────────────── */

/* Register name arrays indexed by ModRM reg/rm field (0-7) */
static const CHAR *DSM_REG32[8] = {"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
static const CHAR *DSM_REG16[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
static const CHAR *DSM_REG8[8]  = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

/* 16-bit ModRM r/m effective-address strings (mod != 3) */
static const CHAR *DSM_RM16[8] = {
    "bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx"
};

/*
 * Append a sprintf-formatted string into buf at position pos.
 * Returns the new position (pos + chars written).
 */
static U32 dsm_apnd(U8 *buf, U32 pos, const CHAR *fmt, ...) {
    va_list a;
    va_start(a, fmt);
    CHAR *out = (CHAR*)buf + pos;
    VFORMAT(buffer_putch, &out, (CHAR*)fmt, a);
    *out = '\0';
    va_end(a);
    return (U32)(out - (CHAR*)buf);
}

/*
 * Find the best matching mnemonic table entry.
 *
 *   prefix  — legacy prefix byte (PF_66 / PF_F2 / PF_F3 / PF_NONE)
 *   is0F    — TRUE when the two-byte 0F escape was seen
 *   opcode0 — main opcode byte (0x0F when is0F)
 *   opcode1 — second byte (valid only when is0F)
 *   ext     — ModRM /digit (0-7) when >= 0
 *             -1 to match entries whose modrm_ext == MODRM_NONE
 */
static const ASM_MNEMONIC_TABLE *dsm_find(U8 prefix, BOOL is0F,
                                           U8 opcode0, U8 opcode1, I32 ext) {
    for (U32 i = 1; i < (U32)MNEM_COUNT; i++) {
        const ASM_MNEMONIC_TABLE *m = &asm_mnemonics[i];

        if (m->prefix != (ASM_PREFIX)prefix) continue;

        if (m->opcode_prefix == PFX_0F) {
            if (!is0F)              continue;
            if (m->opcode[1] != opcode1) continue;
        } else {
            if (is0F)               continue;
            if (m->encoding == ENC_REG_OPCODE) {
                /* low 3 bits of opcode encode the register */
                if ((opcode0 & 0xF8) != m->opcode[0]) continue;
            } else {
                if (m->opcode[0] != opcode0) continue;
            }
        }

        /* Match modrm extension digit */
        if (ext >= 0) {
            /* Caller is looking for a specific /digit — skip non-matching */
            if (m->modrm_ext != (ASM_MODRM_EXTENSION)ext) continue;
        } else {
            /* Caller wants only /r (no fixed digit) entries */
            if (m->modrm_ext != MODRM_NONE) continue;
        }

        return m;
    }
    return NULLPTR;
}

/*
 * Decode a 16-bit ModRM memory operand (mod != 3) into buf+pos.
 * Advances *off past any displacement bytes consumed.
 * Returns the new pos.
 */
static U32 dsm_mem_str_16(U8 *buf, U32 pos, const U8 *data, U32 *off,
                           U32 sz, U8 modrm) {
    U8 mod = (modrm >> 6) & 0x3;
    U8 rm  =  modrm       & 0x7;

    buf[pos++] = '[';

    if (mod == 0 && rm == 6) {
        /* [disp16] — absolute address */
        U16 d16 = 0;
        if (*off + 2 <= sz) { MEMCPY(&d16, data + *off, 2); *off += 2; }
        pos = dsm_apnd(buf, pos, "0x%04X", (U32)d16);
    } else {
        pos = dsm_apnd(buf, pos, "%s", DSM_RM16[rm]);

        /* Displacement */
        if (mod == 1) {
            if (*off < sz) {
                I8 d8 = (I8)data[(*off)++];
                if (d8 < 0) pos = dsm_apnd(buf, pos, "-0x%02X", (U32)(-(I32)d8));
                else        pos = dsm_apnd(buf, pos, "+0x%02X", (U32)d8);
            }
        } else if (mod == 2) {
            if (*off + 2 <= sz) {
                I16 d16s = 0;
                MEMCPY(&d16s, data + *off, 2); *off += 2;
                if (d16s < 0) pos = dsm_apnd(buf, pos, "-0x%04X", (U32)(-(I32)d16s));
                else          pos = dsm_apnd(buf, pos, "+0x%04X", (U32)d16s);
            }
        }
    }

    buf[pos++] = ']';
    buf[pos]   = '\0';
    return pos;
}

/*
 * Decode a ModRM memory operand (mod != 3) into buf+pos.
 * Advances *off past any SIB / displacement bytes consumed.
 * Returns the new pos.
 */
static U32 dsm_mem_str(U8 *buf, U32 pos, const U8 *data, U32 *off,
                        U32 sz, U8 modrm) {
    U8 mod = (modrm >> 6) & 0x3;
    U8 rm  =  modrm       & 0x7;

    buf[pos++] = '[';

    if (rm == 4 && mod != 3) {
        /* SIB byte follows */
        if (*off >= sz) { buf[pos++] = '?'; goto done; }
        U8 sib  = data[(*off)++];
        U8 sc   = (sib >> 6) & 0x3;
        U8 idx  = (sib >> 3) & 0x7;
        U8 base = sib & 0x7;

        if (base == 5 && mod == 0) {
            /* disp32 with no base register */
            U32 d32 = 0;
            if (*off + 4 <= sz) { MEMCPY(&d32, data + *off, 4); *off += 4; }
            pos = dsm_apnd(buf, pos, "0x%08X", d32);
        } else {
            pos = dsm_apnd(buf, pos, "%s", DSM_REG32[base]);
        }
        if (idx != 4) {
            pos = dsm_apnd(buf, pos, "+%s*%u", DSM_REG32[idx], 1u << sc);
        }
    } else if (rm == 5 && mod == 0) {
        /* [disp32] — absolute address */
        U32 d32 = 0;
        if (*off + 4 <= sz) { MEMCPY(&d32, data + *off, 4); *off += 4; }
        pos = dsm_apnd(buf, pos, "0x%08X", d32);
        goto done;
    } else {
        pos = dsm_apnd(buf, pos, "%s", DSM_REG32[rm]);
    }

    /* Displacement */
    if (mod == 1) {
        if (*off < sz) {
            I8 d8 = (I8)data[(*off)++];
            if (d8 < 0) pos = dsm_apnd(buf, pos, "-0x%02X", (U32)(-(I32)d8));
            else        pos = dsm_apnd(buf, pos, "+0x%02X", (U32)d8);
        }
    } else if (mod == 2) {
        if (*off + 4 <= sz) {
            I32 d32s = 0;
            MEMCPY(&d32s, data + *off, 4); *off += 4;
            if (d32s < 0) pos = dsm_apnd(buf, pos, "-0x%08X", (U32)(-d32s));
            else          pos = dsm_apnd(buf, pos, "+0x%08X", (U32)d32s);
        }
    }

done:
    buf[pos++] = ']';
    buf[pos]   = '\0';
    return pos;
}

/* ── Main disassembly loop ────────────────────────────────────────────────── */

/*
 * Format: OFFSET  HH HH ... (16 padded)  MNEMONIC OPERANDS (32 padded)  |ascii|
 */
BOOL DISASSEMBLE_FILE(FILE *input, FILE *output, ASTRAC_ARGS *cfg) {
    if (!input || !output || !input->data || input->sz == 0)
        return FALSE;

    const U8 *data = (const U8 *)input->data;
    U32 size = input->sz;
    U32 off  = 0;

    /* Code mode: 16 or 32 bits (affects default operand/address size) */
    BOOL bits16 = (cfg->dsm_bits == 16);

    while (off < size) {
        U32 start = off;

        /* ── Legacy prefix detection ───────────────────────────────────── */
        U8   raw_prefix  = PF_NONE;
        BOOL is0F        = FALSE;
        BOOL addr_override = FALSE;  /* 0x67 address-size override */

        /* Consume all prefix bytes in order */
        for (;;) {
            if (off >= size) break;
            U8 b = data[off];
            if      (b == 0x66) { raw_prefix = PF_66; off++; }
            else if (b == 0xF3) { raw_prefix = PF_F3; off++; }
            else if (b == 0xF2) { raw_prefix = PF_F2; off++; }
            else if (b == 0x67) { addr_override = TRUE; off++; }
            else break;
        }

        if (off >= size) break;

        /* ── Two-byte escape ───────────────────────────────────────────── */
        U8 opcode0, opcode1 = 0;
        if (data[off] == 0x0F) {
            is0F    = TRUE;
            opcode0 = 0x0F;
            off++;
            if (off >= size) break;
            opcode1 = data[off++];
        } else {
            opcode0 = data[off++];
        }

        /* ── Compute table prefix ──────────────────────────────────────
         *
         * In the mnemonic table, 16-bit entries carry PF_66 and 32-bit
         * entries carry PF_NONE.  This matches what the CPU does in
         * 32-bit mode, where 0x66 selects the non-default (16-bit) size.
         *
         * In 16-bit mode the meaning is inverted:
         *   no prefix in binary → 16-bit operand → match PF_66 entries
         *   0x66 in binary      → 32-bit operand → match PF_NONE entries
         *
         * F2/F3 (REP/REPNE) are mode-independent.
         */
        U8 tbl_prefix = raw_prefix;
        if (bits16) {
            if (raw_prefix == PF_NONE) tbl_prefix = PF_66;
            else if (raw_prefix == PF_66) tbl_prefix = PF_NONE;
        }

        /* ── Effective address size ─────────────────────────────────────
         * bits16 XOR addr_override → actual addressing mode
         *   16-bit mode + no 0x67 → 16-bit addressing
         *   16-bit mode + 0x67    → 32-bit addressing
         *   32-bit mode + no 0x67 → 32-bit addressing
         *   32-bit mode + 0x67    → 16-bit addressing
         */
        BOOL addr16 = (bits16 != addr_override);

        /* ── Peek at ModRM for /digit extension lookup ─────────────────── */
        I32 ext_digit = (off < size) ? (I32)((data[off] >> 3) & 0x7) : -1;

        /* ── Mnemonic lookup ───────────────────────────────────────────
         * Try /digit match first, then /r.
         * When in 16-bit mode with inverted prefix (PF_66), also fall
         * back to PF_NONE to catch SZ_8BIT and SZ_NONE entries that
         * don't change with operand-size override.
         */
        const ASM_MNEMONIC_TABLE *m = NULLPTR;
        if (ext_digit >= 0)
            m = dsm_find(tbl_prefix, is0F, opcode0, opcode1, ext_digit);
        if (!m)
            m = dsm_find(tbl_prefix, is0F, opcode0, opcode1, -1);

        /* Fall back: in 16-bit mode the inverted prefix may have
         * missed SZ_8BIT / SZ_NONE entries which always use PF_NONE */
        if (!m && bits16 && tbl_prefix == PF_66) {
            if (ext_digit >= 0)
                m = dsm_find(PF_NONE, is0F, opcode0, opcode1, ext_digit);
            if (!m)
                m = dsm_find(PF_NONE, is0F, opcode0, opcode1, -1);
        }

        /* ── Decode instruction into instr_buf ─────────────────────────── */
        U8  instr_buf[96] = {0};
        U32 ilen = 0;

        if (!m) {
            /* Unknown byte — emit raw db */
            ilen = dsm_apnd(instr_buf, 0, "db 0x%02X", opcode0);
        } else {
            /* Consume ModRM byte if needed */
            U8 modrm     = 0;
            U8 mod_field = 0, reg_field = 0, rm_field = 0;

            if (m->has_modrm && off < size) {
                modrm     = data[off++];
                mod_field = (modrm >> 6) & 0x3;
                reg_field = (modrm >> 3) & 0x7;
                rm_field  =  modrm       & 0x7;
            }

            /* Mnemonic name */
            ilen = dsm_apnd(instr_buf, 0, "%s", m->name);

            /* Operands */
            if (m->operand_count > OPN_NONE)
                instr_buf[ilen++] = ' ';

            for (U32 op = 0; op < (U32)m->operand_count; op++) {
                if (op > 0) {
                    instr_buf[ilen++] = ',';
                    instr_buf[ilen++] = ' ';
                }

                ASM_OPERAND_TYPE ot   = m->operand[op];
                ASM_OPERAND_SIZE opsz = m->size;

                const CHAR **regnames = (opsz == SZ_8BIT)  ? DSM_REG8  :
                                        (opsz == SZ_16BIT) ? DSM_REG16 :
                                                             DSM_REG32;
                switch (ot) {
                    case OP_REG: {
                        /* For +rd encoding, register comes from low 3 of opcode */
                        U8 ridx = (m->encoding == ENC_REG_OPCODE)
                                  ? (opcode0 & 0x7)
                                  : reg_field;
                        ilen = dsm_apnd(instr_buf, ilen, "%s", regnames[ridx]);
                        break;
                    }
                    case OP_MEM: {
                        if (mod_field == 3) {
                            /* register form */
                            ilen = dsm_apnd(instr_buf, ilen, "%s", regnames[rm_field]);
                        } else if (addr16) {
                            /* 16-bit addressing */
                            ilen = dsm_mem_str_16(instr_buf, ilen, data, &off, size, modrm);
                        } else {
                            /* 32-bit addressing */
                            ilen = dsm_mem_str(instr_buf, ilen, data, &off, size, modrm);
                        }
                        break;
                    }
                    case OP_IMM: {
                        if (m->rel_type == RL_REL8) {
                            if (off < size) {
                                I8  rel8   = (I8)data[off++];
                                U32 target = (U32)((I32)off + (I32)rel8);
                                ilen = dsm_apnd(instr_buf, ilen, "0x%08X", target);
                            }
                        } else if (m->rel_type == RL_REL32) {
                            /*
                             * In 16-bit mode the "rel32" table entry actually
                             * emits a rel16 displacement (unless a 0x66 override
                             * widened it back to 32).
                             */
                            BOOL rel_is_16 = bits16 && (raw_prefix != PF_66);
                            if (rel_is_16) {
                                if (off + 2 <= size) {
                                    I16 rel16 = 0;
                                    MEMCPY(&rel16, data + off, 2); off += 2;
                                    U32 target = (U32)((I32)off + (I32)rel16);
                                    ilen = dsm_apnd(instr_buf, ilen, "0x%04X", target & 0xFFFF);
                                }
                            } else {
                                if (off + 4 <= size) {
                                    I32 rel32 = 0;
                                    MEMCPY(&rel32, data + off, 4); off += 4;
                                    U32 target = (U32)((I32)off + rel32);
                                    ilen = dsm_apnd(instr_buf, ilen, "0x%08X", target);
                                }
                            }
                        } else if (opsz == SZ_8BIT) {
                            if (off < size)
                                ilen = dsm_apnd(instr_buf, ilen, "0x%02X", data[off++]);
                        } else if (opsz == SZ_16BIT) {
                            if (off + 2 <= size) {
                                U16 imm16 = 0;
                                MEMCPY(&imm16, data + off, 2); off += 2;
                                ilen = dsm_apnd(instr_buf, ilen, "0x%04X", (U32)imm16);
                            }
                        } else {
                            if (off + 4 <= size) {
                                U32 imm32 = 0;
                                MEMCPY(&imm32, data + off, 4); off += 4;
                                ilen = dsm_apnd(instr_buf, ilen, "0x%08X", imm32);
                            }
                        }
                        break;
                    }
                    default: break;
                }
            }

            instr_buf[ilen] = '\0';
        }

        /* ── Format and emit output line ───────────────────────────────── */
        /*   OFFSET   HH HH … (16 bytes padded)   MNEMONIC (32 padded)   |ascii|  */
        U8  line[160] = {0};
        U32 ln = 0;
        U32 nbytes = off - start;

        /* Offset */
        ln = dsm_apnd(line, ln, "%08X  ", start);

        /* Hex bytes (padded to 16*3 chars) */
        for (U32 i = 0; i < nbytes && i < 16; i++)
            ln = dsm_apnd(line, ln, "%02X ", data[start + i]);
        for (U32 i = nbytes; i < 16; i++) {
            line[ln++] = ' '; line[ln++] = ' '; line[ln++] = ' ';
        }

        /* Instruction text (left-padded to 32 chars) */
        line[ln++] = ' ';
        MEMCPY(line + ln, instr_buf, ilen); ln += ilen;
        for (U32 i = ilen; i < 32; i++) line[ln++] = ' ';

        /* ASCII representation */
        line[ln++] = ' '; line[ln++] = '|';
        for (U32 i = 0; i < nbytes && i < 16; i++) {
            U8 c = data[start + i];
            line[ln++] = (c >= 0x20 && c < 0x7F) ? c : '.';
        }
        line[ln++] = '|';
        line[ln++] = '\n';
        line[ln]   = '\0';

        FPRINTF(output, "%s", line);
    }

    return TRUE;
}