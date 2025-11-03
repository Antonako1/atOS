#ifndef FAT_H
#define FAT_H

/*
Define FAT_ONLY_DEFINES
for only definitions
*/

#include <STD/TYPEDEF.h>

/// @brief FAT32 Extended BIOS Parameter Block (EXBR) structure
typedef struct ATTRIB_PACKED {
    U32 SECTORS_PER_FAT;       ///< Number of sectors per FAT
    U16 EXT_FLAGS;             ///< FAT32 extended flags
    U16 FS_VERSION;            ///< FAT32 file system version
    U32 ROOT_CLUSTER;          ///< First cluster of root directory
    U16 FS_INFO_SECTOR;        ///< FSInfo sector number
    U16 BACKUP_BOOT_SECTOR;    ///< Backup boot sector location
    U8 RESERVED[12];           ///< Reserved
    U8 DRIVE_NUMBER;           ///< BIOS drive number
    U8 RESERVED1;              ///< Reserved
    U8 BOOT_SIGNATURE;         ///< Boot signature (0x29)
    U32 VOLUME_ID;             ///< Volume serial number
    U8 VOLUME_LABEL[11];       ///< Volume label
    U8 FILE_SYSTEM_TYPE[8];    ///< File system type string (e.g., "FAT32   ")
} EXBR_FAT32;

/// @brief BIOS Parameter Block (BPB) structure, common for FAT12/16/32
typedef struct ATTRIB_PACKED  {
    U8 JMPSHORT[3];            ///< Jump instruction to boot code
    U8 OEM_IDENT[8];            ///< OEM identifier string
    U16 BYTES_PER_SECTOR;       ///< Bytes per sector (usually 512)
    U8 SECTORS_PER_CLUSTER;     ///< Sectors per cluster
    U16 RESERVED_SECTORS;       ///< Reserved sectors before FAT
    U8 NUM_OF_FAT;              ///< Number of FAT copies
    U16 NUM_OF_ROOT_DIR_ENTRIES;///< Number of root directory entries (FAT12/16)
    U16 TOTAL_SECTORS;          ///< Total sectors (small count, use LARGE_SECTOR_COUNT if 0)
    U8 MEDIA_DESCR_TYPE;        ///< Media descriptor byte
    U16 NUM_OF_SECTORS_PER_FAT; ///< Sectors per FAT (FAT12/16)
    U16 NUM_OF_SECTORS_PER_TRACK;///< Sectors per track (for CHS addressing)
    U16 NUM_OF_HEADS;            ///< Number of heads (for CHS addressing)
    U32 NUM_OF_HIDDEN_SECTORS;   ///< Hidden sectors before partition
    U32 LARGE_SECTOR_COUNT;      ///< Total sectors (FAT32 or >65535 for FAT12/16)
    EXBR_FAT32    EXBR;    ///< FAT32-specific fields
} BPB;

/// @brief FAT32 FSInfo structure (sector providing free cluster count and next free cluster)
typedef struct ATTRIB_PACKED {
    U32 SIGNATURE0;             ///< First signature (0x41615252)
    U8 RES0[480];               ///< Reserved
    U32 SIGNATURE1;             ///< Second signature (0x61417272)
    U32 FREE_CLUSTER_COUNT;     ///< Number of free clusters on volume
    U32 CLUSTER_INDICATOR;      ///< Next free cluster to allocate
    U8 RES1[12];                ///< Reserved
    U32 TRAIL_SIGNATURE;        ///< Last signature (0xAA550000)
} FSINFO;

/// @brief Standard 32-byte FAT directory entry
typedef struct ATTRIB_PACKED {
    U8 FILENAME[11];            ///< Short 8.3 filename (padded with spaces)
    U8 ATTRIB;                  ///< File attributes (archive, dir, read-only, etc.)
    U8 RES;                     ///< Reserved
    U8 CREATION_TIME_HUN_SEC;   ///< Hundredths of a second at creation (0-199)
    U16 CREATION_TIME;           ///< Encoded creation time (hour 5bit, min 6bit, sec/2 5bit)
    U16 CREATION_DATE;           ///< Encoded creation date (year since 1980 7bit, month 4bit, day 5bit)
    U16 LAST_ACCESS_DATE;        ///< Last access date
    U16 HIGH_CLUSTER_BITS;       ///< High 16 bits of first cluster (FAT32 only)
    U16 LAST_MOD_TIME;           ///< Last modification time
    U16 LAST_MOD_DATE;           ///< Last modification date
    U16 LOW_CLUSTER_BITS;        ///< Low 16 bits of first cluster
    U32 FILE_SIZE;               ///< File size in bytes
} DIR_ENTRY;

/// @brief FAT file attribute flags
#define FAT_ATTRIB_READ_ONLY    0x01
#define FAT_ATTRIB_HIDDEN       0x02
#define FAT_ATTRIB_SYSTEM       0x04
#define FAT_ATTRIB_VOL_ID       0x08
#define FAT_ATTRB_DIR           0x10
#define FAT_ATTRIB_ARCHIVE      0x20
#define FAT_ATTRIB_LFN          (FAT_ATTRIB_READ_ONLY|FAT_ATTRIB_HIDDEN|FAT_ATTRIB_SYSTEM|FAT_ATTRIB_VOL_ID)


/// @brief Encode hour, minute, second into FAT 16-bit time
#define SET_TIME(h, m, s) \
    (U16)((((h) & 0x1F) << 11) | (((m) & 0x3F) << 5) | (((s) / 2) & 0x1F))

/// @brief Encode year, month, day into FAT 16-bit date
#define SET_DATE(y, m, d) \
    (U16)(((((y) - 1980) & 0x7F) << 9) | (((m) & 0x0F) << 5) | ((d) & 0x1F))

/// @brief Long File Name (LFN) directory entry (32 bytes)
typedef struct ATTRIB_PACKED  {
    U8  ORDER;           ///< Sequence number (bit 6 = 1 if last entry)
    U16 NAME_1[5];       ///< First 5 UTF-16 characters of the long name
    U8  ATTR;            ///< Must be 0x0F
    U8  TYPE;            ///< Must be 0x00
    U8  CHECKSUM;        ///< Checksum of short 8.3 name
    U16 NAME_2[6];       ///< Next 6 UTF-16 characters
    U16 ALWAYS_ZERO;   ///< Always 0x0000
    U16 NAME_3[2];       ///< Final 2 UTF-16 characters
} LFN;



/// @brief FAT path and name limits
#define FAT_MAX_PATH     1024
#define FAT_MAX_FILENAME 260    // Includes extension. 260 to allow 13 LFNs
#define MAX_NESTED_DIRS  32
#define MAX_CHILD_ENTIES 128

#define CHARS_PER_LFN 13
#define LFN_LAST_ENTRY (1<<6)
#define MAX_LFN_COUNT    (FAT_MAX_FILENAME / CHARS_PER_LFN)

typedef struct {
    DIR_ENTRY entry;             ///< Standard directory entry
    CHAR lfn[FAT_MAX_FILENAME];  ///< Full long file name (ASCII, null-terminated)
    U32 parent_cluster;  // not stored on disk — runtime info
    U32 dir_offset;      // offset within the parent directory cluster
} FAT_LFN_ENTRY;




#ifndef FAT_ONLY_DEFINES

#define FIRST_ALLOWED_CLUSTER_NUMBER 2

// FAT32 special cluster markers
#define FAT32_FREE_CLUSTER       0x00000000
#define FAT32_RESERVED_CLUSTER   0xFFFFFFFF
#define FAT32_BAD_CLUSTER        0x0FFFFFF7
#define FAT32_END_OF_CHAIN       0x0FFFFFF8

#define FAT32_DELETED_ENTRY      0xE5

#define SECT_PER_CLUST     8
#define BYTES_PER_SECT     512
#define ENTRIES_PER_SECTOR (BYTES_PER_SECT / sizeof(DIR_ENTRY))
#define CLUSTER_SIZE        (SECT_PER_CLUST * BYTES_PER_SECT)

// Cluster value range checks
#define FAT32_IS_EOC(c)    ((c) >= 0x0FFFFFF8)
#define FAT32_IS_VALID(c)  ((c) >= 2 && (c) <= 0x0FFFFFEF)

// =======================
// FAT32 Filesystem API
// =======================

// ----- Boot Parameter Block (BPB) management -----

BPB *GET_BPB(); 
// Returns pointer to the currently loaded BPB structure.

BOOLEAN LOAD_BPB(); 
// Reads the BPB (boot sector) and FSInfo from disk into memory.

BOOLEAN GET_BPB_LOADED(); 
// Returns TRUE if BPB has been successfully loaded.

VOID FREE_FAT_FS_RESOURCES(); 
// Frees any allocated buffers or FAT tables held in memory.

BOOLEAN ZERO_INITIALIZE_FAT32(VOIDPTR BOOTLOADER_BIN, U32 sz); 
// Initializes a new FAT32 filesystem on a blank disk. 
// Writes BPB, FSInfo, empty FATs, and creates the root directory.

// ----- Directory and file lookup -----

U32 GET_ROOT_CLUSTER();

U32 FIND_DIR_BY_NAME_AND_PARENT(U32 parent, U8 *name); 
// Searches for a subdirectory with the given name inside a parent directory.
// Returns its starting cluster, or 0 if not found.

U32 FIND_FILE_BY_NAME_AND_PARENT(U32 parent, U8 *name);
// Searches for a file with the given name inside a parent directory.
// Returns its starting cluster, or 0 if not found.

BOOLEAN FIND_DIR_ENTRY_BY_NAME_AND_PARENT(DIR_ENTRY *out, U32 parent, U8 *name);
// Fills 'out' with the directory entry structure of the given name in 'parent' directory.
// Returns TRUE on success

/// @brief Reads all LFN entries preceding a DIR_ENTRY and reconstructs the name.
/// @param ent The short DIR_ENTRY for which we want LFNs
/// @param out Preallocated buffer of LFN entries (size should allow MAX_LFN_COUNT)
/// @param size_out Number of LFN entries found
/// @return TRUE if any LFN entries were found, FALSE otherwise
BOOLEAN READ_LFNS(DIR_ENTRY *ent, LFN *out, U32 *size_out);

// Returns the DIR_ENTRY representing the FAT32 root directory.
// The FAT32 root has no parent entry on disk, so we synthesize one.
DIR_ENTRY GET_ROOT_DIR_ENTRY(void);

// ----- Directory and file creation -----

BOOLEAN CREATE_CHILD_DIR(U32 parent_cluster, U8 *name, U8 attrib, U32 *cluster_out);
// Creates a new subdirectory under the given parent directory.
// Allocates a free cluster, zeroes it, and writes "." and ".." entries.

BOOLEAN CREATE_CHILD_FILE(U32 parent_cluster, U8 *name, U8 attrib, PU8 filedata, U32 filedata_size, U32 *cluster_out);
// Creates a new empty file in the specified parent directory.
// Allocates a directory entry but does not allocate data clusters yet.


// ----- Directory management -----

BOOL DIR_ENUMERATE_LFN(U32 dir_cluster, FAT_LFN_ENTRY *out_entries, U32 *max_count);
// Reads all valid directory entries from a directory cluster chain.
// Fills up to 'max_count' entries in 'out_entries'. Returns TRUE if successful.
// Count returned in max_count

BOOL DIR_ENUMERATE(U32 dir_cluster, DIR_ENTRY *out_entries, U32 *max_count);
// Reads all valid directory entries from a directory cluster chain.
// Fills up to 'max_count' entries in 'out_entries'. Returns TRUE if successful.
// Count returned in max_count

BOOL DIR_REMOVE_ENTRY(FAT_LFN_ENTRY *entry, const char *name);
// Deletes a file or directory entry from 'entry'.
// Marks the entry as deleted (0xE5) and frees associated clusters if needed.

// ----- FAT management -----

BOOL FAT_FREE_CHAIN(U32 start_cluster);
// Walks the FAT chain starting at 'start_cluster' and marks all clusters as free.

BOOL FAT_TRUNCATE_CHAIN(U32 start_cluster, U32 new_size_clusters);
// Keeps the first 'new_size_clusters' in a chain, frees the rest.

// ----- File data operations -----

VOIDPTR READ_FILE_CONTENTS(U32 *size_out, DIR_ENTRY *entry);
// Reads the entire contents of a file into a newly allocated buffer.
// Returns pointer to buffer and sets *size_out to file size in bytes.

BOOL FILE_WRITE(FAT_LFN_ENTRY *entry, const U8 *data, U32 size);
// Writes 'size' bytes to a file’s cluster chain, allocating new clusters if necessary.

BOOL FILE_APPEND(FAT_LFN_ENTRY *entry, const U8 *data, U32 size);
// Appends data to the end of a file’s cluster chain, extending it if needed.

U32 FILE_GET_SIZE(DIR_ENTRY *entry);
// Returns the file size in bytes from a directory entry.

// ----- Path handling -----

BOOLEAN PATH_RESOLVE_ENTRY(U8 *path, FAT_LFN_ENTRY *out_entry);
// Resolves a full path (e.g. "/DIR1/DIR2/FILE.TXT") to its entry. (FILE.TXT in this case)
// Returns 0 if not found.

// ----- Directory entry utilities -----

BOOL DIR_ENTRY_IS_FREE(DIR_ENTRY *entry);
// Returns TRUE if entry is unused or deleted (FILENAME[0] == 0x00 or 0xE5).

BOOL DIR_ENTRY_IS_DIR(DIR_ENTRY *entry);
// Returns TRUE if entry represents a directory (ATTRIB has FAT_ATTRB_DIR).

#endif // FAT_ONLY_DEFINES

#endif // FAT_H
