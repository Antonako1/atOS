#define FAT_ONLY_DEFINES
#define ISO9660_ONLY_DEFINES
#include <STD/FS_DISK.h>
#include <CPU/SYSCALL/SYSCALL.h>
#include <STD/MEM.h>
#include <STD/BINARY.h>
#include <STD/STRING.h>

U32 CDROM_READ(U32 lba, U32 sectors, U8 *buf) {
    return SYSCALL3(SYSCALL_CDROM_READ, lba, sectors, buf);
}

IsoDirectoryRecord *READ_ISO9660_FILERECORD(CHAR *path) {
    CHARPTR p = MAlloc(ISO9660_MAX_PATH);
    if (!p) return NULLPTR;
    MEMSET(p, 0, ISO9660_MAX_PATH);
    U32 path_len = STRLEN(path);
    if (path_len >= ISO9660_MAX_PATH) {
        MFree(p);
        return NULLPTR;
    }
    STRCPY(p, path);
    return (IsoDirectoryRecord *)SYSCALL1(SYSCALL_ISO9660_READ_ENTRY, (U32)path);
}
VOIDPTR READ_ISO9660_FILECONTENTS(IsoDirectoryRecord *dir_ptr) {
    if (!dir_ptr) return NULLPTR;
    return (VOIDPTR)SYSCALL1(SYSCALL_ISO9660_FILECONTENTS, (U32)dir_ptr);
}
VOID FREE_ISO9660_MEMORY(VOIDPTR ptr) {
    if (!ptr) return;
    SYSCALL1(SYSCALL_ISO9660_FREE_MEMORY, (U32)ptr);
}

U32 HDD_WRITE(U32 lba, U32 sectors, U8 *buf) {
    return SYSCALL3(SYSCALL_HDD_WRITE, lba, sectors, (U32)buf);
}
U32 HDD_READ(U32 lba, U32 sectors, U8 *buf) {
    return SYSCALL3(SYSCALL_HDD_READ, lba, sectors, (U32)buf);
}







U32 FAT32_GET_ROOT_CLUSTER() {
    return SYSCALL0(SYSCALL_GET_ROOT_CLUSTER);
}
U32 FAT32_FIND_DIR_BY_NAME_AND_PARENT(U32 parent, U8 *name) {
    PU8 tmp = MAlloc(STRLEN(name) + 1);
    if(!tmp) return 0;
    STRCPY(tmp, name);
    U32 res = SYSCALL2(SYSCALL_FIND_DIR_BY_NAME_AND_PARENT, parent, tmp);
    MFree(tmp);
    return res;
}
U32 FAT32_FIND_FILE_BY_NAME_AND_PARENT(U32 parent, U8 *name) {
    PU8 tmp = MAlloc(STRLEN(name) + 1);
    if(!tmp) return 0;
    STRCPY(tmp, name);
    U32 res = SYSCALL2(SYSCALL_FIND_FILE_BY_NAME_AND_PARENT, parent, tmp);
    MFree(tmp);
    return res;
}

BOOLEAN FAT32_FIND_DIR_ENTRY_BY_NAME_AND_PARENT(DIR_ENTRY *out, U32 parent, U8 *name) {
    PU8 tmp = MAlloc(STRLEN(name) + 1);
    if(!tmp) return 0;
    STRCPY(tmp, name);
    DIR_ENTRY *out_tmp = MAlloc(sizeof(DIR_ENTRY));
    if(!out_tmp) {
        MFree(tmp);
        return FALSE;
    }
    U32 res = SYSCALL3(SYSCALL_FIND_DIR_ENTRY_BY_NAME_AND_PARENT, out_tmp, parent, tmp);
    if(res) {
        MEMCPY(out, out_tmp, sizeof(DIR_ENTRY));
    }
    MFree(out_tmp);
    MFree(tmp);
    return res;
}
BOOLEAN FAT32_READ_LFNS(DIR_ENTRY *ent, LFN *out, U32 out_size, U32 *size_out) {
    if(out_size < sizeof(LFN) * MAX_LFN_COUNT) {
        return FALSE;
    }
    DIR_ENTRY *ent_tmp = MAlloc(sizeof(DIR_ENTRY));
    if(!ent_tmp) return FALSE;
    LFN *lfn_tmp = MAlloc(sizeof(LFN) * MAX_LFN_COUNT);
    if(!lfn_tmp) {
        MFree(ent_tmp);
        return FALSE;
    }
    PU32 size_out_tmp = MAlloc(sizeof(U32));
    if(!size_out_tmp) {
        MFree(ent_tmp);
        MFree(lfn_tmp);
        return FALSE;
    }
    U32 res = SYSCALL3(SYSCALL_READ_LFNS, ent_tmp, lfn_tmp, size_out_tmp);
    if(res) {
        MEMCPY(ent, ent_tmp, sizeof(DIR_ENTRY));
        MEMCPY(out, lfn_tmp, sizeof(DIR_ENTRY) * MAX_LFN_COUNT);
        MEMCPY(size_out, size_out_tmp, sizeof(U32));
    }
    MFree(size_out_tmp);
    MFree(ent_tmp);
    MFree(size_out_tmp);
    return res;
}
BOOLEAN FAT32_CREATE_CHILD_DIR(U32 parent_cluster, U8 *name, U8 attrib, U32 *cluster_out) {
    U32 len = STRLEN(name) + 1;
    U8 *ent_tmp = MAlloc(len);
    if (!ent_tmp) return FALSE;

    MEMZERO(ent_tmp, len);
    STRCPY(ent_tmp, name);

    PU32 cluster_out_temp = MAlloc(sizeof(U32));
    if (!cluster_out_temp) {
        MFree(ent_tmp);
        return FALSE;
    }

    U32 res = SYSCALL4(SYSCALL_CREATE_CHILD_DIR, parent_cluster, ent_tmp, (U32)attrib, cluster_out_temp);
    if (res) {
        *cluster_out = *cluster_out_temp;
    }

    MFree(ent_tmp);
    MFree(cluster_out_temp);
    return res;
}

BOOLEAN FAT32_CREATE_CHILD_FILE(U32 parent_cluster, U8 *name, U8 attrib, PU8 filedata, U32 filedata_size, U32 *cluster_out) {
    U32 len = STRLEN(name) + 1;
    U8 *ent_tmp = MAlloc(len);
    if (!ent_tmp) return FALSE;

    MEMZERO(ent_tmp, len);
    STRCPY(ent_tmp, name);

    PU32 cluster_out_temp = MAlloc(sizeof(U32));
    if (!cluster_out_temp) {
        MFree(ent_tmp);
        return FALSE;
    }

    U32 res = SYSCALL5(SYSCALL_CREATE_CHILD_FILE, parent_cluster, ent_tmp, (U32)attrib, filedata, filedata_size);
    if (res) {
        *cluster_out = *cluster_out_temp;
    }

    MFree(ent_tmp);
    MFree(cluster_out_temp);
    return res;
}

BOOL FAT32_DIR_ENUMERATE_LFN(U32 dir_cluster, FAT_LFN_ENTRY *out_entries, U32 *max_count) {
    if (!out_entries || !max_count) return FALSE;

    PU32 max_count_tmp = MAlloc(sizeof(U32));
    if (!max_count_tmp) return FALSE;
    *max_count_tmp = *max_count;

    BOOL res = SYSCALL3(SYSCALL_DIR_ENUMERATE_LFN, dir_cluster, out_entries, max_count_tmp);
    if (res) *max_count = *max_count_tmp;

    MFree(max_count_tmp);
    return res;
}

BOOL FAT32_DIR_ENUMERATE(U32 dir_cluster, DIR_ENTRY *out_entries, U32 *max_count) {
    if (!out_entries || !max_count) return FALSE;

    PU32 max_count_tmp = MAlloc(sizeof(U32));
    if (!max_count_tmp) return FALSE;
    *max_count_tmp = *max_count;

    BOOL res = SYSCALL3(SYSCALL_DIR_ENUMERATE, dir_cluster, out_entries, max_count_tmp);
    if (res) *max_count = *max_count_tmp;

    MFree(max_count_tmp);
    return res;
}

BOOL FAT32_DIR_REMOVE_ENTRY(FAT_LFN_ENTRY *entry, const char *name) {
    if (!entry || !name) return FALSE;
    return SYSCALL2(SYSCALL_DIR_REMOVE_ENTRY, entry, name);
}

VOIDPTR FAT32_READ_FILE_CONTENTS(U32 *size_out, DIR_ENTRY *entry) {
    if (!entry || !size_out) return NULL;

    PU32 size_tmp = MAlloc(sizeof(U32));
    if (!size_tmp) return NULL;

    VOIDPTR buffer = (VOIDPTR)SYSCALL2(SYSCALL_READ_FILE_CONTENTS, entry, size_tmp);
    if (buffer) *size_out = *size_tmp;

    MFree(size_tmp);
    return buffer;
}

BOOL FAT32_FILE_WRITE(FAT_LFN_ENTRY *entry, const U8 *data, U32 size) {
    if (!entry || !data) return FALSE;
    return SYSCALL3(SYSCALL_FILE_WRITE, entry, data, size);
}

BOOL FAT32_FILE_APPEND(FAT_LFN_ENTRY *entry, const U8 *data, U32 size) {
    if (!entry || !data) return FALSE;
    return SYSCALL3(SYSCALL_FILE_APPEND, entry, data, size);
}

BOOLEAN FAT32_PATH_RESOLVE_ENTRY(U8 *path, FAT_LFN_ENTRY *out_entry) {
    if (!path || !out_entry) return FALSE;
    return SYSCALL2(SYSCALL_PATH_RESOLVE_ENTRY, path, out_entry);
}

U32 FAT32_FILE_GET_SIZE(DIR_ENTRY *entry) {
    return entry->FILE_SIZE;
}
BOOL FAT32_DIR_ENTRY_IS_FREE(DIR_ENTRY *entry) {
    if (!entry) return TRUE;
    // MFree if first byte is 0x00 (never used) or 0xE5 (deleted)
    return (entry->FILENAME[0] == 0x00 || entry->FILENAME[0] == 0xE5);
}
BOOL FAT32_DIR_ENTRY_IS_DIR(DIR_ENTRY *entry) {
    if(IS_FLAG_SET(entry->ATTRIB, FAT_ATTRB_DIR)) return TRUE;
    return FALSE;
}

DIR_ENTRY FAT32_GET_ROOT_DIR_ENTRY() {
    DIR_ENTRY *res0 = SYSCALL0(SYSCALL_GET_ROOT_DIR_ENTRY);
    DIR_ENTRY res1 = { 0 };
    if(res0) {
        MEMCPY(&res1, res0, sizeof(DIR_ENTRY));
        MFree(res0);
    }
    return res1;
}





FILE * FOPEN(PU8 path, FILEMODES mode) {
    if (!path) return FALSE;
    FILE *file = MAlloc(sizeof(FILE));
    MEMZERO(file, sizeof(FILE));

    file->mode = mode;
    file->read_ptr = 0;
    file->sz = 0;

    const BOOLEAN iso = (mode & MODE_ISO9660) != 0;
    const BOOLEAN fat = (mode & MODE_FAT32) != 0;

    if (iso) {
        IsoDirectoryRecord *ent = READ_ISO9660_FILERECORD(path);
        if (!ent) goto failure;
        if (ent->fileFlags & ISO9660_FILE_FLAG_DIRECTORY) {
            MFree(ent);
            goto failure;
        }

        MEMCPY(&file->ent.iso_ent, ent, sizeof(IsoDirectoryRecord));
        file->sz = ent->extentLengthLE;
        file->data = READ_ISO9660_FILECONTENTS(ent);

        MFree(ent);
        if (!file->data) goto failure;
        return file;
    }

    if (fat) {
        FAT_LFN_ENTRY ent = {0};
        if (!FAT32_PATH_RESOLVE_ENTRY(path, &ent)) goto failure;
        if (ent.entry.ATTRIB & FAT_ATTRB_DIR) goto failure;

        MEMCPY(&file->ent.fat_ent, &ent, sizeof(FAT_LFN_ENTRY));
        file->sz = ent.entry.FILE_SIZE;

        if (file->sz > 0) {
            file->data = FAT32_READ_FILE_CONTENTS(&file->sz, &ent.entry);
            if (!file->data) goto failure;
        }

        return file;
    }

failure:
    FCLOSE(file);
    return NULLPTR;
}

BOOLEAN FILE_FROM_RAW_FAT_DATA(FILE *file, VOIDPTR data, U32 sz, FAT_LFN_ENTRY *ent) {
    if (!file || !ent) return FALSE;
    file->data = data;
    file->sz = sz;
    file->read_ptr = 0;
    MEMCPY_OPT(&file->ent.fat_ent, ent, sizeof(FAT_LFN_ENTRY));
    file->mode |= MODE_FAT32 | MODE_RW;
    return TRUE;
}

BOOLEAN FILE_FROM_RAW_ISO_DATA(FILE *file, VOIDPTR data, U32 sz, IsoDirectoryRecord *ent) {
    file->data = data;
    file->sz = sz;
    file->read_ptr = 0;
    MEMCPY_OPT(&file->ent.iso_ent, ent, sizeof(IsoDirectoryRecord));
    file->mode |= MODE_ISO9660 | MODE_RW;

}

U32 GET_FULL_CLUSTER(U32 high, U32 low) {
    return (high << 16) | (low & 0xFFFF);
}


VOID FCLOSE(FILE *file) {
    if (!file) return;
    if (file->data) {
        MFree(file->data);
        file->data = NULL;
    }
    file->sz = 0;
    file->mode = 0;
    file->read_ptr = 0;
    MFree(file);
}

U32 FREAD(FILE *file, VOIDPTR buffer, U32 len) {
    if (!file || !buffer || !file->data) return 0;
    if (file->read_ptr >= file->sz) return 0; // EOF

    U32 remaining = file->sz - file->read_ptr;
    if (len > remaining) len = remaining;

    MEMCPY_OPT(buffer, (U8*)file->data + file->read_ptr, len);
    file->read_ptr += len;
    return len;
}

U32 FWRITE(FILE *file, VOIDPTR buffer, U32 len) {
    if (!file || !buffer || len == 0)
        return 0;

    // ISO9660 is read-only
    if (file->mode & MODE_ISO9660)
        return 0;

    // Must be FAT32 backend
    if (!(file->mode & MODE_FAT32))
        return 0;

    U8 *tmp;
    U32 new_size = file->sz;

    if (file->mode & MODE_A) { // Append mode
        new_size += len;

        // Grow in-memory buffer
        tmp = ReAlloc(file->data, new_size);
        if (!tmp)
            return 0;

        // Copy new data at the end
        MEMCPY_OPT(tmp + file->sz, buffer, len);
        file->data = tmp;
        file->sz = new_size;

        // Write combined buffer (existing + new) fully
        if (!FAT32_FILE_WRITE(&file->ent.fat_ent, (PU8)file->data, file->sz))
            return 0;
    }
    else if (file->mode & MODE_W) { // Overwrite mode
        // Replace buffer entirely
        tmp = ReAlloc(file->data, len);
        if (!tmp)
            return 0;

        MEMCPY_OPT(tmp, buffer, len);
        file->data = tmp;
        file->sz = len;

        // Write new data
        if (!FAT32_FILE_WRITE(&file->ent.fat_ent, (PU8)file->data, file->sz))
            return 0;
    }

    return len;
}




BOOLEAN FSEEK(FILE *file, U32 offset) {
    if (!file) return FALSE;
    if (offset > file->sz) return FALSE;
    file->read_ptr = offset;
    return TRUE;
}

U32 FTELL(FILE *file) {
    if (!file) return 0;
    return file->read_ptr;
}

VOID FREWIND(FILE *file) {
    if (file) file->read_ptr = 0;
}

U32 FSIZE(FILE *file) {
    return file ? file->sz : 0;
}

BOOLEAN FILE_EOF(FILE *file) {
    if (!file) return TRUE;
    return file->read_ptr >= file->sz;
}

BOOLEAN FILE_GET_LINE(FILE *file, PU8 line, U32 max_len) {
    if (!file || !line || !file->data || max_len == 0) return FALSE;

    U32 i = 0;
    while (file->read_ptr < file->sz && i < max_len - 1) {
        U8 ch = ((U8*)file->data)[file->read_ptr++];
        line[i++] = ch;
        if (ch == '\n' || ch == '\r') break;
    }

    line[i] = '\0';
    if (i == 0) return FALSE;
    return TRUE;
}



// --- helpers ---------------------------------------------------------------
static const U8 *GET_BASENAME(const U8 *path) {
    if (!path) return (const U8*)"";
    const U8 *p = path;
    const U8 *last = p;
    while (*p) {
        if (*p == '/' || *p == '\\') last = p + 1;
        p++;
    }
    return last;
}

// Copies parent path into out_parent (must be freed by caller). If path has no
// parent (no slash), returns "/" as parent (allocated).
static U8 *GET_PARENT_PATH(const U8 *path) {
    if (!path) return NULL;
    const U8 *sep = NULL;
    const U8 *p = path;
    while (*p) {
        if (*p == '/' || *p == '\\') sep = p;
        p++;
    }
    if (!sep) {
        // root
        U8 *r = MAlloc(2);
        if (!r) return NULL;
        r[0] = '/'; r[1] = 0;
        return r;
    }
    // if sep points to last char (trailing slash), walk back to find previous
    const U8 *end = sep;
    // if path ends with slash, skip trailing slashes
    if (*(sep + 1) == '\0') {
        const U8 *q = sep;
        // walk backwards to find previous slash (or start)
        const U8 *prev = NULL;
        const U8 *s = path;
        while (s < sep) {
            if (*s == '/' || *s == '\\') prev = s;
            s++;
        }
        if (!prev) {
            U8 *r = MAlloc(2);
            if (!r) return NULL;
            r[0] = '/'; r[1] = 0;
            return r;
        }
        end = prev;
    }
    size_t len = (size_t)(end - path);
    if (len == 0) {
        U8 *r = MAlloc(2);
        if (!r) return NULL;
        r[0] = '/'; r[1] = 0;
        return r;
    }
    U8 *out = MAlloc(len + 1);
    if (!out) return NULL;
    MEMCPY(out, path, len);
    out[len] = 0;
    return out;
}

// --- function implementations ----------------------------------------------
BOOLEAN FILE_EXISTS(PU8 path) {
    if (!path) return FALSE;

    // check ISO9660
    IsoDirectoryRecord *iso_ent = READ_ISO9660_FILERECORD((CHAR*)path);
    if (iso_ent) {
        MFree(iso_ent);
        return TRUE;
    }

    // check FAT32
    FAT_LFN_ENTRY ent = {0};
    if (FAT32_PATH_RESOLVE_ENTRY(path, &ent)) {
        return TRUE;
    }
    return FALSE;
}

BOOLEAN DIR_EXISTS(PU8 path) {
    if (!path) return FALSE;

    // ISO9660
    IsoDirectoryRecord *iso_ent = READ_ISO9660_FILERECORD((CHAR*)path);
    if (iso_ent) {
        BOOLEAN is_dir = (iso_ent->fileFlags & ISO9660_FILE_FLAG_DIRECTORY) != 0;
        MFree(iso_ent);
        if (is_dir) return TRUE;
    }

    // FAT32
    FAT_LFN_ENTRY ent = {0};
    if (FAT32_PATH_RESOLVE_ENTRY(path, &ent)) {
        if (IS_FLAG_SET(ent.entry.ATTRIB, FAT_ATTRB_DIR)) return TRUE;
    }

    return FALSE;
}

BOOLEAN FILE_DELETE(PU8 path) {
    if (!path) return FALSE;

    FAT_LFN_ENTRY ent = {0};
    if (!FAT32_PATH_RESOLVE_ENTRY(path, &ent)) return FALSE;

    const U8 *basename = GET_BASENAME(path);
    if (FAT32_DIR_REMOVE_ENTRY(&ent, basename)) return TRUE;

    return FALSE;
}

BOOLEAN DIR_DELETE(PU8 path, BOOLEAN force) {
    if (!path) return FALSE;

    FAT_LFN_ENTRY ent = {0};
    if (!FAT32_PATH_RESOLVE_ENTRY(path, &ent)) return FALSE;

    if (!IS_FLAG_SET(ent.entry.ATTRIB, FAT_ATTRB_DIR))
        return FALSE;
    const U8 *basename = GET_BASENAME(path);
    if (FAT32_DIR_REMOVE_ENTRY(&ent, basename)) return TRUE;

    if (FAT32_DIR_REMOVE_ENTRY(&ent, basename)) return TRUE;

    return FALSE;
}

BOOLEAN FILE_CREATE(PU8 path) {
    if (!path) {
        DEBUG_PUTS_LN("FILE_CREATE: path is NULL");
        return FALSE;
    }

    // Split path into parent and name
    const U8 *name = GET_BASENAME(path);
    U8 *parent = GET_PARENT_PATH(path);
    if (!parent) {
        DEBUG_PUTS_LN("FILE_CREATE: failed to get parent path");
        return FALSE;
    }

    // resolve parent - if parent path is root ("/") we use root cluster; otherwise try to resolve
    U32 parent_cluster = FAT32_GET_ROOT_CLUSTER();
    if (!(parent[0] == '/' && parent[1] == '\0')) {
        FAT_LFN_ENTRY parent_ent = {0};
        if (!FAT32_PATH_RESOLVE_ENTRY(parent, &parent_ent)) {
            MFree(parent);
            return FALSE;
        }
        parent_cluster = GET_FULL_CLUSTER(parent_ent.entry.HIGH_CLUSTER_BITS, parent_ent.entry.LOW_CLUSTER_BITS);
    }

    // create empty file (attrib = archive)
    U32 out_cluster = 0;
    BOOLEAN res = FAT32_CREATE_CHILD_FILE(parent_cluster, (U8*)name, FAT_ATTRIB_ARCHIVE, NULL, 0, &out_cluster);


    MFree(parent);
    return res;
}


BOOLEAN DIR_CREATE(PU8 path) {
    if (!path) return FALSE;

    // ISO9660 read-only -> cannot create
    const U8 *name = GET_BASENAME(path);
    U8 *parent = GET_PARENT_PATH(path);
    if (!parent) return FALSE;

    U32 parent_cluster = FAT32_GET_ROOT_CLUSTER();
    if (!(parent[0] == '/' && parent[1] == '\0')) {
        FAT_LFN_ENTRY parent_ent = {0};
        if (!FAT32_PATH_RESOLVE_ENTRY(parent, &parent_ent)) {
            MFree(parent);
            return FALSE;
        }
        // Same note as FILE_CREATE: real implementation should extract parent cluster.
        parent_cluster = FAT32_GET_ROOT_CLUSTER();
    }

    U32 out_cluster = 0;
    BOOLEAN res = FAT32_CREATE_CHILD_DIR(parent_cluster, (U8*)name, FAT_ATTRB_DIR, &out_cluster);
    MFree(parent);
    return res;
}

BOOLEAN FILE_TRUNCATE(FILE *file, U32 new_size) {
    if (!file) return FALSE;
    if (!(file->mode & MODE_FAT32)) return FALSE;

    U8 *temp = NULL;
    if (new_size > 0) {
        temp = MAlloc(new_size);
        if (!temp) return FALSE;
        MEMZERO(temp, new_size);
        U32 to_copy = (file->sz < new_size) ? file->sz : new_size;
        if (to_copy && file->data)
            MEMCPY(temp, file->data, to_copy);
    }

    if (!FAT32_FILE_WRITE(&file->ent.fat_ent, temp, new_size)) {
        if (temp) MFree(temp);
        return FALSE;
    }

    if (file->data) {
        MFree(file->data);
        file->data = NULL;
    }
    if (new_size > 0) {
        file->data = (VOIDPTR)MAlloc(new_size);
        if (file->data)
            MEMCPY(file->data, temp, new_size);
    }

    file->sz = new_size;
    if (temp) MFree(temp);
    return TRUE;
}

BOOLEAN FILE_FLUSH(FILE *file) {
    if (!file) return FALSE;
    if (!(file->mode & MODE_FAT32)) return FALSE;

    if (file->data && file->sz > 0)
        return FAT32_FILE_WRITE(&file->ent.fat_ent, (const U8*)file->data, file->sz);

    return TRUE;
}

VOID FAT_DECODE_TIME(U16 time, U16 date, U32 *year, U32 *month, U32 *day, U32 *hour, U32 *minute, U32 *second) {
    *hour   = (time >> 11) & 0x1F;
    *minute = (time >> 5)  & 0x3F;
    *second = (time & 0x1F) * 2;
    *year   = 1980 + ((date >> 9) & 0x7F);
    *month  = (date >> 5) & 0x0F;
    *day    = (date & 0x1F);
}