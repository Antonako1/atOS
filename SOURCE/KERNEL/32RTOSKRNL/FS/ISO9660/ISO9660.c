#include "./ISO9660.h"
#include "../MEMORY/MEMORY.h"
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <MEMORY/HEAP/KHEAP.h>
#include <DRIVERS/ATAPI/ATAPI.h>
#include <DRIVERS/VIDEO/VBE.h>
#include <RTOSKRNL/RTOSKRNL_INTERNAL.h>

static PrimaryVolumeDescriptor *pvd = NULLPTR;

void SetGlobalPVD(PrimaryVolumeDescriptor *descriptor) {
    if (!descriptor) return;
    if (pvd && pvd != descriptor) ISO9660_FREE_MEMORY_INTERNAL(&pvd);
    pvd = descriptor;
}

BOOLEAN ISO9660_IMAGE_CHECK(PrimaryVolumeDescriptor *pvd) {
    if (!pvd) return FALSE;
    return STRNCMP(pvd->standardIdentifier, "CD001", 5) == 0;
}

BOOLEAN ISO9660_READ_PVD(PrimaryVolumeDescriptor *descriptor, U32 size) {
    if (!descriptor || size < sizeof(PrimaryVolumeDescriptor)) return FALSE;

    U32 atapiStatus = INITIALIZE_ATAPI();
    if (atapiStatus == ATA_FAILED) return FALSE;
    if (READ_CDROM(atapiStatus, 16, 1, descriptor) == ATA_FAILED) return FALSE;

    return TRUE;
}

CHAR *ISO9660_NORMALIZE_PATH(CHAR *path) {
    if (!path) return NULLPTR;
    U32 len = STRLEN(path);
    if (len == 0 || len >= ISO9660_MAX_PATH) return NULLPTR;

    CHAR *res = KMALLOC(ISO9660_MAX_PATH);
    if (!res) return NULLPTR;
    MEMZERO(res, ISO9660_MAX_PATH);
    U32 filenameLen = 0;
    U32 extensionLen = 0;
    BOOLEAN hasExtension = FALSE;
    U32 j = 0;

    for (U32 i = 0; i < len && j < ISO9660_MAX_PATH - 1; i++) {
        CHAR c = TOUPPER(path[i]);

        if (c == '/' || c == '\\') {
            res[j++] = '/';
            filenameLen = 0;
            extensionLen = 0;
            hasExtension = FALSE;
            continue;
        }

        if (c == '.') {
            if (hasExtension) {
                ISO9660_FREE_MEMORY_INTERNAL(&res);
                return NULLPTR; // multiple dots not allowed
            }
            hasExtension = TRUE;
            extensionLen = 0;
            res[j++] = c;
            continue;
        }

        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            ISO9660_FREE_MEMORY_INTERNAL(&res);
            return NULLPTR;
        }

        if (hasExtension) {
            if (extensionLen >= ISO9660_MAX_EXTENSION_LENGTH) continue;
            extensionLen++;
        } else {
            if (filenameLen >= ISO9660_MAX_FILENAME_LENGTH) continue;
            filenameLen++;
        }

        res[j++] = c;
    }

    res[j] = '\0';

    if (j < 2 || !(res[j - 2] == ';' && res[j - 1] == '1')) {
        if (j + 2 >= ISO9660_MAX_PATH) {
            ISO9660_FREE_MEMORY_INTERNAL(&res);
            return NULLPTR;
        }
        res[j++] = ';';
        res[j++] = '1';
        res[j] = '\0';
    }

    return res;
}

void ISO9660_EXTRACT_ROOT_FROM_PVD(PrimaryVolumeDescriptor *pvd, IsoDirectoryRecord *root) {
    if (!pvd || !root) return;
    *root = pvd->rootDirectoryRecord;
}

U32 ISO9660_ROUND_UP_TO_SECTOR(U32 size) {
    if (size == 0)
        return 0;

    U32 remainder = size % ISO9660_SECTOR_SIZE;
    if (remainder == 0)
        return size;

    U32 rounded = size + (ISO9660_SECTOR_SIZE - remainder);

    // Prevent overflow if size is near 0xFFFFFFFF
    if (rounded < size)
        return 0xFFFFFFFFU;

    return rounded;
}


U32 ISO9660_CALCULATE_SECTORS(U32 extent_length) {
    /* Avoid overflow safely with 32-bit math */
    if (extent_length == 0)
        return 0;

    U32 sectors = extent_length / ISO9660_SECTOR_SIZE;
    if ((extent_length % ISO9660_SECTOR_SIZE) != 0)
        sectors++;
    return sectors;
}
/* Recursive directory reading */
BOOLEAN ISO9660_READ_FROM_DIRECTORY(
    U32 lba,
    U32 size,
    CHAR *target,
    CHAR *original_target,
    IsoDirectoryRecord *output
) {
    if (!target || !original_target || !output) return FALSE;

    U32 sectors = ISO9660_CALCULATE_SECTORS(size);
    if (sectors > (0xFFFFFFFFu / ISO9660_SECTOR_SIZE))
        return FALSE;
    U32 buf_sz = sectors * ISO9660_SECTOR_SIZE;
    U8 *buffer = KMALLOC(buf_sz);
    if (!buffer) return FALSE;
    MEMZERO(buffer, buf_sz);
    if (READ_CDROM(GET_ATAPI_INFO(), lba, sectors, buffer) == ATA_FAILED) {
        ISO9660_FREE_MEMORY_INTERNAL(&buffer);
        return FALSE;
    }


    U32 offset = 0;
    while (offset < size) {
        if(offset >= buf_sz) break;  
        IsoDirectoryRecord *record = (IsoDirectoryRecord *)(buffer + offset);
        if (record->length == 0) break;
        if (record->fileNameLength >= ISO9660_MAX_PATH) {
            ISO9660_FREE_MEMORY_INTERNAL(&buffer);
            return FALSE;
        }
        CHAR record_name[ISO9660_MAX_PATH];
        STRNCPY(record_name, record->fileIdentifier, record->fileNameLength);
        record_name[record->fileNameLength] = '\0';

        if (record->fileFlags & ISO9660_FILE_FLAG_DIRECTORY) {
            if (STRCMP(record_name, target) == 0) {
                CHAR *slash = STRCHR(original_target, '/');
                if (slash) {
                    U32 remaining = STRLEN(slash + 1);
                    MEMMOVE(original_target, slash + 1, remaining);
                    original_target[remaining] = '\0';

                    MEMCPY(target, original_target, remaining + 1);
                    slash = STRCHR(target, '/');
                    if (slash) *slash = '\0';
                } else {
                    target[0] = '\0';
                }

                BOOLEAN res = ISO9660_READ_FROM_DIRECTORY(
                    record->extentLocationLE_LBA,
                    record->extentLengthLE,
                    target,
                    original_target,
                    output
                );
                ISO9660_FREE_MEMORY_INTERNAL(&buffer);
                return res;
            }
        } else {
            if (STRCMP(record_name, target) == 0) {
                MEMCPY(output, record, sizeof(IsoDirectoryRecord));
                ISO9660_FREE_MEMORY_INTERNAL(&buffer);
                return TRUE;
            }
        }

        offset += record->length;
    }
    ISO9660_FREE_MEMORY_INTERNAL(&buffer);
    return FALSE;
}

BOOLEAN ISO9660_PATH_TABLE_SEARCH(
    CHAR *original_target,
    IsoDirectoryRecord *output,
    IsoDirectoryRecord *root
) {
    if(!pvd) return FALSE;
    U32 path_table_size = pvd->pathTableSizeLE;
    U32 path_table_loc = pvd->pathTableLocationLE;
    U32 sectors = ATAPI_CALC_SECTORS(path_table_size);
    U8 *buf = KMALLOC(path_table_size);
    if(!buf) return FALSE;
    MEMZERO(buf, path_table_size);
    BOOL res = READ_CDROM(GET_ATAPI_INFO(), path_table_loc, sectors, buf);
    if(!res) {
        ISO9660_FREE_MEMORY_INTERNAL(&buf);
        return res;
    }
    U8 *target = KMALLOC(STRLEN(original_target) + 1);
    if (!target) {
        ISO9660_FREE_MEMORY_INTERNAL(&buf);
        return FALSE;
    };
    MEMZERO(target, STRLEN(original_target) + 1);
    STRCPY(target, original_target);
    U8 *slash = (U8 *)STRCHR(target, '/');
    if (slash) *slash = '\0';

    PathTableEntry *ent = NULLPTR;
    U32 pos = 0;
    U32 look_for_parent_id = 1;//root id
    BOOL found_dir = FALSE;
    while(1) {
        ent = (PathTableEntry *)(buf + pos);

        if (STRNCMP(target, ent->name, ent->nameLength) == 0 &&
            ent->parentDirNum == look_for_parent_id) {
            
            look_for_parent_id = ent->parentDirNum;

            // Check for remaining path
            CHAR *slash = STRCHR(original_target, '/');
            if (slash) {
                U32 remaining = STRLEN(slash + 1);
                MEMMOVE(original_target, slash + 1, remaining);
                original_target[remaining] = '\0';
                MEMCPY(target, original_target, remaining + 1);
                slash = STRCHR(target, '/');
                if (slash) *slash = '\0';
                pos = 0; // restart scanning path table
            } else {
                // Last component reached
                found_dir = TRUE;
                break;
            }
        }

        pos += sizeof(PathTableEntry) - 1 + ent->nameLength;
        if (ent->nameLength % 2 == 1) pos++;
        if(pos >= path_table_size) break;
    }
    // ISO9660_FREE_MEMORY_INTERNAL(&buf);
    if (!found_dir && STRLEN(target) == 0) {
        ISO9660_FREE_MEMORY_INTERNAL(&target);
        return FALSE;
    }
    
    res = READ_CDROM(GET_ATAPI_INFO(), ent->lbaLocation, 1, buf);
    if(!res) {
        ISO9660_FREE_MEMORY_INTERNAL(&target);
        ISO9660_FREE_MEMORY_INTERNAL(&buf);
        return FALSE;
    }
    MEMCPY(output, buf, sizeof(IsoDirectoryRecord));
    DUMP_MEMORY(output, sizeof(IsoDirectoryRecord));
    // ISO9660_FREE_MEMORY_INTERNAL(&buf);
    HLT;
    if(STRLEN(target) != 0) {
        DUMP_MEMORY(buf, ATAPI_SECTOR_SIZE);
        HLT;
    }
    ISO9660_FREE_MEMORY_INTERNAL(&target);
    ISO9660_FREE_MEMORY_INTERNAL(&buf);
    return TRUE;
}

BOOLEAN ISO9660_READ_DIRECTORY_RECORD(
    CHAR *path,
    IsoDirectoryRecord *root_record,
    IsoDirectoryRecord *out_record
) {
    if (!path || !root_record || !out_record) return FALSE;

    U32 LBA = root_record->extentLocationLE_LBA;
    U32 size = root_record->extentLengthLE;
    if (size == 0 || LBA == 0) return FALSE;

    U32 mod_path_len = STRLEN(path) + 1;
    U8 *modifiable_path = KMALLOC(mod_path_len);
    if (!modifiable_path) return FALSE;
    MEMZERO(modifiable_path, mod_path_len);
    STRCPY(modifiable_path, path);

    U8 *slash = (U8 *)STRCHR(modifiable_path, '/');
    if (slash) *slash = '\0';

    *out_record = *root_record;

    BOOLEAN res = ISO9660_READ_FROM_DIRECTORY(
        LBA,
        size,
        (CHAR *)modifiable_path,
        path,
        out_record
    );
    ISO9660_FREE_MEMORY_INTERNAL(&modifiable_path);
    return res;
}

IsoDirectoryRecord *ISO9660_FILERECORD_TO_MEMORY(CHAR *path) {
    CHAR *normPath = ISO9660_NORMALIZE_PATH(path);
    if (!normPath) return NULLPTR;

    PrimaryVolumeDescriptor *_pvd = pvd ? pvd : KMALLOC(sizeof(PrimaryVolumeDescriptor));
    if (!_pvd) { ISO9660_FREE_MEMORY_INTERNAL(&normPath); return NULLPTR; }
    
    if (!pvd) {
        MEMZERO(_pvd, sizeof(PrimaryVolumeDescriptor));
        if (!ISO9660_READ_PVD(_pvd, sizeof(PrimaryVolumeDescriptor))) {
            ISO9660_FREE_MEMORY_INTERNAL(&normPath);
            ISO9660_FREE_MEMORY_INTERNAL(&_pvd);
            return NULLPTR;
        }
        SetGlobalPVD(_pvd);
    }
    if (!ISO9660_IMAGE_CHECK(_pvd)) {
        ISO9660_FREE_MEMORY_INTERNAL(&normPath);
        if (!pvd) ISO9660_FREE_MEMORY_INTERNAL(&_pvd);
        return NULLPTR;
    }


    IsoDirectoryRecord root; // root record
    IsoDirectoryRecord out; // output record
    ISO9660_EXTRACT_ROOT_FROM_PVD(_pvd, &root);

    if (!ISO9660_READ_DIRECTORY_RECORD(normPath, &root, &out)) {
        ISO9660_FREE_MEMORY_INTERNAL(&normPath);
        if (!pvd) ISO9660_FREE_MEMORY_INTERNAL(&_pvd);
        return NULLPTR;
    }
    ISO9660_FREE_MEMORY_INTERNAL(&normPath);
    IsoDirectoryRecord *retval = KMALLOC(sizeof(IsoDirectoryRecord));
    if (!retval) {
        if (!pvd) ISO9660_FREE_MEMORY_INTERNAL(&_pvd);
        return NULLPTR;
    }
    MEMZERO(retval, sizeof(IsoDirectoryRecord));

    MEMCPY(retval, &out, sizeof(IsoDirectoryRecord));
    return retval;
}

VOIDPTR ISO9660_READ_FILEDATA_TO_MEMORY(IsoDirectoryRecord *fileptr) {
    if (!fileptr) return NULLPTR;
    U32 size = fileptr->extentLengthLE;
    U32 lba = fileptr->extentLocationLE_LBA;
    if (size == 0 || lba == 0) return NULLPTR;
    U32 real_size = ISO9660_ROUND_UP_TO_SECTOR(size);
    U8 *buffer = KMALLOC(real_size);
    if (!buffer) return NULLPTR;
    MEMZERO(buffer, real_size);
    U32 sectors = ISO9660_CALCULATE_SECTORS(real_size);
    U32 atapiStatus = INITIALIZE_ATAPI();
    if (atapiStatus == ATA_FAILED) { ISO9660_FREE_MEMORY_INTERNAL(&buffer); return NULLPTR; }

    if (READ_CDROM(atapiStatus, lba, sectors, buffer) == ATA_FAILED) {
        ISO9660_FREE_MEMORY_INTERNAL(&buffer);
        return NULLPTR;
    }
    return (VOIDPTR)buffer;
}

void ISO9660_FREE_MEMORY(VOIDPTR ptr) {
    if (!ptr) return;
    KFREE(ptr);
}
void ISO9660_FREE_MEMORY_INTERNAL(VOIDPTR *ptr) {
    if (!ptr || !*ptr) return;
    KFREE(*ptr);
    *ptr = NULLPTR;
}

void ISO9660_FREE_LIST(IsoDirectoryRecord **ptr, U32 cnt) {
    if (!ptr) return;

    for(U32 i = 0; i < cnt; i++) {
        if(ptr[i]) {
            KFREE(ptr[i]);
        }
    }
    KFREE(ptr);
}
void ISO9660_FREE_LIST_INTERNAL(IsoDirectoryRecord ***ptr, U32 cnt) {
    if (!ptr || !*ptr) return;

    for(U32 i = 0; i < cnt; i++) {
        if((*ptr)[i]) {
            KFREE((*ptr)[i]);
            (*ptr)[i] = NULLPTR;
        }
    }
    KFREE(*ptr);
    *ptr = NULLPTR;
}

U32 ISO9660_GET_TYPE(IsoDirectoryRecord *rec) {
    if(rec->fileFlags & ISO9660_FILE_FLAG_DIRECTORY) return ISO9660_TYPE_DIRECTORY;
    else return ISO9660_TYPE_FILE;
}

IsoDirectoryRecord **ISO9660_GET_DIR_CONTENTS(IsoDirectoryRecord *dir, BOOLEAN recursive, U32 *size_out, U8 *success) {
    if (!dir || !size_out || !success) return NULLPTR;

    *size_out = 0;
    *success = FALSE;
    recursive = FALSE; // reserved for future use

    U32 lba = dir->extentLocationLE_LBA;
    U32 size = dir->extentLengthLE;
    if (lba == 0 || size == 0) return NULLPTR;
    
    U32 sectors = ISO9660_CALCULATE_SECTORS(size);
    U8 *buffer = KMALLOC(sectors * ISO9660_SECTOR_SIZE);
    if (!buffer) return NULLPTR;
    MEMZERO(buffer, sectors * ISO9660_SECTOR_SIZE);

    if (READ_CDROM(GET_ATAPI_INFO(), lba, sectors, buffer) == ATA_FAILED) {
        ISO9660_FREE_MEMORY_INTERNAL(&buffer);
        return NULLPTR;
    }

    U32 offset = 0;
    IsoDirectoryRecord **entries = NULL;
    U32 count = 0;
    while (offset < size) {
        IsoDirectoryRecord *record = (IsoDirectoryRecord *)(buffer + offset);
        if (record->length == 0) break;
        if (offset + record->length > size) {
            break; 
        }
        if (record->fileNameLength >= ISO9660_MAX_PATH) {
            ISO9660_FREE_MEMORY_INTERNAL(&buffer);
            return FALSE;
        }
        if(record->fileNameLength > 2 && record->fileIdentifier[0] != '.' && record->fileIdentifier[1] != '.') {
            // DUMP_STRINGN(record->fileIdentifier, record->fileNameLength);
            IsoDirectoryRecord *copy = KMALLOC(record->length);
            if (!copy) {
                // Cleanup
                ISO9660_FREE_LIST_INTERNAL(entries, count);
                ISO9660_FREE_MEMORY_INTERNAL(&buffer);
                return NULLPTR;
            }

            MEMCPY(copy, record, record->length);
            
            IsoDirectoryRecord **tmp = NULL;
            if (entries == NULL) {
                tmp = KMALLOC((count + 1) * sizeof(IsoDirectoryRecord *));
                if (!tmp) {
                    ISO9660_FREE_LIST_INTERNAL(entries, count);
                    ISO9660_FREE_MEMORY_INTERNAL(&copy);
                    ISO9660_FREE_MEMORY_INTERNAL(&buffer);
                    return NULLPTR;
                }
                MEMZERO(tmp, (count) * sizeof(IsoDirectoryRecord *));
                entries = tmp;
            } else {
                // Use your KREALLOC correctly
                tmp = (IsoDirectoryRecord **)KREALLOC(entries, (count + 1) * sizeof(IsoDirectoryRecord *));
                if (!tmp) {
                    ISO9660_FREE_LIST_INTERNAL(entries, count);
                    ISO9660_FREE_MEMORY_INTERNAL(&copy);
                    ISO9660_FREE_MEMORY_INTERNAL(&buffer);
                    return NULLPTR;
                }
                entries = tmp;
            }
            entries[count++] = copy;
        }
        offset += record->length;
    }
    ISO9660_FREE_MEMORY_INTERNAL(&buffer);
    *size_out = count;
    *success = TRUE;
    // DUMP_INTNO(count);
    return entries;
}



VOIDPTR ISO9660_READ_FILEDATA_TO_MEMORY_QUICKLY(U8 *filename, U32 *filesize_out) {
    IsoDirectoryRecord *rec = NULLPTR;
    VOIDPTR bin = NULLPTR;
    rec = ISO9660_FILERECORD_TO_MEMORY(filename);
    if(!rec) return NULLPTR;
    *filesize_out = rec->extentLengthLE;
    bin = ISO9660_READ_FILEDATA_TO_MEMORY(rec);

    if(!bin) {
        ISO9660_FREE_MEMORY_INTERNAL(&rec);
        return NULLPTR;
    }

    ISO9660_FREE_MEMORY_INTERNAL(&rec);

    return bin;
}

BOOLEAN ISO9660_IS_DIR(IsoDirectoryRecord *rec) {

}