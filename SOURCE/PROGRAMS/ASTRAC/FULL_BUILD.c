/*
 * FULL_BUILD.c — Orchestrates the full C -> ASM -> BIN pipeline.
 *
 * Invoked when the user runs:   ASTRAC.BIN file.C -B
 *
 * Pipeline:
 *   1. Preprocess the C source     (shared preprocessor)
 *   2. Compile C -> ASM            (COMPILER/)
 *   3. Preprocess the ASM output   (shared preprocessor)
 *   4. Assemble ASM -> BIN         (ASSEMBLER/)
 *
 * Each step feeds its output into the next via temporary files in /TMP/.
 *
 * TODO: implement once the compiler front-end is functional.
 */

#include <PROGRAMS/ASTRAC/ASTRAC.h>
