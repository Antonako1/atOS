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
        Free(p);
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
    Free(tmp);
    return res;
}
U32 FAT32_FIND_FILE_BY_NAME_AND_PARENT(U32 parent, U8 *name) {
    PU8 tmp = MAlloc(STRLEN(name) + 1);
    if(!tmp) return 0;
    STRCPY(tmp, name);
    U32 res = SYSCALL2(SYSCALL_FIND_FILE_BY_NAME_AND_PARENT, parent, tmp);
    Free(tmp);
    return res;
}

BOOLEAN FAT32_FIND_DIR_ENTRY_BY_NAME_AND_PARENT(DIR_ENTRY *out, U32 parent, U8 *name) {
    PU8 tmp = MAlloc(STRLEN(name) + 1);
    if(!tmp) return 0;
    STRCPY(tmp, name);
    DIR_ENTRY *out_tmp = MAlloc(sizeof(DIR_ENTRY));
    if(!out_tmp) {
        Free(tmp);
        return FALSE;
    }
    U32 res = SYSCALL3(SYSCALL_FIND_DIR_ENTRY_BY_NAME_AND_PARENT, out_tmp, parent, tmp);
    if(res) {
        MEMCPY(out, out_tmp, sizeof(DIR_ENTRY));
    }
    Free(out_tmp);
    Free(tmp);
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
        Free(ent_tmp);
        return FALSE;
    }
    PU32 size_out_tmp = MAlloc(sizeof(U32));
    if(!size_out_tmp) {
        Free(ent_tmp);
        Free(lfn_tmp);
        return FALSE;
    }
    U32 res = SYSCALL3(SYSCALL_READ_LFNS, ent_tmp, lfn_tmp, size_out_tmp);
    if(res) {
        MEMCPY(ent, ent_tmp, sizeof(DIR_ENTRY));
        MEMCPY(out, lfn_tmp, sizeof(DIR_ENTRY) * MAX_LFN_COUNT);
        MEMCPY(size_out, size_out_tmp, sizeof(U32));
    }
    Free(size_out_tmp);
    Free(ent_tmp);
    Free(size_out_tmp);
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
        Free(ent_tmp);
        return FALSE;
    }

    U32 res = SYSCALL4(SYSCALL_CREATE_CHILD_DIR, parent_cluster, ent_tmp, (U32)attrib, cluster_out_temp);
    if (res) {
        *cluster_out = *cluster_out_temp;
    }

    Free(ent_tmp);
    Free(cluster_out_temp);
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
        Free(ent_tmp);
        return FALSE;
    }

    U32 res = SYSCALL5(SYSCALL_CREATE_CHILD_FILE, parent_cluster, ent_tmp, (U32)attrib, filedata, filedata_size);
    if (res) {
        *cluster_out = *cluster_out_temp;
    }

    Free(ent_tmp);
    Free(cluster_out_temp);
    return res;
}

BOOL FAT32_DIR_ENUMERATE_LFN(U32 dir_cluster, FAT_LFN_ENTRY *out_entries, U32 *max_count) {
    if (!out_entries || !max_count) return FALSE;

    PU32 max_count_tmp = MAlloc(sizeof(U32));
    if (!max_count_tmp) return FALSE;
    *max_count_tmp = *max_count;

    BOOL res = SYSCALL3(SYSCALL_DIR_ENUMERATE_LFN, dir_cluster, out_entries, max_count_tmp);
    if (res) *max_count = *max_count_tmp;

    Free(max_count_tmp);
    return res;
}

BOOL FAT32_DIR_ENUMERATE(U32 dir_cluster, DIR_ENTRY *out_entries, U32 *max_count) {
    if (!out_entries || !max_count) return FALSE;

    PU32 max_count_tmp = MAlloc(sizeof(U32));
    if (!max_count_tmp) return FALSE;
    *max_count_tmp = *max_count;

    BOOL res = SYSCALL3(SYSCALL_DIR_ENUMERATE, dir_cluster, out_entries, max_count_tmp);
    if (res) *max_count = *max_count_tmp;

    Free(max_count_tmp);
    return res;
}

BOOL FAT32_DIR_REMOVE_ENTRY(DIR_ENTRY *entry, const char *name) {
    if (!entry || !name) return FALSE;
    return SYSCALL2(SYSCALL_DIR_REMOVE_ENTRY, entry, name);
}

VOIDPTR FAT32_READ_FILE_CONTENTS(U32 *size_out, DIR_ENTRY *entry) {
    if (!entry || !size_out) return NULL;

    PU32 size_tmp = MAlloc(sizeof(U32));
    if (!size_tmp) return NULL;

    VOIDPTR buffer = (VOIDPTR)SYSCALL2(SYSCALL_READ_FILE_CONTENTS, entry, size_tmp);
    if (buffer) *size_out = *size_tmp;

    Free(size_tmp);
    return buffer;
}

BOOL FAT32_FILE_WRITE(DIR_ENTRY *entry, const U8 *data, U32 size) {
    if (!entry || !data) return FALSE;
    return SYSCALL3(SYSCALL_FILE_WRITE, entry, data, size);
}

BOOL FAT32_FILE_APPEND(DIR_ENTRY *entry, const U8 *data, U32 size) {
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
    // Free if first byte is 0x00 (never used) or 0xE5 (deleted)
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
        Free(res0);
    }
    return res1;
}