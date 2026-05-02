#include <LIBRARIES/ATHASH/BASE64.h>
#include <LIBRARIES/ATHASH/SHA1.h>
#include <STD/STRING.h>
#include <STD/IO.h>
#include <STD/DEBUG.h>
#include <STD/MEM.h>
#include <STD/FS_DISK.h>

/* ============================================================
   HASH - SHA-1 / Base64 encode-decode utility

   Usage:
     HASH sha1   [--string <text>|--file <path>] [--out <path>]
     HASH base64 encode [--string <text>|--file <path>] [--out <path>]
     HASH base64 decode [--string <text>|--file <path>] [--out <path>]
     HASH --help
   ============================================================ */

/* ---------- helpers ---------------------------------------- */

static VOID print_help(VOID) {
    printf("HASH - SHA-1 / Base64 utility\n");
    printf("\nUsage:\n");
    printf("  HASH sha1   [--string <text>|--file <path>] [--out <path>]\n");
    printf("  HASH base64 encode [--string <text>|--file <path>] [--out <path>]\n");
    printf("  HASH base64 decode [--string <text>|--file <path>] [--out <path>]\n");
    printf("\nNotes:\n");
    printf("  SHA-1 is a one-way hash - only encoding is supported.\n");
    printf("  Output defaults to stdout when --out is not specified.\n");
}

/* Read a file into a heap-allocated buffer. Sets *out_len. Returns NULL on error. */
static PU8 read_file_buf(PU8 path, U32 *out_len) {
    FILE *f = FOPEN(path, MODE_FR);
    if (!f) {
        printf("[HASH] Error: cannot open file '%s'\n", path);
        return NULLPTR;
    }
    U32 sz = FSIZE(f);
    PU8 buf = (PU8)MAlloc(sz + 1);
    if (!buf) {
        FCLOSE(f);
        printf("[HASH] Error: out of memory\n");
        return NULLPTR;
    }
    FREAD(f, buf, sz);
    buf[sz] = '\0';
    FCLOSE(f);
    *out_len = sz;
    return buf;
}

/* Write data to a file, or print to stdout if path is NULL. */
static VOID write_output(PU8 path, PU8 data, U32 len) {
    if (!path) {
        /* stdout - just printf; data is expected to be null-terminated */
        printf("%s\n", data);
        return;
    }
    if (FILE_EXISTS(path)) FILE_DELETE(path);
    FILE_CREATE(path);
    FILE *f = FOPEN(path, MODE_FW);
    if (!f) {
        printf("[HASH] Error: cannot open output file '%s'\n", path);
        return;
    }
    FWRITE(f, data, len);
    FCLOSE(f);
    printf("[HASH] Written %u bytes to %s\n", len, path);
}

/* Convert 20-byte SHA-1 digest to 40-char hex string (+ NUL). */
static VOID digest_to_hex(CONST U8 digest[SHA1_DIGEST_SIZE], U8 out[41]) {
    static CONST U8 hex[] = "0123456789abcdef";
    U32 i;
    for (i = 0; i < SHA1_DIGEST_SIZE; i++) {
        out[i*2]     = hex[(digest[i] >> 4) & 0xF];
        out[i*2 + 1] = hex[digest[i] & 0xF];
    }
    out[40] = '\0';
}

/* ============================================================
   MAIN
   ============================================================ */

U32 main(U32 argc, PPU8 argv) {
    if (argc < 2 || STRCMP(argv[1], "--help") == 0 || STRCMP(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }

    /* --- parse mode --- */
    PU8 mode = argv[1];          /* "sha1" or "base64" */
    PU8 op   = NULLPTR;          /* "encode" / "decode" (base64 only) */
    U32 arg_start = 2;

    BOOL is_sha1   = (STRCMP(mode, "sha1")   == 0);
    BOOL is_base64 = (STRCMP(mode, "base64") == 0);

    if (!is_sha1 && !is_base64) {
        printf("[HASH] Error: unknown mode '%s'. Use sha1 or base64.\n", mode);
        return 1;
    }

    if (is_base64) {
        if (argc < 3) {
            printf("[HASH] Error: base64 requires 'encode' or 'decode' subcommand.\n");
            return 1;
        }
        op = argv[2];
        if (STRCMP(op, "encode") != 0 && STRCMP(op, "decode") != 0) {
            printf("[HASH] Error: unknown base64 operation '%s'. Use encode or decode.\n", op);
            return 1;
        }
        arg_start = 3;
    }

    /* --- parse --string / --file / --out --- */
    PU8 input_string = NULLPTR;
    PU8 input_file   = NULLPTR;
    PU8 output_file  = NULLPTR;

    U32 i;
    for (i = arg_start; i < argc; i++) {
        if (STRCMP(argv[i], "--string") == 0 && i + 1 < argc) {
            input_string = argv[++i];
        } else if (STRCMP(argv[i], "--file") == 0 && i + 1 < argc) {
            input_file = argv[++i];
        } else if (STRCMP(argv[i], "--out") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else {
            printf("[HASH] Warning: unknown argument '%s'\n", argv[i]);
        }
    }

    if (!input_string && !input_file) {
        printf("[HASH] Error: provide --string <text> or --file <path>\n");
        return 1;
    }
    if (input_string && input_file) {
        printf("[HASH] Error: --string and --file are mutually exclusive.\n");
        return 1;
    }

    /* --- load input --- */
    PU8 data     = NULLPTR;
    U32 data_len = 0;
    BOOL allocated = FALSE;

    if (input_string) {
        data     = (PU8)input_string;
        data_len = STRLEN(input_string);
    } else {
        data = read_file_buf(input_file, &data_len);
        if (!data) return 1;
        allocated = TRUE;
    }

    /* --- process --- */
    if (is_sha1) {
        U8 digest[SHA1_DIGEST_SIZE];
        U8 hex[41];
        SHA1_HASH(data, data_len, digest);
        digest_to_hex(digest, hex);
        write_output(output_file, hex, 40);

    } else { /* base64 */
        if (STRCMP(op, "encode") == 0) {
            /* output length: 4 * ceil(n/3) + 1 */
            U32 out_len = 4 * ((data_len + 2) / 3) + 1;
            PU8 out_buf = (PU8)MAlloc(out_len + 1);
            if (!out_buf) {
                printf("[HASH] Error: out of memory\n");
                if (allocated) MFree(data);
                return 1;
            }
            BASE64_ENCODE(data, data_len, out_buf);
            write_output(output_file, out_buf, STRLEN(out_buf));
            MFree(out_buf);

        } else { /* decode */
            /* decoded length <= input_len * 3/4 */
            U32 out_len = data_len * 3 / 4 + 4;
            PU8 out_buf = (PU8)MAlloc(out_len + 1);
            if (!out_buf) {
                printf("[HASH] Error: out of memory\n");
                if (allocated) MFree(data);
                return 1;
            }
            U32 decoded_len = BASE64_DECODE(data, out_buf);
            out_buf[decoded_len] = '\0';
            write_output(output_file, out_buf, decoded_len);
            MFree(out_buf);
        }
    }

    if (allocated) MFree(data);
    return 0;
}
