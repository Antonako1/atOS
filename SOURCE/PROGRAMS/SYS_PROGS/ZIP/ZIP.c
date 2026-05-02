#include <LIBRARIES/ATZP/ATZP.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <STD/IO.h>
#include <STD/FS_DISK.h>

/*
 * ════════════════════════════════════════════════════════════════════
 *  ATZ Archive Format  (.atz)
 *
 *  [4]  magic        "ATZ1"
 *  [4]  file_count   U32
 *  Per file entry:
 *    [2]  name_len        U16
 *    [?]  name            bytes (relative stored path, no leading /)
 *    [4]  original_size   U32
 *    [4]  compressed_size U32
 *    [4]  data_offset     U32  (absolute byte offset from start of file)
 *    [1]  flags           U8   (bit 0 = 1: LZ4 compressed, 0: stored raw)
 *  Data section (compressed blocks back to back)
 *
 *  Usage:
 *    ZIP zip   <out.atz> <path1> [path2 ...]
 *    ZIP unzip <arch.atz> [dest/]
 *    ZIP list  <arch.atz>
 * ════════════════════════════════════════════════════════════════════
 */

#define ATZ_MAGIC       "ATZ1"
#define ATZ_MAX_FILES   256
#define ATZ_MAX_PATH    512
#define ATZ_FLAG_LZ4    0x01  /* entry is LZ4 compressed */
#define ATZ_HDR_ENTRY_FIXED (2 + 4 + 4 + 4 + 1)  /* name_len + orig + comp + off + flags */

/* ── In-memory entry descriptor ──────────────────────────────────── */
typedef struct {
    U8  stored[ATZ_MAX_PATH];   /* relative name stored in archive     */
    U8  fs_path[ATZ_MAX_PATH];  /* absolute FS path for reading/writing*/
    U32 original_size;
    U32 compressed_size;
    U32 data_offset;
    U8  flags;
    PU8 compressed_data;        /* heap buffer, used during zip only   */
} ATZ_ENTRY;

static ATZ_ENTRY g_entries[ATZ_MAX_FILES];
static U32       g_entry_count;

/* ── Path helpers ─────────────────────────────────────────────────── */

static VOID path_join(PU8 out, U32 out_sz, PU8 base, PU8 name) {
    STRNCPY(out, base, out_sz - 1);
    out[out_sz - 1] = '\0';
    U32 blen = STRLEN(out);
    if (blen > 0 && out[blen - 1] != '/') {
        if (blen + 1 < out_sz) { out[blen] = '/'; out[blen + 1] = '\0'; }
    }
    STRNCAT(out, name, out_sz - STRLEN(out) - 1);
}

/* Ensure all parent directories of path exist (FAT32). */
static VOID ensure_dirs(PU8 path) {
    U8 tmp[ATZ_MAX_PATH];
    STRNCPY(tmp, path, ATZ_MAX_PATH - 1);
    tmp[ATZ_MAX_PATH - 1] = '\0';
    U32 len = STRLEN(tmp);
    U32 i;
    for (i = 1; i < len; i++) {
        if (tmp[i] == '/') {
            tmp[i] = '\0';
            if (!DIR_EXISTS(tmp) && !FILE_EXISTS(tmp))
                DIR_CREATE(tmp);
            tmp[i] = '/';
        }
    }
}

/* ── Recursive file collection ───────────────────────────────────── */

static VOID collect_files(PU8 fs_path, PU8 stored_prefix) {
    if (g_entry_count >= ATZ_MAX_FILES) {
        printf("[ZIP] Warning: max file limit (%d) reached.\n", ATZ_MAX_FILES);
        return;
    }

    if (FILE_EXISTS(fs_path)) {
        /* Plain file - add entry */
        ATZ_ENTRY *e = &g_entries[g_entry_count++];
        STRNCPY((PU8)e->stored,  stored_prefix, ATZ_MAX_PATH - 1);
        STRNCPY((PU8)e->fs_path, fs_path,        ATZ_MAX_PATH - 1);
        e->stored[ATZ_MAX_PATH - 1]  = '\0';
        e->fs_path[ATZ_MAX_PATH - 1] = '\0';
        e->original_size    = 0;
        e->compressed_size  = 0;
        e->data_offset      = 0;
        e->flags            = 0;
        e->compressed_data  = NULLPTR;
        return;
    }

    if (DIR_EXISTS(fs_path)) {
        /* Directory - enumerate children and recurse */
        FAT_LFN_ENTRY dir_ent;
        U32 dir_cluster;

        if (STRCMP(fs_path, "/") == 0) {
            dir_cluster = FAT32_GET_ROOT_CLUSTER();
        } else {
            if (!FAT32_PATH_RESOLVE_ENTRY((PU8)fs_path, &dir_ent)) {
                printf("[ZIP] Warning: cannot open dir '%s'\n", fs_path);
                return;
            }
            dir_cluster = ((U32)dir_ent.entry.HIGH_CLUSTER_BITS << 16) |
                           (U32)dir_ent.entry.LOW_CLUSTER_BITS;
        }

        FAT_LFN_ENTRY children[MAX_CHILD_ENTIES];
        U32 child_count = MAX_CHILD_ENTIES;
        if (!FAT32_DIR_ENUMERATE_LFN(dir_cluster, children, &child_count))
            return;

        U32 ci;
        for (ci = 0; ci < child_count; ci++) {
            FAT_LFN_ENTRY *ch = &children[ci];
            if (FAT32_DIR_ENTRY_IS_FREE(&ch->entry)) continue;
            if (ch->lfn[0] == '\0') continue;
            if (ch->lfn[0] == '.')  continue; /* skip . and .. */

            U8 child_fs[ATZ_MAX_PATH];
            U8 child_stored[ATZ_MAX_PATH];
            path_join(child_fs,     ATZ_MAX_PATH, fs_path,        (PU8)ch->lfn);
            path_join(child_stored, ATZ_MAX_PATH, stored_prefix,  (PU8)ch->lfn);
            collect_files(child_fs, child_stored);
        }
        return;
    }

    printf("[ZIP] Warning: '%s' not found, skipping.\n", fs_path);
}

/* ── ZIP command ─────────────────────────────────────────────────── */

static U32 cmd_zip(U32 argc, PPU8 argv) {
    if (argc < 3) {
        printf("Usage: ZIP zip <out.atz> <path1> [path2 ...]\n");
        return 1;
    }

    PU8 out_path = argv[2];
    g_entry_count = 0;
    U32 fi;

    /* Collect all input paths */
    U32 a;
    for (a = 3; a < argc; a++) {
        PU8 fspath = argv[a];
        /* Stored name = last path component */
        PU8 sep = STRRCHR(fspath, '/');
        PU8 stored_root = sep ? sep + 1 : fspath;
        collect_files(fspath, stored_root);
    }

    if (g_entry_count == 0) {
        printf("[ZIP] No files found.\n");
        return 1;
    }

    /* Compress each file */
    for (fi = 0; fi < g_entry_count; fi++) {
        ATZ_ENTRY *e = &g_entries[fi];

        FILE *f = FOPEN((PU8)e->fs_path, MODE_FR);
        if (!f) {
            printf("[ZIP] Error: cannot open '%s'\n", e->fs_path);
            goto zip_cleanup;
        }

        U32 orig = FSIZE(f);
        e->original_size = orig;

        PU8 raw = (PU8)MAlloc(orig ? orig : 1);
        if (!raw) { FCLOSE(f); goto zip_cleanup; }
        if (orig) FREAD(f, raw, orig);
        FCLOSE(f);

        U32 bound = LZ4_COMPRESS_BOUND(orig);
        PU8 cmp   = (PU8)MAlloc(bound);
        if (!cmp) { MFree(raw); goto zip_cleanup; }

        U32 cmp_sz = 0;
        if (orig > 0) cmp_sz = LZ4_COMPRESS(raw, orig, cmp, bound);
        MFree(raw);

        if (cmp_sz > 0 && cmp_sz < orig) {
            /* Compressed is smaller - store LZ4 */
            e->compressed_size = cmp_sz;
            e->flags           = ATZ_FLAG_LZ4;
            e->compressed_data = cmp;
        } else {
            /* Store raw (empty file, or compression expanded data) */
            MFree(cmp);
            e->compressed_size = orig;
            e->flags           = 0;
            e->compressed_data = NULLPTR;

            if (orig > 0) {
                /* Re-read raw for storage */
                FILE *f2 = FOPEN((PU8)e->fs_path, MODE_FR);
                if (!f2) goto zip_cleanup;
                PU8 raw2 = (PU8)MAlloc(orig);
                if (!raw2) { FCLOSE(f2); goto zip_cleanup; }
                FREAD(f2, raw2, orig);
                FCLOSE(f2);
                e->compressed_data = raw2;
            }
        }
    }

    /* Calculate header size and assign data offsets */
    U32 header_sz = 4 + 4; /* magic + count */
    for (fi = 0; fi < g_entry_count; fi++) {
        U16 nlen  = (U16)STRLEN(g_entries[fi].stored);
        header_sz += ATZ_HDR_ENTRY_FIXED + nlen;
    }
    U32 data_off = header_sz;
    for (fi = 0; fi < g_entry_count; fi++) {
        g_entries[fi].data_offset = data_off;
        data_off += g_entries[fi].compressed_size;
    }

    /* Write archive */
    if (FILE_EXISTS(out_path)) FILE_DELETE(out_path);
    if (!FILE_CREATE(out_path)) {
        printf("[ZIP] Error: cannot create '%s'\n", out_path);
        goto zip_cleanup;
    }
    FILE *out = FOPEN(out_path, MODE_FA);
    if (!out) {
        printf("[ZIP] Error: cannot open output '%s'\n", out_path);
        goto zip_cleanup;
    }

    /* Header */
    FWRITE(out, (PU8)ATZ_MAGIC,       4);
    FWRITE(out, (PU8)&g_entry_count,  4);
    for (fi = 0; fi < g_entry_count; fi++) {
        ATZ_ENTRY *e = &g_entries[fi];
        U16 nlen = (U16)STRLEN(e->stored);
        FWRITE(out, (PU8)&nlen,              2);
        FWRITE(out, (PU8)e->stored,          nlen);
        FWRITE(out, (PU8)&e->original_size,  4);
        FWRITE(out, (PU8)&e->compressed_size,4);
        FWRITE(out, (PU8)&e->data_offset,    4);
        FWRITE(out, (PU8)&e->flags,          1);
    }

    /* Data blocks */
    for (fi = 0; fi < g_entry_count; fi++) {
        ATZ_ENTRY *e = &g_entries[fi];
        if (e->compressed_size > 0 && e->compressed_data)
            FWRITE(out, e->compressed_data, e->compressed_size);
    }
    FCLOSE(out);

    /* Summary */
    U32 total_orig = 0, total_comp = 0;
    for (fi = 0; fi < g_entry_count; fi++) {
        total_orig += g_entries[fi].original_size;
        total_comp += g_entries[fi].compressed_size;
    }
    printf("[ZIP] Created '%s' - %u file(s), %u -> %u bytes",
           out_path, g_entry_count, total_orig, total_comp);
    if (total_orig > 0)
        printf(" (%u%% of original)", (total_comp * 100) / total_orig);
    printf("\n");

zip_cleanup:
    for (fi = 0; fi < g_entry_count; fi++)
        if (g_entries[fi].compressed_data) {
            MFree(g_entries[fi].compressed_data);
            g_entries[fi].compressed_data = NULLPTR;
        }
    return 0;
}

/* ── UNZIP command ───────────────────────────────────────────────── */

static U32 cmd_unzip(U32 argc, PPU8 argv) {
    if (argc < 3) {
        printf("Usage: ZIP unzip <arch.atz> [dest/]\n");
        return 1;
    }

    PU8 arch_path = argv[2];
    U8  dest[ATZ_MAX_PATH];
    if (argc >= 4) {
        STRNCPY(dest, argv[3], ATZ_MAX_PATH - 1);
        dest[ATZ_MAX_PATH - 1] = '\0';
        /* Strip trailing slash */
        U32 dlen = STRLEN(dest);
        if (dlen > 1 && dest[dlen - 1] == '/') dest[dlen - 1] = '\0';
    } else {
        dest[0] = '\0'; /* extract in place (no prefix) */
    }

    FILE *arch = FOPEN(arch_path, MODE_FR);
    if (!arch) {
        printf("[ZIP] Error: cannot open '%s'\n", arch_path);
        return 1;
    }

    /* Verify magic */
    U8 magic[4];
    FREAD(arch, magic, 4);
    if (magic[0] != 'A' || magic[1] != 'T' || magic[2] != 'Z' || magic[3] != '1') {
        printf("[ZIP] Error: '%s' is not a valid ATZ archive.\n", arch_path);
        FCLOSE(arch);
        return 1;
    }

    U32 file_count = 0;
    FREAD(arch, (PU8)&file_count, 4);
    printf("[ZIP] Extracting %u file(s) from '%s'...\n", file_count, arch_path);

    /* Read entry headers into memory */
    ATZ_ENTRY *entries = (ATZ_ENTRY *)MAlloc(file_count * sizeof(ATZ_ENTRY));
    if (!entries) {
        printf("[ZIP] Error: out of memory\n");
        FCLOSE(arch);
        return 1;
    }

    U32 fi;
    for (fi = 0; fi < file_count; fi++) {
        ATZ_ENTRY *e = &entries[fi];
        U16 nlen = 0;
        FREAD(arch, (PU8)&nlen, 2);
        if (nlen >= ATZ_MAX_PATH) nlen = ATZ_MAX_PATH - 1;
        FREAD(arch, (PU8)e->stored, nlen);
        e->stored[nlen] = '\0';
        FREAD(arch, (PU8)&e->original_size,   4);
        FREAD(arch, (PU8)&e->compressed_size, 4);
        FREAD(arch, (PU8)&e->data_offset,     4);
        FREAD(arch, (PU8)&e->flags,           1);
        e->compressed_data = NULLPTR;
    }

    /* Extract each file */
    for (fi = 0; fi < file_count; fi++) {
        ATZ_ENTRY *e = &entries[fi];

        /* Build output path */
        U8 out_path[ATZ_MAX_PATH];
        if (dest[0] != '\0')
            path_join(out_path, ATZ_MAX_PATH, dest, (PU8)e->stored);
        else
            STRNCPY(out_path, (PU8)e->stored, ATZ_MAX_PATH - 1);
        out_path[ATZ_MAX_PATH - 1] = '\0';

        /* Seek to compressed data */
        if (!FSEEK(arch, e->data_offset)) {
            printf("[ZIP] Error: seek failed for '%s'\n", e->stored);
            continue;
        }

        /* Read compressed data */
        PU8 cmp_buf = NULLPTR;
        if (e->compressed_size > 0) {
            cmp_buf = (PU8)MAlloc(e->compressed_size);
            if (!cmp_buf) {
                printf("[ZIP] Error: out of memory for '%s'\n", e->stored);
                continue;
            }
            FREAD(arch, cmp_buf, e->compressed_size);
        }

        /* Decompress or copy */
        PU8 out_data = NULLPTR;
        U32 out_sz   = 0;

        if (e->original_size == 0) {
            /* Empty file */
            out_data = NULLPTR;
            out_sz   = 0;
        } else if (e->flags & ATZ_FLAG_LZ4) {
            out_data = (PU8)MAlloc(e->original_size);
            if (!out_data) {
                printf("[ZIP] Error: out of memory for '%s'\n", e->stored);
                if (cmp_buf) MFree(cmp_buf);
                continue;
            }
            out_sz = LZ4_DECOMPRESS(cmp_buf, e->compressed_size,
                                    out_data, e->original_size);
            if (out_sz == 0 && e->original_size > 0) {
                printf("[ZIP] Error: decompression failed for '%s'\n", e->stored);
                MFree(out_data);
                if (cmp_buf) MFree(cmp_buf);
                continue;
            }
        } else {
            /* Stored raw */
            out_data = cmp_buf;
            out_sz   = e->compressed_size;
            cmp_buf  = NULLPTR; /* transferred ownership */
        }

        /* Ensure parent directories exist */
        PU8 last_sep = STRRCHR(out_path, '/');
        if (last_sep) {
            U8 parent[ATZ_MAX_PATH];
            U32 plen = (U32)(last_sep - out_path);
            STRNCPY(parent, out_path, plen);
            parent[plen] = '\0';
            ensure_dirs(parent);
            if (!DIR_EXISTS(parent)) DIR_CREATE(parent);
        }

        /* Write output file */
        if (FILE_EXISTS(out_path)) FILE_DELETE(out_path);
        FILE_CREATE(out_path);
        if (out_sz > 0 && out_data) {
            FILE *wf = FOPEN(out_path, MODE_FW);
            if (wf) {
                FWRITE(wf, out_data, out_sz);
                FCLOSE(wf);
            } else {
                printf("[ZIP] Error: cannot write '%s'\n", out_path);
            }
        }

        printf("  %s (%u bytes)\n", out_path, e->original_size);

        if (out_data) MFree(out_data);
        if (cmp_buf)  MFree(cmp_buf);
    }

    FCLOSE(arch);
    MFree(entries);
    printf("[ZIP] Done.\n");
    return 0;
}

/* ── LIST command ────────────────────────────────────────────────── */

static U32 cmd_list(U32 argc, PPU8 argv) {
    if (argc < 3) {
        printf("Usage: ZIP list <arch.atz>\n");
        return 1;
    }

    FILE *arch = FOPEN(argv[2], MODE_FR);
    if (!arch) {
        printf("[ZIP] Error: cannot open '%s'\n", argv[2]);
        return 1;
    }

    U8 magic[4];
    FREAD(arch, magic, 4);
    if (magic[0] != 'A' || magic[1] != 'T' || magic[2] != 'Z' || magic[3] != '1') {
        printf("[ZIP] Error: not a valid ATZ archive.\n");
        FCLOSE(arch);
        return 1;
    }

    U32 file_count = 0;
    FREAD(arch, (PU8)&file_count, 4);

    printf("  %-40s  %10s  %10s  %s\n", "Name", "Original", "Stored", "Method");
    printf("  %-40s  %10s  %10s  %s\n", "----", "--------", "------", "------");

    U32 total_orig = 0, total_comp = 0;
    U32 fi;
    for (fi = 0; fi < file_count; fi++) {
        U8  name[ATZ_MAX_PATH];
        U32 orig, comp, off;
        U8  flags;
        U16 nlen = 0;

        FREAD(arch, (PU8)&nlen, 2);
        if (nlen >= ATZ_MAX_PATH) nlen = ATZ_MAX_PATH - 1;
        FREAD(arch, name, nlen);
        name[nlen] = '\0';
        FREAD(arch, (PU8)&orig,  4);
        FREAD(arch, (PU8)&comp,  4);
        FREAD(arch, (PU8)&off,   4);
        FREAD(arch, (PU8)&flags, 1);

        PU8 method = (flags & ATZ_FLAG_LZ4) ? "lz4" : "store";

        /* Truncate name display if too long */
        U8 disp[41];
        U32 nlen32 = STRLEN(name);
        if (nlen32 > 40) {
            STRNCPY(disp, name, 37);
            disp[37] = '.'; disp[38] = '.'; disp[39] = '.'; disp[40] = '\0';
        } else {
            STRCPY(disp, name);
        }

        printf("  %-40s  %10u  %10u  %s\n", disp, orig, comp, method);
        total_orig += orig;
        total_comp += comp;
    }

    printf("  %-40s  %10s  %10s\n", "----", "--------", "------");
    printf("  %-40s  %10u  %10u", (PU8)"-", total_orig, total_comp);
    if (total_orig > 0)
        printf("  (%u%%)", (total_comp * 100) / total_orig);
    printf("\n  %u file(s)\n", file_count);

    FCLOSE(arch);
    return 0;
}

/* ── Help ────────────────────────────────────────────────────────── */

static VOID print_help(VOID) {
    printf("ZIP - ATZ archive utility (LZ4 compression)\n\n");
    printf("  ZIP zip   <out.atz> <path1> [path2 ...]   Create archive\n");
    printf("  ZIP unzip <arch.atz> [dest/]               Extract archive\n");
    printf("  ZIP list  <arch.atz>                       List contents\n");
    printf("  ZIP --help                                  Show this help\n\n");
    printf("  Directories are added recursively.\n");
    printf("  Files are compressed with LZ4; stored raw if compression\n");
    printf("  does not reduce size.\n");
}

/* ── Entry point ─────────────────────────────────────────────────── */

U32 main(U32 argc, PPU8 argv) {
    if (argc < 2 || STRCMP(argv[1], "--help") == 0 || STRCMP(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }

    if (STRCMP(argv[1], "zip") == 0)   return cmd_zip(argc, argv);
    if (STRCMP(argv[1], "unzip") == 0) return cmd_unzip(argc, argv);
    if (STRCMP(argv[1], "list") == 0)  return cmd_list(argc, argv);

    printf("[ZIP] Unknown command '%s'. Run ZIP --help for usage.\n", argv[1]);
    return 1;
}
