#include <DRIVERS/ATA_PIO/ATA_PIO.h>
#include <DRIVERS/CMOS/CMOS.h>
#include <RTOSKRNL/RTOSKRNL_INTERNAL.h>
#include <FS/FAT/FAT.h>
#include <FS/ISO9660/ISO9660.h>
#include <HEAP/KHEAP.h>
#include <PROC/PROC.h>
#include <STD/MEM.h>
#include <STD/MATH.h>
#include <STD/STRING.h>
#include <STD/BINARY.h>

#define Free(ptr) Free(ptr); ptr = NULLPTR; 

static BPB bpb ATTRIB_DATA = { 0 };
static U32 *fat32 ATTRIB_DATA = NULLPTR;
static DIR_ENTRY *root_dir = NULLPTR;
static U32 root_dir_end ATTRIB_DATA = 0;
static BOOL bpb_loaded ATTRIB_DATA = 0;
static FSINFO fsinfo ATTRIB_DATA = { 0 };

#define SET_BPB(x, val, sz) MEMCPY(&x, val, sz)

#define GET_CLUSTER_SIZE() (bpb.BYTES_PER_SECTOR * bpb.SECTORS_PER_CLUSTER)
#define GET_CLUSTERS_NEEDED(cluster_size, bytes)( (bytes + cluster_size - 1) / cluster_size)
#define GET_CLUSTERS_NEEDED_IN_BYTES(clusters_needed, cluster_size) (clusters_needed * (cluster_size))


// Writes a single sector to disk at absolute sector number
BOOL FAT_WRITE_SECTOR_ON_DISK(U32 lba, const U8 *buf) {
    if (!buf) return FALSE;
    // ATA_PIO_WRITE_SECTORS expects LBA, count, buffer
    return ATA_PIO_WRITE_SECTORS(lba, 1, buf);
}

// Reads a single sector from disk at absolute sector number
BOOL FAT_READ_SECTOR_FROM_DISK(U32 lba, U8 *buf) {
    if (!buf) return FALSE;
    return ATA_PIO_READ_SECTORS(lba, 1, buf);
}

// Write cluster buffer (cluster -> disk sector mapping)
BOOL FAT_WRITE_CLUSTER(U32 cluster, const U8 *buf) {
    if (!buf) return FALSE;
    if (cluster < FIRST_ALLOWED_CLUSTER_NUMBER) return FALSE;

    // Make sure CLUSTER_SIZE matches sectors per cluster
    if (bpb.SECTORS_PER_CLUSTER * 512 != CLUSTER_SIZE) return FALSE;

    U32 data_start_sector = bpb.RESERVED_SECTORS + (bpb.NUM_OF_FAT * bpb.EXBR.SECTORS_PER_FAT);
    U32 sector = data_start_sector + (cluster - FIRST_ALLOWED_CLUSTER_NUMBER) * bpb.SECTORS_PER_CLUSTER;

    // Optionally, check sector range against total disk sectors
    // if (sector + bpb.SECTORS_PER_CLUSTER > TOTAL_DISK_SECTORS) return FALSE;

    return ATA_PIO_WRITE_SECTORS(sector, bpb.SECTORS_PER_CLUSTER, buf);
}


// Read cluster buffer (cluster -> disk sector mapping)
BOOL FAT_READ_CLUSTER(U32 cluster, U8 *buf) {
    if (!buf) return FALSE;
    if (cluster < FIRST_ALLOWED_CLUSTER_NUMBER) return FALSE;

    U32 data_start_sector = bpb.RESERVED_SECTORS + (bpb.NUM_OF_FAT * bpb.EXBR.SECTORS_PER_FAT);
    U32 sector = data_start_sector + (cluster - FIRST_ALLOWED_CLUSTER_NUMBER) * bpb.SECTORS_PER_CLUSTER;
    
    return ATA_PIO_READ_SECTORS(sector, bpb.SECTORS_PER_CLUSTER, buf);
}


BOOLEAN READ_FAT(){
    return TRUE;
}
BOOLEAN READ_ROOT_DIR(){
    // root_dir_end
    return TRUE;
}

BOOLEAN LOAD_BPB() { 
    U8 *buf = KMALLOC(ATA_PIO_SECTOR_SIZE);
    if(!buf) return FALSE;
    if(!FAT_READ_SECTOR_FROM_DISK(0, buf)) {
        Free(buf);
        return FALSE;
    }
    MEMCPY(&bpb, buf, sizeof(BPB));

    Free(buf);
    if(!READ_FAT()) return FALSE;
    if(!READ_ROOT_DIR()) return FALSE;
    bpb_loaded = TRUE;
    return TRUE;
}
BOOLEAN GET_BPB_LOADED(){ 
    // TODO: read from disk and check ids, if IDs, LOAD_BPB()
    return TRUE; 
}


BPB *GET_BPB() { 
    if(!bpb_loaded) {
        if(!LOAD_BPB()) return NULLPTR;
    } 
    return &bpb;
}
BOOLEAN WRITE_DISK_BPB() {
    MEMZERO(&bpb, sizeof(BPB));
    bpb.JMPSHORT[0] = 0xEB;
    bpb.JMPSHORT[1] = 0x3C;
    bpb.JMPSHORT[2] = 0x90;
    SET_BPB(bpb.OEM_IDENT, "MSWIN4.1", 8);

    bpb.BYTES_PER_SECTOR = BYTES_PER_SECT;
    bpb.SECTORS_PER_CLUSTER = SECT_PER_CLUST;     // 4 KB clusters
    bpb.RESERVED_SECTORS = 32;
    bpb.NUM_OF_FAT = 2;
    bpb.NUM_OF_ROOT_DIR_ENTRIES = 0; // FAT32 stores root dir in cluster chain
    bpb.TOTAL_SECTORS = 0;           // set 0 if >65535 sectors (use LARGE_SECTOR_COUNT)
    bpb.MEDIA_DESCR_TYPE = 0xF8;
    bpb.NUM_OF_SECTORS_PER_FAT = 0;  // not used for FAT32
    bpb.NUM_OF_SECTORS_PER_TRACK = 63;
    bpb.NUM_OF_HEADS = 255;
    bpb.NUM_OF_HIDDEN_SECTORS = 0;
    bpb.LARGE_SECTOR_COUNT = 65536 * 8; // example: ~256MB

    // Extended section (FAT32)
    bpb.EXBR.SECTORS_PER_FAT = 8192;
    bpb.EXBR.EXT_FLAGS = 0;
    bpb.EXBR.FS_VERSION = 0;
    bpb.EXBR.ROOT_CLUSTER = 2; // root starts at cluster 2
    bpb.EXBR.FS_INFO_SECTOR = 1;
    bpb.EXBR.BACKUP_BOOT_SECTOR = 6;
    MEMSET(bpb.EXBR.RESERVED, 0, 12);
    bpb.EXBR.DRIVE_NUMBER = 0x80;
    bpb.EXBR.RESERVED1 = 0;
    bpb.EXBR.BOOT_SIGNATURE = 0x29;
    bpb.EXBR.VOLUME_ID = 0x12345678;
    SET_BPB(bpb.EXBR.VOLUME_LABEL, "ATOSRT__FS_", 11);
    SET_BPB(bpb.EXBR.FILE_SYSTEM_TYPE, "FAT32   ", 8);
    
    
    U8 *buf = KMALLOC(ATA_PIO_SECTOR_SIZE);
    if(!buf) return FALSE;
    MEMZERO(buf, ATA_PIO_SECTOR_SIZE);
    MEMCPY(buf, &bpb, sizeof(BPB));
    U32 res = FAT_WRITE_SECTOR_ON_DISK(0, buf);
    Free(buf);
    
    bpb_loaded = TRUE;
    return res;
}

BOOLEAN POPULATE_BOOTLOADER(VOIDPTR BOOTLOADER_BIN, U32 sz) {
    if (sz + sizeof(BPB) > ATA_PIO_SECTOR_SIZE)
        return FALSE;

    U8 *loader = (U8 *)BOOTLOADER_BIN;

    U8 *buf = KMALLOC(ATA_PIO_SECTOR_SIZE);
    if (!buf) return FALSE;
    MEMZERO(buf, ATA_PIO_SECTOR_SIZE);

    if (!FAT_READ_SECTOR_FROM_DISK(0, buf)) {
        Free(buf);
        return FALSE;
    }

    MEMCPY(buf, &bpb, sizeof(BPB));

    MEMCPY(buf + sizeof(BPB), loader, sz);

    buf[510] = 0x55;
    buf[511] = 0xAA;

    BOOLEAN ok = FAT_WRITE_SECTOR_ON_DISK(0, buf);
    Free(buf);
    return ok;
}

void FAT_UPDATETIMEDATE(DIR_ENTRY *entry) {
    if (!entry) return;
    RTC_DATE_TIME r = GET_SYS_TIME();
    entry->LAST_MOD_DATE = SET_DATE(r.year, r.month, r.day_of_month);
    entry->LAST_MOD_TIME = SET_TIME(r.hours, r.minutes, r.seconds);
}

VOID FAT_CREATION_UPDATETIMEDATE(DIR_ENTRY *entry) {
    RTC_DATE_TIME r = GET_SYS_TIME();
    entry->CREATION_TIME_HUN_SEC = 0;
    entry->CREATION_DATE = SET_DATE(r.year, r.month, r.day_of_month);
    entry->CREATION_TIME = SET_TIME(r.hours, r.minutes, r.seconds);
    entry->LAST_MOD_DATE = SET_DATE(r.year, r.month, r.day_of_month);
    entry->LAST_MOD_TIME = SET_TIME(r.hours, r.minutes, r.seconds);
}

BOOLEAN INITIAL_WRITE_FAT() {
    if (!bpb_loaded) return FALSE;

    U32 fat_sectors = bpb.EXBR.SECTORS_PER_FAT;
    U32 bytes_per_fat = fat_sectors * bpb.BYTES_PER_SECTOR;

    // Allocate and zero FAT buffer
    U8 *fatbuf = KMALLOC(bytes_per_fat);
    if (!fatbuf) return FALSE;
    MEMZERO(fatbuf, bytes_per_fat);

    fat32 = (U32 *)fatbuf;

    // Mark first 3 FAT entries
    fat32[0] = 0x0FFFFFF8;                  // Media descriptor + reserved
    fat32[1] = 0xFFFFFFFF;                  // Reserved
    fat32[2] = FAT32_END_OF_CHAIN;          // Root directory EOC

    // Total clusters
    U32 total_clusters = bytes_per_fat / 4;

    // Mark remaining clusters as free
    for (U32 i = 3; i < total_clusters; i++) {
        fat32[i] = FAT32_FREE_CLUSTER;      // 0x00000000
    }

    // Write FAT #1
    if (!ATA_PIO_WRITE_SECTORS(bpb.RESERVED_SECTORS, fat_sectors, fatbuf)) {
        Free(fatbuf);
        return FALSE;
    }

    // Write FAT #2 (backup)
    if (!ATA_PIO_WRITE_SECTORS(bpb.RESERVED_SECTORS + fat_sectors, fat_sectors, fatbuf)) {
        Free(fatbuf);
        return FALSE;
    }

    return TRUE;
}


BOOL FAT_FLUSH(void) {
    U32 fat_sectors = bpb.EXBR.SECTORS_PER_FAT;
    U32 first_fat_lba = bpb.RESERVED_SECTORS;
    // write FAT #1
    if (!ATA_PIO_WRITE_SECTORS(first_fat_lba, fat_sectors, (U8 *)fat32)) return FALSE;
    // write FAT #2 (backup)
    if (!ATA_PIO_WRITE_SECTORS(first_fat_lba + fat_sectors, fat_sectors, (U8 *)fat32)) return FALSE;
    return TRUE;
}


static U32 last_allocated_cluster = 2;

U32 FIND_NEXT_FREE_CLUSTER() {
    U32 fat_sectors = bpb.EXBR.SECTORS_PER_FAT;
    U32 bytes_per_fat = fat_sectors * bpb.BYTES_PER_SECTOR;
    U32 total_clusters = bytes_per_fat / 4;

    for(U32 offset = 0; offset < total_clusters - 2; offset++) {
        U32 i = 2 + (last_allocated_cluster - 2 + offset) % (total_clusters - 2);
        if(fat32[i] == FAT32_FREE_CLUSTER) {
            last_allocated_cluster = i + 1;
            return i;
        }
    }
    return 0; // no free cluster
}


U32 FAT32_GetLastCluster(U32 start_cluster) {
    U32 current = start_cluster;
    while (fat32[current] < FAT32_END_OF_CHAIN && fat32[current] != 0) {
        current = fat32[current];
    }
    return current;
}

BOOL FAT32_AppendCluster(U32 start_cluster, U32 new_cluster) {
    if (new_cluster < FIRST_ALLOWED_CLUSTER_NUMBER) return FALSE;

    U32 last = FAT32_GetLastCluster(start_cluster);
    fat32[last] = new_cluster;          // link old end to new
    fat32[new_cluster] = FAT32_END_OF_CHAIN;     // mark new as end
    return FAT_FLUSH();
}

U32 BYTES_FREE_IN_CLUSTER(U32 cluster) {
    U8 *buf = KMALLOC(CLUSTER_SIZE); // cluster_size = sectors_per_cluster * bytes_per_sector
    if(!buf) return 0;
    MEMZERO(buf, CLUSTER_SIZE);
    FAT_READ_CLUSTER(cluster, buf); // read cluster into buffer

    U32 used_bytes = 0;
    for(U32 offset = 0; offset < CLUSTER_SIZE; offset += 32) {
        DIR_ENTRY *entry = (DIR_ENTRY *)(buf + offset);
        if(entry->FILENAME[0] != 0x00 && entry->FILENAME[0] != FAT32_DELETED_ENTRY)
            used_bytes += 32;
        else
            break; // first free slot found, rest are free
    }
    Free(buf);
    return CLUSTER_SIZE - used_bytes;
}

BOOL MARK_CLUSTER_FREE(U32 cluster) {
    if(cluster < FIRST_ALLOWED_CLUSTER_NUMBER) return FALSE; // reserved clusters

    fat32[cluster] = FAT32_FREE_CLUSTER; // mark as free

    return FAT_FLUSH();
}

BOOL MARK_CLUSTER_USED(U32 cluster) {
    if (cluster < FIRST_ALLOWED_CLUSTER_NUMBER) return FALSE;
    fat32[cluster] = FAT32_END_OF_CHAIN;
    return FAT_FLUSH();
}

static BOOLEAN DIR_NAME_COMP_CASE(const DIR_ENTRY *entry, const U8 *name) {
    if (!entry || !name) return FALSE;

    // Convert input name to FAT 8.3 format (uppercase not forced)
    DIR_ENTRY tmp;
    MEMZERO(&tmp, sizeof(tmp));
    FAT_83FILENAMEFY(&tmp, (U8*)name);

    // Strict, case-sensitive comparison of all 11 bytes
    return (MEMCMP(entry->FILENAME, tmp.FILENAME, 11) == 0);
}

static BOOLEAN LFN_MATCH(U8 *name, LFN *lfns, U32 count) {
    if (!name || !lfns || count == 0) return FALSE;

    U32 pos = 0;
    for (U32 i = 0; i < count; i++) {
        LFN *e = &lfns[i];
        // NAME_1 (5 chars)
        for (U32 j = 0; j < 5 && e->NAME_1[j] != 0 && pos < FAT_MAX_FILENAME; j++, pos++)
            if ((U8)e->NAME_1[j] != name[pos]) return FALSE;
        // NAME_2 (6 chars)
        for (U32 j = 0; j < 6 && e->NAME_2[j] != 0 && pos < FAT_MAX_FILENAME; j++, pos++)
            if ((U8)e->NAME_2[j] != name[pos]) return FALSE;
        // NAME_3 (2 chars)
        for (U32 j = 0; j < 2 && e->NAME_3[j] != 0 && pos < FAT_MAX_FILENAME; j++, pos++)
            if ((U8)e->NAME_3[j] != name[pos]) return FALSE;
    }

    return name[pos] == '\0';
}

// --- Find a free slot (cluster+offset) in a directory chain ---
// returns TRUE and sets *out_cluster and *out_offset (byte offset in cluster) when free slot found.
// If no free slot in existing chain, appends a new cluster and returns its start offset 0.
BOOL DIR_FIND_FREE_SLOT(U32 dir_start_cluster, U32 *out_cluster, U32 *out_offset) {
    if (dir_start_cluster < FIRST_ALLOWED_CLUSTER_NUMBER) return FALSE;

    U32 cluster = dir_start_cluster;
    U8 *buf = KMALLOC(CLUSTER_SIZE);
    if(!buf) return FALSE;
    MEMZERO(buf, CLUSTER_SIZE);
    while (1) {
        if (!FAT_READ_CLUSTER(cluster, buf)) {
            Free(buf);
            return FALSE;
        }
        // scan entries in this cluster
        for (U32 offset = 0; offset < CLUSTER_SIZE; offset += 32) {
            DIR_ENTRY *e = (DIR_ENTRY *)(buf + offset);
            if (e->FILENAME[0] == 0x00 || e->FILENAME[0] == FAT32_DELETED_ENTRY) {
                *out_cluster = cluster;
                *out_offset = offset;
                Free(buf);
                return TRUE;
            }
        }

        // no free slot in this cluster -> check chain
        if (fat32[cluster] >= FAT32_END_OF_CHAIN) {
            // need to allocate and append a new cluster for directory
            U32 new_cluster = FIND_NEXT_FREE_CLUSTER();
            if (new_cluster == 0) return FALSE;
            fat32[cluster] = new_cluster;
            fat32[new_cluster] = FAT32_END_OF_CHAIN;
            if (!FAT_FLUSH()) {
                Free(buf);
                return FALSE;
            }
            // clear new cluster on disk (zero it)
            MEMZERO(buf, CLUSTER_SIZE);
            if (!FAT_WRITE_CLUSTER(new_cluster, buf)) {
                Free(buf);
                return FALSE;
            }
            *out_cluster = new_cluster;
            *out_offset = 0;
            Free(buf);
            return TRUE;
        }

        // follow chain
        cluster = fat32[cluster];
        if (cluster < FIRST_ALLOWED_CLUSTER_NUMBER)  {
            Free(buf);
            return FALSE;
        } // corrupted
    }
}

U8 LFN_CHECKSUM(U8 *shortname) {
    U8 sum = 0;
    for (int i = 0; i < 11; i++) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + shortname[i];
    }
    return sum;
}

VOID FAT_83FILENAMEFY(DIR_ENTRY *entry, const U8 *original) {
    if (!entry || !original) return;
    
    // Initialize all 12 bytes: 8 (name) + 1 (dot) + 3 (ext)
    for (U32 i = 0; i < 11; i++)
        entry->FILENAME[i] = ' ';
    
    // Find last dot, if any
    const U8 *dot = STRRCHR(original, '.');
    U32 base_len = 0, ext_len = 0;

    if (dot) {
        base_len = (U32)(dot - original);
        ext_len  = STRLEN(dot + 1);
    } else {
        base_len = STRLEN(original);
    }

    // FAT base name is 8 chars max, extension is 3 chars max
    if (base_len > 8) base_len = 8;
    if (ext_len > 3)  ext_len  = 3;

    // Copy and uppercase base name (positions 0–7)
    for (U32 i = 0; i < base_len; i++) {
        entry->FILENAME[i] = TOUPPER(original[i]);
    }

    // Insert dot at position 8 (after name)
    if (dot && ext_len > 0)
        entry->FILENAME[7] = '.';
    else
        entry->FILENAME[8] = ' ';

    // Copy and uppercase extension (positions 9–11)
    if (dot && ext_len > 0) {
        for (U32 i = 0; i < ext_len; i++) {
            entry->FILENAME[8 + i] = TOUPPER(dot[1 + i]);
        }
    }
}


// For display: convert 8.3 name into printable string with dot
VOID FAT_PRINT_83NAME(DIR_ENTRY *entry, CHAR *out) {
    U32 i;
    // Copy base name
    for(i = 0; i < 8 && entry->FILENAME[i] != ' '; i++) {
        out[i] = entry->FILENAME[i];
    }
    // If extension exists, add dot
    if(entry->FILENAME[8] != ' ') {
        out[i++] = '.';
        for(U32 j = 0; j < 3 && entry->FILENAME[8 + j] != ' '; j++) {
            out[i++] = entry->FILENAME[8 + j];
        }
    }
    out[i] = '\0';
}


VOID FILL_LFNs(LFN *LFNs, U32 n, U8 *name) {
    U32 str_index = 0;
    U32 strlen = STRLEN(name);
    for(U32 i = 0; i < n; i++) {
        LFN *ent = &LFNs[n - 1 - i]; // reverse order
        ent->ALWAYS_ZERO = 0;
        ent->CHECKSUM = LFN_CHECKSUM(name);
        ent->ATTR = FAT_ATTRIB_LFN;
        ent->TYPE = 0x0;
        ent->ORDER = i + 1;
        if(i == n - 1) ent->ORDER |= LFN_LAST_ENTRY;

        for(U32 j = 0; j < 5; j++) ent->NAME_1[j] = 0xFFFF;
        for(U32 j = 0; j < 6; j++) ent->NAME_2[j] = 0xFFFF;
        for(U32 j = 0; j < 2; j++) ent->NAME_3[j] = 0xFFFF;

        for(U32 j = 0; j < 5 && str_index < strlen; j++, str_index++) {
            ent->NAME_1[j] = name[str_index];
        }
        for(U32 j = 0; j < 6 && str_index < strlen; j++, str_index++) {
            ent->NAME_2[j] = name[str_index];
        }
        for(U32 j = 0; j < 2 && str_index < strlen; j++, str_index++) {
            ent->NAME_3[j] = name[str_index];
        }
    }
}

BOOLEAN WRITE_FILEDATA(DIR_ENTRY *out_ent, PU8 filedata, U32 sz) {
    if (!out_ent) return FALSE;
    
    // Empty file: no clusters needed
    if (sz == 0) {
        out_ent->LOW_CLUSTER_BITS  = 0;
        out_ent->HIGH_CLUSTER_BITS = 0;
        return TRUE;
    }
    
    U32 clusters_needed = (sz + CLUSTER_SIZE - 1) / CLUSTER_SIZE;
    U32 *allocated_clusters = KMALLOC(clusters_needed * sizeof(U32));
    if (!allocated_clusters) return FALSE;
    
    U32 allocated_count = 0;
    U32 first_cluster = 0;
    U32 prev_cluster  = 0;
    U32 remaining = sz;
    PU8 src = filedata;

    U8 *buf = KMALLOC(CLUSTER_SIZE);
    if (!buf) {
        Free(allocated_clusters);
        return FALSE;
    }

    for (U32 i = 0; i < clusters_needed; i++) {
        U32 c = FIND_NEXT_FREE_CLUSTER();
        if (c == 0) {
            // Out of space — rollback all allocated clusters
            for (U32 j = 0; j < allocated_count; j++)
                fat32[allocated_clusters[j]] = FAT32_FREE_CLUSTER;
            FAT_FLUSH();
            Free(buf);
            Free(allocated_clusters);
            return FALSE;
        }

        allocated_clusters[allocated_count++] = c;

        if (prev_cluster != 0) {
            fat32[prev_cluster] = c;
        } else {
            first_cluster = c;
        }
        prev_cluster = c;

        // Prepare buffer contents
        U32 tocopy = (remaining > CLUSTER_SIZE) ? CLUSTER_SIZE : remaining;
        if (tocopy < CLUSTER_SIZE) {
            MEMZERO(buf, CLUSTER_SIZE);
            MEMCPY(buf, src, tocopy);
        } else {
            // For full clusters, copy directly (no need to zero)
            MEMCPY(buf, src, CLUSTER_SIZE);
        }

        if (!FAT_WRITE_CLUSTER(c, buf)) {
            // rollback on failure
            for (U32 j = 0; j < allocated_count; j++)
                fat32[allocated_clusters[j]] = FAT32_FREE_CLUSTER;
            FAT_FLUSH();
            Free(buf);
            Free(allocated_clusters);
            return FALSE;
        }

        src       += tocopy;
        remaining -= tocopy;
    }

    // Mark end-of-chain
    fat32[prev_cluster] = FAT32_END_OF_CHAIN;

    // Flush FAT to disk
    if (!FAT_FLUSH()) {
        Free(buf);
        Free(allocated_clusters);
        return FALSE;
    }

    // Set directory entry start cluster
    out_ent->LOW_CLUSTER_BITS  = first_cluster & 0xFFFF;
    out_ent->HIGH_CLUSTER_BITS = (first_cluster >> 16) & 0xFFFF;
    out_ent->FILE_SIZE = sz;
    Free(buf);
    Free(allocated_clusters);
    return TRUE;
}

BOOLEAN CREATE_DIR_ENTRY(
    U32 parent_cluster,
    U8 *FILENAME,
    U8 ATTRIB,
    PU8 filedata,
    U32 filedata_size,
    DIR_ENTRY *out,
    U32 opt_initial_cluster
) {
    if (!out || !FILENAME) return FALSE;

    // 1. Duplicate check
    DIR_ENTRY temp;
    if (FIND_DIR_ENTRY_BY_NAME_AND_PARENT(&temp, parent_cluster, FILENAME))
        return FALSE;

    MEMZERO(out, sizeof(DIR_ENTRY));

    // 2. Attributes, 8.3 name, timestamps
    out->ATTRIB = ATTRIB;
    FAT_83FILENAMEFY(out, FILENAME);
    FAT_CREATION_UPDATETIMEDATE(out);
    out->CREATION_TIME_HUN_SEC = (get_ticks() % 199) + 1;

    // 3. Assign initial cluster
    if (ATTRIB & FAT_ATTRB_DIR) {
        out->LOW_CLUSTER_BITS  = opt_initial_cluster & 0xFFFF;
        out->HIGH_CLUSTER_BITS = (opt_initial_cluster >> 16) & 0xFFFF;
    } else {
        out->LOW_CLUSTER_BITS  = 0;
        out->HIGH_CLUSTER_BITS = 0;
    }

    // 4. LFN handling
    U32 num_lfn_entries = 0;
    LFN LFNs[MAX_LFN_COUNT];
    MEMZERO(LFNs, sizeof(LFNs));
    U32 filename_len = STRLEN(FILENAME);
    if (filename_len > 11) {
        num_lfn_entries = (filename_len + CHARS_PER_LFN - 1) / CHARS_PER_LFN;
        FILL_LFNs(LFNs, num_lfn_entries, FILENAME);
    }

    // 5. File data (skip for directories)
    if (!(ATTRIB & FAT_ATTRB_DIR) && filedata_size > 0 && filedata) {
        out->FILE_SIZE = filedata_size;
        if (!WRITE_FILEDATA(out, filedata, filedata_size)) return FALSE;
    } else {
        out->FILE_SIZE = 0;
    }

    // 6. Find free slot in parent directory
    U32 slot_cluster = 0, slot_offset = 0;
    if (!DIR_FIND_FREE_SLOT(parent_cluster, &slot_cluster, &slot_offset)) return FALSE;

    // 7. Allocate cluster buffer and read
    U8 *buf = KMALLOC(CLUSTER_SIZE);
    if (!buf) return FALSE;
    MEMZERO(buf, CLUSTER_SIZE);
    if (!FAT_READ_CLUSTER(slot_cluster, buf)) {
        Free(buf);
        return FALSE;
    }

    // 8. Ensure enough space for LFN + entry
    U32 available = CLUSTER_SIZE - slot_offset;
    U32 required = num_lfn_entries * sizeof(LFN) + sizeof(DIR_ENTRY);
    if (required > available) {
        U32 new_cluster = FIND_NEXT_FREE_CLUSTER();
        if (new_cluster == 0) { Free(buf); return FALSE; }

        fat32[slot_cluster] = new_cluster;
        fat32[new_cluster] = FAT32_END_OF_CHAIN;
        if (!FAT_FLUSH()) { Free(buf); return FALSE; }

        MEMZERO(buf, CLUSTER_SIZE);
        slot_cluster = new_cluster;
        slot_offset = 0;
    }

    // 9. Write LFN + short entry
    U8 *dest = buf + slot_offset;
    if (num_lfn_entries) {
        MEMCPY(dest, LFNs, num_lfn_entries * sizeof(LFN));
        dest += num_lfn_entries * sizeof(LFN);
    }
    MEMCPY(dest, out, sizeof(DIR_ENTRY));

    if (!FAT_WRITE_CLUSTER(slot_cluster, buf)) {
        Free(buf);
        return FALSE;
    }

    Free(buf);
    return TRUE;
}

BOOLEAN CREATE_ROOT_DIR(U32 root_cluster) {
    U8 *buf = KMALLOC(CLUSTER_SIZE);
    if(!buf) return FALSE;
    MEMZERO(buf, CLUSTER_SIZE);

    DIR_ENTRY dot = {0};
    DIR_ENTRY dotdot = {0};

    // --- . Entry ---
    MEMCPY(dot.FILENAME, ".          ", 11);
    dot.ATTRIB = FAT_ATTRB_DIR; // Correct: Must be marked as a directory
    // Correct: Points to itself (root_cluster)
    dot.LOW_CLUSTER_BITS = root_cluster & 0xFFFF;
    dot.HIGH_CLUSTER_BITS = (root_cluster >> 16) & 0xFFFF;

    // --- .. Entry ---
    MEMCPY(dotdot.FILENAME, "..         ", 11);
    dotdot.ATTRIB = FAT_ATTRB_DIR; // Correct: Must be marked as a directory
    // Correct: Points to itself (root_cluster)
    dotdot.LOW_CLUSTER_BITS = root_cluster & 0xFFFF;
    dotdot.HIGH_CLUSTER_BITS = (root_cluster >> 16) & 0xFFFF;

    MEMCPY(buf, &dot, sizeof(DIR_ENTRY));
    MEMCPY(buf + sizeof(DIR_ENTRY), &dotdot, sizeof(DIR_ENTRY));
    
    // ... write to disk and update FAT table ...
    if (!FAT_WRITE_CLUSTER(root_cluster, buf)) {
        Free(buf);
        return FALSE;
    }
    // Mark root cluster as used
    fat32[root_cluster] = FAT32_END_OF_CHAIN;
    Free(buf);
    return FAT_FLUSH();
}

BOOLEAN CREATE_FSINFO() {
    FSINFO fsinfo;
    MEMZERO(&fsinfo, sizeof(fsinfo));

    fsinfo.SIGNATURE0  = 0x41615252;
    fsinfo.SIGNATURE1 = 0x61417272;
    fsinfo.FREE_CLUSTER_COUNT = 0xFFFFFFFF;
    fsinfo.CLUSTER_INDICATOR   = FIRST_ALLOWED_CLUSTER_NUMBER; // typically 2
    fsinfo.TRAIL_SIGNATURE  = 0xAA550000;

    return FAT_WRITE_SECTOR_ON_DISK(bpb.EXBR.FS_INFO_SECTOR, (U8 *)&fsinfo);
}

// Recursive function to copy ISO directories and files
BOOLEAN copy_iso_to_fat32(IsoDirectoryRecord *iso_dir, U32 parent_cluster) {
    if (!iso_dir) return FALSE;

    U32 count = 0;
    U8 success;
    IsoDirectoryRecord **dir_contents = ISO9660_GET_DIR_CONTENTS(iso_dir, FALSE, &count, &success);
    if(!success) return FALSE;
    if (count == 0) return TRUE;
    for (U32 i = 0; i < count; i++) {
        IsoDirectoryRecord *rec = dir_contents[i];
        if (!rec || rec->length == 0) continue;
        CHAR name[ISO9660_MAX_PATH];
        MEMZERO(name, sizeof(name));
        U32 copy_len = rec->fileNameLength < ISO9660_MAX_PATH - 1 ? rec->fileNameLength : ISO9660_MAX_PATH - 1;
        MEMCPY(name, rec->fileIdentifier, copy_len);
        name[copy_len] = '\0';
        for (U32 j = 0; j < copy_len; j++) {    
            if (name[j] == ';') {
                name[j] = '\0';
                break;
            }
        }
        if (rec->fileFlags & ISO9660_FILE_FLAG_DIRECTORY) {
            // Create directory in FAT32
            U32 cluster_out = 0;
            if (!CREATE_CHILD_DIR(parent_cluster, (U8 *)name, FAT_ATTRB_DIR, &cluster_out)) {
                panic_debug(name, 0xAA);
                ISO9660_FREE_LIST(dir_contents, count);
                return FALSE;
            }
            if (!copy_iso_to_fat32(rec, cluster_out)) {
                panic_debug(name, 0xAB);
                ISO9660_FREE_LIST(dir_contents, count);
                return FALSE;
            }
        } else {
            // Read file contents from ISO
            U8 *file_data = ISO9660_READ_FILEDATA_TO_MEMORY(rec);
            if (!file_data) {
                rec->fileIdentifier[rec->fileNameLength] = '\0';
                panic_debug(rec->fileIdentifier,0xAC);
                ISO9660_FREE_LIST(dir_contents, count);
                return FALSE;
            }
            U32 cluster_out = 0;
            // Create file in FAT32
            if (!CREATE_CHILD_FILE(parent_cluster, (U8 *)name, FAT_ATTRIB_ARCHIVE, file_data, rec->extentLengthLE, &cluster_out)) {
                rec->fileIdentifier[rec->fileNameLength] = '\0';
                panic_debug(rec->fileIdentifier, 0xAD);
                ISO9660_FREE_MEMORY(file_data);
                ISO9660_FREE_LIST(dir_contents, count);
                return FALSE;
            }
            ISO9660_FREE_MEMORY(file_data);
        }
    }
    ISO9660_FREE_LIST(dir_contents, count);
    return TRUE;
}

BOOLEAN COPY_ISO9660_CONTENTS_TO_FAT32() {
    PrimaryVolumeDescriptor pvd;
    if (!ISO9660_READ_PVD(&pvd, sizeof(PrimaryVolumeDescriptor))) return FALSE;
    IsoDirectoryRecord root_iso;
    ISO9660_EXTRACT_ROOT_FROM_PVD(&pvd, &root_iso);
    return copy_iso_to_fat32(&root_iso, bpb.EXBR.ROOT_CLUSTER);
}

BOOLEAN ZERO_INITIALIZE_FAT32(VOIDPTR BOOTLOADER_BIN, U32 sz) {
    if (!WRITE_DISK_BPB()) return FALSE;
    if (!POPULATE_BOOTLOADER(BOOTLOADER_BIN, sz)) return FALSE;
    if (!CREATE_FSINFO()) return FALSE;
    if (!INITIAL_WRITE_FAT()) return FALSE;
    if (!CREATE_ROOT_DIR(bpb.EXBR.ROOT_CLUSTER)) return FALSE;
    if(!COPY_ISO9660_CONTENTS_TO_FAT32()) return FALSE;
    return TRUE;
}

BOOLEAN CREATE_CHILD_DIR(U32 parent_cluster, U8 *name, U8 ATTRIB, U32 *cluster_out) {
    if (!cluster_out || !name) return FALSE;
    *cluster_out = 0;

    // Allocate new cluster for directory
    U32 new_cluster = FIND_NEXT_FREE_CLUSTER();
    if (!new_cluster) return FALSE;
    fat32[new_cluster] = FAT32_END_OF_CHAIN;
    if (!FAT_FLUSH()) return FALSE;

    // Initialize '.' and '..' entries
    U8 *buf = KMALLOC(CLUSTER_SIZE);
    if(!buf) return FALSE;
    MEMZERO(buf, CLUSTER_SIZE);

    DIR_ENTRY dot = {0};
    DIR_ENTRY dotdot = {0};

    MEMCPY(dot.FILENAME, ".          ", 11);
    dot.ATTRIB = FAT_ATTRB_DIR;
    dot.LOW_CLUSTER_BITS = new_cluster & 0xFFFF;
    dot.HIGH_CLUSTER_BITS = (new_cluster >> 16) & 0xFFFF;

    MEMCPY(dotdot.FILENAME, "..         ", 11);
    dotdot.ATTRIB = FAT_ATTRB_DIR;
    dotdot.LOW_CLUSTER_BITS = parent_cluster & 0xFFFF;
    dotdot.HIGH_CLUSTER_BITS = (parent_cluster >> 16) & 0xFFFF;

    MEMCPY(buf, &dot, sizeof(DIR_ENTRY));
    MEMCPY(buf + sizeof(DIR_ENTRY), &dotdot, sizeof(DIR_ENTRY));

    if (!FAT_WRITE_CLUSTER(new_cluster, buf)) { Free(buf); return FALSE; }

    // Create directory entry in parent
    DIR_ENTRY entry;
    if (!CREATE_DIR_ENTRY(parent_cluster, name, FAT_ATTRB_DIR | ATTRIB, NULL, 0, &entry, new_cluster)) {
        Free(buf);
        return FALSE;
    }

    *cluster_out = new_cluster;
    Free(buf);
    return TRUE;
}

BOOLEAN CREATE_CHILD_FILE(U32 parent_cluster, U8 *name, U8 attrib, PU8 filedata, U32 filedata_size, U32 *cluster_out) {
    // Add directory entry for this new folder in parent
    DIR_ENTRY entry;
    attrib |= FAT_ATTRIB_ARCHIVE;
    if (!CREATE_DIR_ENTRY(parent_cluster, name, attrib, filedata, filedata_size, &entry, 0))
        return FALSE;
    *cluster_out = (entry.HIGH_CLUSTER_BITS << 16) | entry.LOW_CLUSTER_BITS;
    return TRUE;
}

VOID FREE_FAT_FS_RESOURCES() {
    if(fat32) Free(fat32);
}

U32 GET_ROOT_CLUSTER() {
    return bpb.EXBR.ROOT_CLUSTER;
}

BOOLEAN READ_LFNS(DIR_ENTRY *ent, LFN *out, U32 *size_out) {
    if (!ent || !out || !size_out) return FALSE;

    *size_out = 0;

    // Short entries are usually immediately after LFN entries in reverse order
    // We'll scan backwards in memory from the DIR_ENTRY location (assume contiguous cluster buffer)
    LFN *ptr = (LFN *)ent;
    I32 count = 0;

    while (count < MAX_LFN_COUNT) {
        ptr--; // move to previous 32-byte entry
        if (((DIR_ENTRY *)ptr)->ATTRIB != FAT_ATTRIB_LFN)
            break; // no more LFN entries

        out[count] = *(LFN *)ptr; // copy LFN entry
        count++;

        if (out[count - 1].ORDER & LFN_LAST_ENTRY)
            break; // reached first LFN in chain
    }

    if (count == 0)
        return FALSE;

    // Reverse order to get the correct sequence
    for (I32 i = 0; i < count / 2; i++) {
        LFN temp = out[i];
        out[i] = out[count - 1 - i];
        out[count - 1 - i] = temp;
    }

    *size_out = (U32)count;
    return TRUE;
}

BOOLEAN FIND_DIR_ENTRY_BY_CLUSTER_NUMBER(U32 cluster, DIR_ENTRY *out) {
    if (!out || cluster < FIRST_ALLOWED_CLUSTER_NUMBER) return FALSE;

    U8 *buf = KMALLOC(CLUSTER_SIZE);
    if(!buf) return FALSE;
    MEMZERO(buf, CLUSTER_SIZE);
    U32 current_cluster = cluster;

    while (current_cluster >= FIRST_ALLOWED_CLUSTER_NUMBER && current_cluster < FAT32_END_OF_CHAIN) {
        if (!FAT_READ_CLUSTER(current_cluster, buf)) return FALSE;

        for (U32 offset = 0; offset < CLUSTER_SIZE; offset += sizeof(DIR_ENTRY)) {
            DIR_ENTRY *entry = (DIR_ENTRY *)(buf + offset);

            if (entry->FILENAME[0] == 0x00) {
                // No more entries in this directory
                Free(buf);
                return FALSE;
            }

            if (entry->FILENAME[0] == FAT32_DELETED_ENTRY || entry->ATTRIB == FAT_ATTRIB_LFN) {
                // Skip deleted entries and LFN entries
                continue;
            }

            U32 entry_cluster = ((U32)entry->HIGH_CLUSTER_BITS << 16) | entry->LOW_CLUSTER_BITS;
            if (entry_cluster == cluster) {
                MEMCPY(out, entry, sizeof(DIR_ENTRY));
                Free(buf);
                return TRUE;
            }
        }

        // Move to next cluster in directory chain
        current_cluster = fat32[current_cluster];
        if (current_cluster >= FAT32_END_OF_CHAIN) break;
    }
    Free(buf);
    return FALSE;
}

BOOLEAN FIND_DIR_ENTRY_BY_NAME_AND_PARENT(DIR_ENTRY *out, U32 parent_cluster, U8 *name) {
    if (!out || !name || !*name) return FALSE;

    DIR_ENTRY entries[MAX_CHILD_ENTIES];
    U32 cnt = MAX_CHILD_ENTIES;
    if (!DIR_ENUMERATE(parent_cluster, entries, &cnt)) return FALSE;

    // FAT LFN entries appear *before* the main entry.
    LFN lfns[MAX_LFN_COUNT];
    U32 lfn_count = 0;

    for (U32 i = 0; i < cnt; i++) {
        if (entries[i].FILENAME[0] == 0x00)
            break; // end of directory

        if (entries[i].FILENAME[0] == FAT32_DELETED_ENTRY)
            continue; // deleted entry

        if (entries[i].ATTRIB == FAT_ATTRIB_LFN) {
            // collect LFN segment
            if (lfn_count < MAX_LFN_COUNT)
                MEMCPY(&lfns[lfn_count++], &entries[i], sizeof(DIR_ENTRY));
            continue;
        }

        // if we reach a normal entry, check accumulated LFNs first
        if (lfn_count > 0) {
            if (LFN_MATCH(name, lfns, lfn_count)) {
                MEMCPY(out, &entries[i], sizeof(DIR_ENTRY));
                return TRUE;
            }
            lfn_count = 0; // reset after using them
        }

        // fallback: compare short 8.3 name
        U8 shortname[13];
        MEMZERO(shortname, sizeof(shortname));

        // Copy base name (trim spaces)
        U32 j = 0;
        for (; j < 8 && entries[i].FILENAME[j] != ' ' && entries[i].FILENAME[j] != '\0'; j++)
            shortname[j] = entries[i].FILENAME[j];

        // Add dot + extension if present
        if (entries[i].FILENAME[8] != ' ' && entries[i].FILENAME[8] != '\0') {
            shortname[j++] = '.';
            for (U32 k = 0; k < 3 && entries[i].FILENAME[8 + k] != ' ' && entries[i].FILENAME[8 + k] != '\0'; k++)
                shortname[j++] = entries[i].FILENAME[8 + k];
        }
        shortname[j] = '\0';

        // FAT short names are uppercase; make case-insensitive compare
        if (STRICMP(shortname, name) == 0) {
            MEMCPY(out, &entries[i], sizeof(DIR_ENTRY));
            return TRUE;
        }
    }

    return FALSE;
}

U32 FIND_DIR_BY_NAME_AND_PARENT(U32 parent_cluster, U8 *name) {
    DIR_ENTRY e;
    if (FIND_DIR_ENTRY_BY_NAME_AND_PARENT(&e, parent_cluster, name)) {
        if (IS_FLAG_SET(e.ATTRIB, FAT_ATTRB_DIR)) {
            return ((U32)e.HIGH_CLUSTER_BITS << 16) | e.LOW_CLUSTER_BITS;
        }
    }
    return 0;
}

U32 FIND_FILE_BY_NAME_AND_PARENT(U32 parent_cluster, U8 *name) {
    DIR_ENTRY e;
    if (FIND_DIR_ENTRY_BY_NAME_AND_PARENT(&e, parent_cluster, name)) {
        if (!IS_FLAG_SET(e.ATTRIB, FAT_ATTRB_DIR)) {
            return ((U32)e.HIGH_CLUSTER_BITS << 16) | e.LOW_CLUSTER_BITS;
        }
    }
    return 0;
}

VOIDPTR READ_FILE_CONTENTS(U32 *size_out, DIR_ENTRY *ent) {
    VOIDPTR buf = NULL;
    *size_out = 0;
    if(IS_FLAG_UNSET(ent->ATTRIB, FAT_ATTRIB_ARCHIVE)) return buf;
    U32 file_size = ent->FILE_SIZE;
    if (file_size == 0)
        return buf;
    
    U32 clust = ((U32)ent->HIGH_CLUSTER_BITS << 16) | ent->LOW_CLUSTER_BITS;
    if (clust < FIRST_ALLOWED_CLUSTER_NUMBER)
        return buf;
    
    U32 cluster_size = GET_CLUSTER_SIZE();
    U32 clusters_needed = GET_CLUSTERS_NEEDED(cluster_size, file_size);
    U32 bytes = GET_CLUSTERS_NEEDED_IN_BYTES(clusters_needed, cluster_size);
    buf = KMALLOC(bytes);
    if(!buf) return NULL;
    MEMZERO(buf, bytes);
    
    U32 total_read = 0;
    U32 current_cluster = ent->LOW_CLUSTER_BITS | ent->HIGH_CLUSTER_BITS << 16;
    U8 *tmp_clust = KMALLOC(cluster_size);
    if(!tmp_clust) {
        Free(buf);
        return NULL;
    }
    MEMZERO(tmp_clust, cluster_size);
    
    while (current_cluster >= FIRST_ALLOWED_CLUSTER_NUMBER &&
           current_cluster < FAT32_END_OF_CHAIN &&
           total_read < file_size) {

        // Read current cluster
        if (!FAT_READ_CLUSTER(current_cluster, tmp_clust)) {
            Free(buf);
            Free(tmp_clust);
            return NULL;
        }

        // Copy data into output buffer
        U32 bytes_to_copy = MIN(cluster_size, file_size - total_read);
        MEMCPY(buf + total_read, tmp_clust, bytes_to_copy);
        total_read += bytes_to_copy;

        // Move to next cluster in chain
        current_cluster = fat32[current_cluster];
    }
    Free(tmp_clust);
    *size_out = total_read;
    return buf;
}

U32 FAT_CLUSTER_TO_LBA(U32 cluster) {
    if (cluster < FIRST_ALLOWED_CLUSTER_NUMBER) return 0; // invalid cluster
    U32 first_data_sector = bpb.RESERVED_SECTORS + (bpb.NUM_OF_FAT * bpb.EXBR.SECTORS_PER_FAT);
    return first_data_sector + (cluster - FIRST_ALLOWED_CLUSTER_NUMBER) * bpb.SECTORS_PER_CLUSTER;
}
U32 FAT_GET_NEXT_CLUSTER(U32 cluster) {
    if (cluster < FIRST_ALLOWED_CLUSTER_NUMBER) return 0;  // reserved/invalid
    U32 val = fat32[cluster];
    if (val >= FAT32_END_OF_CHAIN) return 0;               // end of chain
    if (val == FAT32_BAD_CLUSTER) return 0;                // bad cluster
    return val;
}
BOOL DIR_ENUMERATE_LFN(U32 dir_cluster, FAT_LFN_ENTRY *out_entries, U32 *max_count) {
    if (!out_entries || max_count == 0) return FALSE;

    U32 filled = 0;
    U8 sector_buf[BYTES_PER_SECT];
    LFN lfn_entries[MAX_LFN_COUNT];
    U32 lfn_count = 0;

    while (dir_cluster >= 2 && !FAT32_IS_EOC(dir_cluster)) {
        U32 lba = FAT_CLUSTER_TO_LBA(dir_cluster);

        // iterate over each sector
        for (U32 s = 0; s < bpb.SECTORS_PER_CLUSTER; s++) {
            if (!FAT_READ_SECTOR_FROM_DISK(lba + s, sector_buf)) return FALSE;

            // each sector has 16 entries (512 / 32)
            for (U32 i = 0; i < ENTRIES_PER_SECTOR; i++) {
                U8 *ptr = sector_buf + (i * sizeof(DIR_ENTRY));
                DIR_ENTRY *ent = (DIR_ENTRY *)ptr;

                if (ent->FILENAME[0] == 0x00) {
                    // End of directory
                    return TRUE;
                }

                if (ent->FILENAME[0] == FAT32_DELETED_ENTRY) {
                    // Deleted entry, skip and reset LFN chain
                    lfn_count = 0;
                    continue;
                }

                if (ent->ATTRIB == FAT_ATTRIB_LFN) {
                    // This is an LFN part — store it
                    LFN *lfn = (LFN *)ptr;
                    if (lfn_count < MAX_LFN_COUNT) {
                        lfn_entries[lfn_count++] = *lfn;
                    }
                    continue;
                }

                // Normal entry (file or directory)
                if (filled >= max_count)
                    return TRUE;

                // Build full LFN (if any collected)
                CHAR name[FAT_MAX_FILENAME];
                MEMZERO(name, FAT_MAX_FILENAME);

                if (lfn_count > 0) {
                    // Sort/rebuild from last to first LFN
                    for (I32 idx = (I32)lfn_count - 1; idx >= 0; idx--) {
                        LFN *lfn = &lfn_entries[idx];
                        U16 *segments[3] = { lfn->NAME_1, lfn->NAME_2, lfn->NAME_3 };
                        U32 seg_lengths[3] = { 5, 6, 2 };

                        for (U32 si = 0; si < 3; si++) {
                            for (U32 ci = 0; ci < seg_lengths[si]; ci++) {
                                U16 ch = segments[si][ci];
                                if (ch == 0x0000 || ch == 0xFFFF)
                                    goto done_lfn;
                                U32 len = STRLEN(name);
                                if (len < FAT_MAX_FILENAME - 1)
                                    name[len] = (CHAR)(ch & 0xFF); // ASCII truncation
                                else
                                    goto done_lfn;
                            }
                        }
                    }
                done_lfn:;
                } else {
                    // No LFN entries — use short 8.3
                    CHAR short_name[13];
                    U32 k = 0;
                    for (U32 j = 0; j < 8 && ent->FILENAME[j] != ' '; j++)
                        short_name[k++] = ent->FILENAME[j];
                    if (ent->FILENAME[8] != ' ')
                        short_name[k++] = '.';
                    for (U32 j = 8; j < 11 && ent->FILENAME[j] != ' '; j++)
                        short_name[k++] = ent->FILENAME[j];
                    short_name[k] = '\0';
                    STRCPY(name, short_name);
                }

                // Fill entry
                MEMCPY(&out_entries[filled].entry, ent, sizeof(DIR_ENTRY));
                STRCPY(out_entries[filled].lfn, name);
                filled++;

                // Reset LFN collector
                lfn_count = 0;
            }
        }

        // Move to next cluster
        dir_cluster = FAT_GET_NEXT_CLUSTER(dir_cluster);
    }
    *max_count = filled;
    return TRUE;
}

BOOL DIR_ENUMERATE(U32 dir_cluster, DIR_ENTRY *out_entries, U32 *max_count) {
    if (!out_entries || !max_count || *max_count == 0)
        return FALSE;

    U32 entries_filled = 0;
    U32 current_cluster = dir_cluster;
    U8 *cluster_buf = KMALLOC(CLUSTER_SIZE);
    if (!cluster_buf)
        return FALSE;

    BOOL in_lfn_sequence = FALSE;

    while (FAT32_IS_VALID(current_cluster) && entries_filled < *max_count) {
        MEMZERO(cluster_buf, CLUSTER_SIZE);

        if (!FAT_READ_CLUSTER(current_cluster, cluster_buf)) {
            Free(cluster_buf);
            return FALSE;
        }

        for (U32 i = 0; i < CLUSTER_SIZE / sizeof(DIR_ENTRY); i++) {
            DIR_ENTRY *ent = (DIR_ENTRY *)(cluster_buf + i * sizeof(DIR_ENTRY));

            // End of directory
            if (ent->FILENAME[0] == 0x00)
                goto done;

            // Deleted entry
            if (ent->FILENAME[0] == FAT32_DELETED_ENTRY) {
                in_lfn_sequence = FALSE;
                continue;
            }
            // LFN entry — acknowledge and skip
            if ((ent->ATTRIB & 0x3F) == FAT_ATTRIB_LFN) {
                in_lfn_sequence = TRUE;
                continue;
            }

            // Skip volume ID or label entries
            if (ent->ATTRIB & FAT_ATTRIB_VOL_ID) {
                in_lfn_sequence = FALSE;
                continue;
            }

            // Reset after LFN sequence
            if (in_lfn_sequence)
                in_lfn_sequence = FALSE;

            // Copy valid short entry
            MEMCPY(&out_entries[entries_filled], ent, sizeof(DIR_ENTRY));
            entries_filled++;
            if (entries_filled >= *max_count)
                goto done;
        }

        current_cluster = FAT_GET_NEXT_CLUSTER(current_cluster);
        if (FAT32_IS_EOC(current_cluster))
            break;
    }

done:
    *max_count = entries_filled;
    Free(cluster_buf);
    return TRUE;
}

U32 FILE_GET_SIZE(DIR_ENTRY *entry) {
    return entry->FILE_SIZE;
}

CHAR *STRTOK_R(CHAR *str, const CHAR *delim, CHAR **saveptr) {
    CHAR *start;

    if (str)
        start = str;
    else if (*saveptr)
        start = *saveptr;
    else
        return NULLPTR;

    // Skip leading delimiters
    while (*start && STRCHR(delim, *start)) start++;

    if (*start == '\0') {
        *saveptr = NULLPTR;
        return NULLPTR;
    }

    CHAR *token_end = start;
    while (*token_end && !STRCHR(delim, *token_end)) token_end++;

    if (*token_end) {
        *token_end = '\0';
        *saveptr = token_end + 1;
    } else {
        *saveptr = NULLPTR;
    }

    return start;
}

BOOL DIR_ENTRY_IS_FREE(DIR_ENTRY *entry) {
    if (!entry) return TRUE;
    // Free if first byte is 0x00 (never used) or FAT32_DELETED_ENTRY (deleted)
    return (entry->FILENAME[0] == 0x00 || entry->FILENAME[0] == FAT32_DELETED_ENTRY);
}

BOOLEAN PATH_RESOLVE_ENTRY(U8 *path, FAT_LFN_ENTRY *out_entry) {
    if (!path || !out_entry) return FALSE;

    MEMZERO(out_entry, sizeof(FAT_LFN_ENTRY));

    U32 current_cluster = GET_ROOT_CLUSTER();
    out_entry->lfn[0] = '\0';

    // Copy path safely
    U8 path_copy[FAT_MAX_PATH];
    STRNCPY(path_copy, path, FAT_MAX_PATH - 1);
    path_copy[FAT_MAX_PATH - 1] = '\0';

    if (STRCMP(path, "/") == 0) {
        out_entry->entry.LOW_CLUSTER_BITS  = GET_ROOT_CLUSTER() & 0xFFFF;
        out_entry->entry.HIGH_CLUSTER_BITS = (GET_ROOT_CLUSTER() >> 16) & 0xFFFF;
        out_entry->entry.ATTRIB = FAT_ATTRB_DIR;
        STRCPY(out_entry->lfn, "/");
        return TRUE;
    }

    U8 *saveptr = NULL;
    U8 *component = STRTOK_R(path_copy, "/", &saveptr);

    while (component) {
        // Skip empty components
        if (component[0] == '\0') {
            component = STRTOK_R(NULL, "/", &saveptr);
            continue;
        }

        // Enumerate directory
        DIR_ENTRY entries[MAX_CHILD_ENTIES];
        U32 actual_count = MAX_CHILD_ENTIES;
        if (!DIR_ENUMERATE(current_cluster, entries, &actual_count))
            return FALSE;

        BOOLEAN found = FALSE;
        for (U32 i = 0; i < actual_count; i++) {
            DIR_ENTRY *e = &entries[i];
            if (DIR_ENTRY_IS_FREE(e)) continue;

            // Retrieve name (LFN preferred, fallback to 8.3)
            U8 entry_name[FAT_MAX_FILENAME];
            MEMZERO(entry_name, sizeof(entry_name));

            if (!READ_LFNS(e, entry_name, sizeof(entry_name))) {
                // fallback: convert FILENAME[11] to string manually
                STRNCPY(entry_name, e->FILENAME, 11);
                entry_name[11] = '\0';
                str_trim(entry_name);
            }

            if (STRCMP(entry_name, component) == 0 || DIR_NAME_COMP_CASE(e, component)) {
                MEMCPY(&out_entry->entry, e, sizeof(DIR_ENTRY));
                STRNCPY(out_entry->lfn, entry_name, FAT_MAX_FILENAME - 1);
                out_entry->lfn[FAT_MAX_FILENAME - 1] = '\0';

                current_cluster = (e->HIGH_CLUSTER_BITS << 16) | e->LOW_CLUSTER_BITS;
                found = TRUE;
                break;
            }
        }

        if (!found) return FALSE;
        component = STRTOK_R(NULL, "/", &saveptr);
    }

    return TRUE;
}


BOOL DIR_ENTRY_IS_DIR(DIR_ENTRY *entry) {
    if(IS_FLAG_SET(entry->ATTRIB, FAT_ATTRB_DIR)) return TRUE;
    return FALSE;
}
U32 DIR_ENTRY_CLUSTER(DIR_ENTRY *entry) {
    U32 res = (entry->HIGH_CLUSTER_BITS << 16) | entry->LOW_CLUSTER_BITS;
    return res;
}

// Frees the entire FAT chain starting from start_cluster
BOOL FAT_FREE_CHAIN(U32 start_cluster) {
    if (start_cluster < FIRST_ALLOWED_CLUSTER_NUMBER) return FALSE;

    U32 cluster = start_cluster;
    while (cluster >= FIRST_ALLOWED_CLUSTER_NUMBER && cluster < FAT32_END_OF_CHAIN) {
        U32 next = FAT_GET_NEXT_CLUSTER(cluster);
        fat32[cluster] = FAT32_FREE_CLUSTER; // mark as free
        cluster = next;
    }

    return FAT_FLUSH();
}

// Truncates a FAT chain to new_size_clusters, freeing the rest
BOOL FAT_TRUNCATE_CHAIN(U32 start_cluster, U32 new_size_clusters) {
    if (start_cluster < FIRST_ALLOWED_CLUSTER_NUMBER) return FALSE;
    if (new_size_clusters == 0) return FAT_FREE_CHAIN(start_cluster);

    U32 cluster = start_cluster;
    U32 count = 1;

    while (cluster >= FIRST_ALLOWED_CLUSTER_NUMBER && cluster < FAT32_END_OF_CHAIN) {
        U32 next = FAT_GET_NEXT_CLUSTER(cluster);
        if (count == new_size_clusters) {
            // truncate here
            fat32[cluster] = FAT32_END_OF_CHAIN;
            // free remaining chain
            cluster = next;
            while (cluster >= FIRST_ALLOWED_CLUSTER_NUMBER && cluster < FAT32_END_OF_CHAIN) {
                U32 tmp = FAT_GET_NEXT_CLUSTER(cluster);
                fat32[cluster] = FAT32_FREE_CLUSTER;
                cluster = tmp;
            }
            break;
        }
        cluster = next;
        count++;
    }

    return FAT_FLUSH();
}


// Overwrite a file completely with new data
BOOL FILE_WRITE(DIR_ENTRY *entry, const U8 *data, U32 size) {
    if (!entry || !data) return FALSE;

    // Free existing cluster chain if any
    U32 start_cluster = (entry->HIGH_CLUSTER_BITS << 16) | entry->LOW_CLUSTER_BITS;
    if (start_cluster >= FIRST_ALLOWED_CLUSTER_NUMBER) {
        FAT_FREE_CHAIN(start_cluster);  // marks clusters as free in FAT
    }

    // Write new data into clusters
    if (!WRITE_FILEDATA(entry, data, size)) return FALSE;

    // Update file size
    entry->FILE_SIZE = size;
    FAT_UPDATETIMEDATE(entry);

    return TRUE;
}

// Append data to existing file
BOOL FILE_APPEND(DIR_ENTRY *entry, const U8 *data, U32 size) {
    if (!entry || !data || size == 0) return FALSE;

    U32 cluster_size = GET_CLUSTER_SIZE();
    U8 *buf = KMALLOC(cluster_size);
    if (!buf) return FALSE;

    U32 start_cluster = ((U32)entry->HIGH_CLUSTER_BITS << 16) | entry->LOW_CLUSTER_BITS;
    if (start_cluster == 0) {
        Free(buf);
        return FILE_WRITE(entry, data, size); // empty file
    }

    U32 last_cluster = FAT32_GetLastCluster(start_cluster);
    U32 used_bytes = entry->FILE_SIZE % cluster_size;

    const U8 *src = data;
    U32 remaining = size;

    // Fill last partially used cluster if needed
    if (used_bytes > 0 && used_bytes < cluster_size) {
        if (!FAT_READ_CLUSTER(last_cluster, buf)) {
            Free(buf);
            return FALSE;
        }

        U32 to_copy = MIN(cluster_size - used_bytes, remaining);
        MEMCPY(buf + used_bytes, src, to_copy);

        if (!FAT_WRITE_CLUSTER(last_cluster, buf)) {
            Free(buf);
            return FALSE;
        }

        src += to_copy;
        remaining -= to_copy;
    }

    // Allocate and write new clusters
    while (remaining > 0) {
        U32 new_cluster = FIND_NEXT_FREE_CLUSTER();
        if (new_cluster == 0) {
            Free(buf);
            return FALSE; // out of space
        }

        MEMZERO(buf, cluster_size);
        U32 to_copy = MIN(cluster_size, remaining);
        MEMCPY(buf, src, to_copy);

        if (!FAT_WRITE_CLUSTER(new_cluster, buf)) {
            Free(buf);
            return FALSE;
        }

        // Link into FAT chain after successful write
        fat32[last_cluster] = new_cluster;
        fat32[new_cluster] = FAT32_END_OF_CHAIN;

        src += to_copy;
        remaining -= to_copy;
        last_cluster = new_cluster;
    }

    // Flush FAT to disk
    if (!FAT_FLUSH()) {
        Free(buf);
        return FALSE;
    }

    // Update entry size and time
    entry->FILE_SIZE += size;
    FAT_UPDATETIMEDATE(entry);

    // Persist updated directory entry to disk
    // todo:
    // if (!UPDATE_DIR_ENTRY(entry)) {
    //     Free(buf);
    //     return FALSE;
    // }

    Free(buf);
    return TRUE;
}

// Removes a directory entry named `name` in the directory pointed by `entry`.
// Marks the entry as deleted and frees its clusters.
BOOL DIR_REMOVE_ENTRY(DIR_ENTRY *dir_entry, const char *name) {
    if (!dir_entry || !name) return FALSE;

    U32 dir_cluster = (dir_entry->HIGH_CLUSTER_BITS << 16) | dir_entry->LOW_CLUSTER_BITS;
    if (dir_cluster < FIRST_ALLOWED_CLUSTER_NUMBER) return FALSE;

    U8 *cluster_buf = KMALLOC(CLUSTER_SIZE);
    if (!cluster_buf) return FALSE;
    MEMZERO(cluster_buf, CLUSTER_SIZE);

    BOOL result = FALSE;

    while (dir_cluster >= FIRST_ALLOWED_CLUSTER_NUMBER && dir_cluster < FAT32_END_OF_CHAIN) {
        if (!FAT_READ_CLUSTER(dir_cluster, cluster_buf)) break;

        for (U32 offset = 0; offset < CLUSTER_SIZE; offset += sizeof(DIR_ENTRY)) {
            DIR_ENTRY *ent = (DIR_ENTRY *)(cluster_buf + offset);

            if (ent->FILENAME[0] == 0x00) goto cleanup; // end of entries
            if (ent->FILENAME[0] == FAT32_DELETED_ENTRY || ent->ATTRIB == FAT_ATTRIB_LFN)
                continue;

            if (STRCMP(ent->FILENAME, name) == 0) {
                // Free clusters if it’s a file or directory
                U32 start_cluster = (ent->HIGH_CLUSTER_BITS << 16) | ent->LOW_CLUSTER_BITS;
                if (start_cluster >= FIRST_ALLOWED_CLUSTER_NUMBER) {
                    FAT_FREE_CHAIN(start_cluster);
                }

                // Mark directory entry as deleted
                ent->FILENAME[0] = FAT32_DELETED_ENTRY;

                if (FAT_WRITE_CLUSTER(dir_cluster, cluster_buf) && FAT_FLUSH())
                    result = TRUE;

                goto cleanup;
            }
        }

        dir_cluster = fat32[dir_cluster];
    }

cleanup:
    Free(cluster_buf);
    return result;
}

// -----------------------------------------------------------------------------
// Returns the DIR_ENTRY representing the FAT32 root directory.
// The FAT32 root has no parent entry on disk, so we synthesize one.
// -----------------------------------------------------------------------------
DIR_ENTRY GET_ROOT_DIR_ENTRY(void)
{
    DIR_ENTRY root;
    MEMSET(&root, 0, sizeof(root));

    // The root directory cluster is stored in BPB (BIOS Parameter Block)
    root.FILENAME[0] = '/'; // optional visual marker
    root.ATTRIB = FAT_ATTRB_DIR;
    root.LOW_CLUSTER_BITS = (U16)(bpb.EXBR.ROOT_CLUSTER & 0xFFFF);
    root.HIGH_CLUSTER_BITS = (U16)(bpb.EXBR.ROOT_CLUSTER >> 16);
    root.FILE_SIZE = 0; // directories typically 0 for compatibility

    // optional: pad the filename field with spaces to be valid 8.3 format
    for (int i = 1; i < 11; ++i)
        root.FILENAME[i] = ' ';

    return root;
}
