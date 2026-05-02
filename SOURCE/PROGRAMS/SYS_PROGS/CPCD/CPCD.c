#include <STD/FS_DISK.h>
#include <STD/IO.h>
#include <STD/STRING.h>
#include <STD/MEM.h>

/* Split a full path into parent dir path and the last component (filename).
 * parent_buf must be at least STRLEN(path)+1 bytes.
 * Returns a pointer into path at the start of the filename, or NULL on error. */
static PU8 split_path(PU8 path, PU8 parent_buf) {
    U32 len = STRLEN(path);
    /* Find the last '/' */
    I32 slash = -1;
    for (I32 i = (I32)len - 1; i >= 0; i--) {
        if (path[i] == '/') { slash = i; break; }
    }
    if (slash < 0) return NULLPTR; /* no slash — cannot determine parent */

    if (slash == 0) {
        /* Parent is root */
        parent_buf[0] = '/';
        parent_buf[1] = '\0';
    } else {
        MEMCPY(parent_buf, path, slash);
        parent_buf[slash] = '\0';
    }
    return path + slash + 1; /* pointer to filename component */
}

/* Resolve a directory path and return its starting cluster, or 0 on error. */
static U32 resolve_dir_cluster(PU8 dir_path) {
    /* Root directory is a special case */
    if (dir_path[0] == '/' && dir_path[1] == '\0') {
        return FAT32_GET_ROOT_CLUSTER();
    }
    FAT_LFN_ENTRY ent;
    MEMZERO(&ent, sizeof(ent));
    if (!FAT32_PATH_RESOLVE_ENTRY(dir_path, &ent)) return 0;
    return ((U32)ent.entry.HIGH_CLUSTER_BITS << 16) | ent.entry.LOW_CLUSTER_BITS;
}

CMAIN() {
    if(argc < 3) {
        printf("Usage: %s <source> <destination>. --help for more information.\n", argv[0]);
        return 1;
    }
    for(U32 i = 1; i < argc; i++) {
        if (STRCMP(argv[i], "-h") == 0 || STRCMP(argv[i], "--help") == 0) {
            printf(
                "CPCD - Copy files from the CD-ROM ISO image to the hard drive.\n\n"
                "CPCD.BIN <SOURCE> <DESTINATION>\n\n"
                "SOURCE: The path inside the CD-ROM ISO image (e.g., /ATOS/CPCD.BIN)\n"
                "DESTINATION: The path on the hard drive where the file should be copied (e.g., /CPCD-COPY.BIN)\n"
            );
            return 0;
        }
    }

    PU8 source_path = argv[1];
    PU8 dest_path   = argv[2];

    FILE *source_file = FOPEN(source_path, MODE_ISO9660 | MODE_R);

    if(!source_file) {
        printf("Failed to open source file: %s\n", source_path);
        return 1;
    }

    printf("[CPCD] Opened source file: %s (size: %u bytes)\n", source_path, source_file->sz);
    printf("[CPCD] Copying to destination: %s\n", dest_path);

    if(ISO9660_IS_DIR(&source_file->ent.iso_ent)) {
        FCLOSE(source_file);
        BOOL ok = DIR_CREATE(dest_path);
        if (!ok) {
            printf("Failed to create directory at destination: %s\n", dest_path);
            return 1;
        }
        printf("[CPCD] Created directory at destination: %s\n", dest_path);
    } else {
        /* Split dest_path into parent directory path + filename */
        U8 parent_path[FAT_MAX_PATH];
        PU8 filename = split_path(dest_path, parent_path);
        if (!filename || filename[0] == '\0') {
            printf("Invalid destination path: %s\n", dest_path);
            FCLOSE(source_file);
            return 1;
        }

        /* Resolve parent directory cluster */
        U32 parent_cluster = resolve_dir_cluster(parent_path);
        if (parent_cluster == 0) {
            printf("Failed to resolve parent directory '%s' for destination: %s\n", parent_path, dest_path);
            FCLOSE(source_file);
            return 1;
        }

        U32 cluster_out = 0;
        BOOL result = FAT32_CREATE_CHILD_FILE(parent_cluster, filename, FAT_ATTRIB_ARCHIVE,
                                              source_file->data, source_file->sz, &cluster_out);
        FCLOSE(source_file);
        if (!result) {
            printf("Failed to create file at destination: %s\n", dest_path);
            return 1;
        }
        printf("[CPCD] Created file at destination: %s (cluster: %u)\n", dest_path, cluster_out);
    }
    return 0;
}