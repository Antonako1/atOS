/**
 * @file FS_DISK.h
 * @author Antonako1
 * @brief Filesystem and Disk IO functions
 */
#ifndef STD_FS_DISK_H
#define STD_FS_DISK_H

#include <STD/TYPEDEF.h>

#ifndef ISO9660_ONLY_DEFINES
#define ISO9660_ONLY_DEFINES
#endif 
#include <FS/ISO9660/ISO9660.h> // For ISO9660 filesystem types
#ifndef FAT_ONLY_DEFINES
#define FAT_ONLY_DEFINES
#endif 
#include <FS/FAT/FAT.h>     // For FAT32 filesystem types



/*+++
Standard file manipulation functions.
Raw ISO9660 and FAT32 functions are below
---*/
typedef enum {
    MODE_R        = 0x0001,  // Read
    MODE_W        = 0x0002,  // Write
    MODE_RW       = MODE_R | MODE_W,   // Read & Write
    MODE_A        = 0x0008,  // Append
    MODE_RA       = MODE_R | MODE_A,
    MODE_FAT32    = 0x0100,  // FAT32 backend
    MODE_ISO9660  = 0x0200,  // ISO9660 backend
    MODE_FR       = MODE_R | MODE_FAT32, // FAT32 Read
    MODE_FW       = MODE_W | MODE_FAT32, // FAT32 Write
    MODR_FRW      = MODE_RW| MODE_FAT32, // FAT32 Read & Write
    MODE_FA       = MODE_A | MODE_FAT32, // FAT32 Append
    MODE_FRA      = MODE_RA| MODE_FAT32, // FAT32 Read & Append
} FILEMODES;

typedef struct {
    VOIDPTR data;        // Raw data (allocated or mapped file buffer)
    U32 sz;              // File size in bytes
    U32 read_ptr;        // Current read position
    FILEMODES mode;
    union {
        FAT_LFN_ENTRY fat_ent;
        IsoDirectoryRecord iso_ent;
    } ent;
} ATTRIB_PACKED FILE;

// ===================================================
// File I/O Abstraction Functions
// Supports FAT32 (read/write) and ISO9660 (read-only) modes
// ===================================================

// Open a file in the specified mode (FAT32 or ISO9660)
FILE *FOPEN(PU8 path, FILEMODES mode);

// Close a file and free associated memory
VOID FCLOSE(FILE *file);

// Read up to `len` bytes from file into `buffer`
// Returns number of bytes actually read (0 at EOF)
U32 FREAD(FILE *file, VOIDPTR buffer, U32 len);

// Write up to `len` bytes from `buffer` into file
// Returns number of bytes written; always 0 for read-only (ISO9660) files
// To write file's buffer, point parameters to file.data and file.sz
U32 FWRITE(FILE *file, VOIDPTR buffer, U32 len);

// Move the read pointer to `offset` bytes from the beginning
// Returns TRUE if seek successful, FALSE if offset is out of bounds
BOOLEAN FSEEK(FILE *file, U32 offset);

// Return the current read pointer offset
U32 FTELL(FILE *file);

// Reset read pointer to the beginning of the file
VOID FREWIND(FILE *file);

// Return the total size of the file in bytes
U32 FSIZE(FILE *file);

// Check if the read pointer is at or past the end of the file
BOOLEAN FILE_EOF(FILE *file);

// Read a single line (up to `max_len - 1` bytes) from file into `line`
// Line includes newline character if present
// Returns TRUE if a line was read, FALSE if EOF or empty
BOOLEAN FILE_GET_LINE(FILE *file, PU8 line, U32 max_len);

// ===================================================
// File / Directory Management
// ===================================================

// Delete a file from the filesystem
// Returns TRUE on success, FALSE if file does not exist or cannot be deleted
BOOLEAN FILE_DELETE(PU8 path);

// Delete a directory
// If `force` is TRUE, may remove non-empty directories
// Returns TRUE on success
BOOLEAN DIR_DELETE(PU8 path, BOOLEAN force);

// Check if a file exists
BOOLEAN FILE_EXISTS(PU8 path);

// Check if a directory exists
BOOLEAN DIR_EXISTS(PU8 path);

// Create a new empty file (FAT32 only)
// Returns TRUE if file was created successfully
BOOLEAN FILE_CREATE(PU8 path);

// Create a new directory (FAT32 only)
// Returns TRUE if directory was created successfully
BOOLEAN DIR_CREATE(PU8 path);

// Truncate or expand a file to `new_size` bytes (FAT32 only)
// Returns TRUE if operation succeeded
BOOLEAN FILE_TRUNCATE(FILE *file, U32 new_size);

// Flush in-memory file data to disk (FAT32 only)
// Returns TRUE on success
BOOLEAN FILE_FLUSH(FILE *file);

// ===================================================
// Raw File Initialization
// Used to create FILE structures from in-memory FAT32 or ISO9660 data
// ===================================================

// Initialize a FILE object from raw FAT32 data and directory entry
BOOLEAN FILE_FROM_RAW_FAT_DATA(FILE *file, VOIDPTR data, U32 sz, FAT_LFN_ENTRY *ent);

// Initialize a FILE object from raw ISO9660 data and directory record
BOOLEAN FILE_FROM_RAW_ISO_DATA(FILE *file, VOIDPTR data, U32 sz, IsoDirectoryRecord *ent);


/**
 * ISO9660
 */

/// @brief Reads an ISO9660 file record from the filesystem.
/// @param path Path to the file/ directory inside the ISO9660 image
/// @note Path must NOT be ISO9660 normalized, the function will do it.
/// @note MFree the returned pointer with FREE_ISO9660_MEMORY when done.
/// @return Pointer to the file record structure or NULL on failure
IsoDirectoryRecord *READ_ISO9660_FILERECORD(CHAR *path);

/// @brief Reads the contents of an ISO9660 file.
/// @param dir_ptr Pointer to the file record structure
/// @note MFree the returned pointer with FREE_ISO9660_MEMORY when done.
/// @return Pointer to the file contents or NULL on failure
VOIDPTR READ_ISO9660_FILECONTENTS(IsoDirectoryRecord *dir_ptr);

/// @brief Frees memory allocated by ISO9660 functions.
/// @param ptr Pointer to the memory to free
/// @note Not really necessary, just legacy code... MFree does the same job.
VOID FREE_ISO9660_MEMORY(VOIDPTR ptr);

U32 FAT32_GET_ROOT_CLUSTER();

// Searches for a subdirectory with the given name inside a parent directory.
// Returns its starting cluster, or 0 if not found.
U32 FAT32_FIND_DIR_BY_NAME_AND_PARENT(U32 parent, U8 *name); 

// Searches for a file with the given name inside a parent directory.
// Returns its starting cluster, or 0 if not found.
U32 FAT32_FIND_FILE_BY_NAME_AND_PARENT(U32 parent, U8 *name);

// Fills 'out' with the directory entry structure of the given name in 'parent' directory.
// Returns TRUE on success
BOOLEAN FAT32_FIND_DIR_ENTRY_BY_NAME_AND_PARENT(DIR_ENTRY *out, U32 parent, U8 *name);

/// @brief Reads all LFN entries preceding a DIR_ENTRY and reconstructs the name.
/// @param ent The short DIR_ENTRY for which we want LFNs
/// @param out Preallocated buffer of LFN entries (size should allow MAX_LFN_COUNT)
/// @param out_size size of `out`. If out != sizeof(LFN) * MAX_LFN_COUNT, false is returned
/// @param size_out Number of LFN entries found
/// @return TRUE if any LFN entries were found, FALSE otherwise
BOOLEAN FAT32_READ_LFNS(DIR_ENTRY *ent, LFN *out, U32 out_size, U32 *size_out);

// Creates a new subdirectory under the given parent directory.
// Allocates a free cluster, zeroes it, and writes "." and ".." entries.
BOOLEAN FAT32_CREATE_CHILD_DIR(U32 parent_cluster, U8 *name, U8 attrib, U32 *cluster_out);

// Creates a new empty file in the specified parent directory.
// Allocates a directory entry but does not allocate data clusters yet.
// Filedata MUST be heap allocated
BOOLEAN FAT32_CREATE_CHILD_FILE(U32 parent_cluster, U8 *name, U8 attrib, PU8 filedata, U32 filedata_size, U32 *cluster_out);

// Reads all valid directory entries from a directory cluster chain.
// Fills up to 'max_count' entries in 'out_entries'. Returns TRUE if successful.
// Count returned in max_count
BOOL FAT32_DIR_ENUMERATE_LFN(U32 dir_cluster, FAT_LFN_ENTRY *out_entries, U32 *max_count);

// Reads all valid directory entries from a directory cluster chain.
// Fills up to 'max_count' entries in 'out_entries'. Returns TRUE if successful.
// Count returned in max_count
BOOL FAT32_DIR_ENUMERATE(U32 dir_cluster, DIR_ENTRY *out_entries, U32 *max_count);

// Deletes a file or directory entry from 'entry'.
// Marks the entry as deleted (0xE5) and frees associated clusters if needed.
BOOL FAT32_DIR_REMOVE_ENTRY(FAT_LFN_ENTRY *entry, const char *name);

// Reads the entire contents of a file into a newly allocated buffer.
// Returns pointer to buffer and sets *size_out to file size in bytes.
VOIDPTR FAT32_READ_FILE_CONTENTS(U32 *size_out, DIR_ENTRY *entry);

// Writes 'size' bytes to a file’s cluster chain, allocating new clusters if necessary.
BOOL FAT32_FILE_WRITE(FAT_LFN_ENTRY *entry, const U8 *data, U32 size);

// Appends data to the end of a file’s cluster chain, extending it if needed.
BOOL FAT32_FILE_APPEND(FAT_LFN_ENTRY *entry, const U8 *data, U32 size);

// Returns the file size in bytes from a directory entry.
U32 FAT32_FILE_GET_SIZE(DIR_ENTRY *entry);

// Resolves a full path (e.g. "/DIR1/DIR2/FILE.TXT") to its entry. (FILE.TXT in this case)
// Returns 0 if not found.
BOOLEAN FAT32_PATH_RESOLVE_ENTRY(U8 *path, FAT_LFN_ENTRY *out_entry);

// Returns TRUE if entry is unused or deleted (FILENAME[0] == 0x00 or 0xE5).
BOOL FAT32_DIR_ENTRY_IS_FREE(DIR_ENTRY *entry);

// Returns TRUE if entry represents a directory (ATTRIB has FAT_ATTRB_DIR).
BOOL FAT32_DIR_ENTRY_IS_DIR(DIR_ENTRY *entry);

// Returns the DIR_ENTRY representing the FAT32 root directory.
// The FAT32 root has no parent entry on disk, so we synthesize one.
// On error, empty struct is returned
DIR_ENTRY FAT32_GET_ROOT_DIR_ENTRY(void);

VOID FAT_DECODE_TIME(U16 time, U16 date, U32 *year, U32 *month, U32 *day, U32 *hour, U32 *minute, U32 *second);

/**
 * Raw Disk IO functions
 */
/// @brief Reads sectors from main CD-ROM drive.
/// @param lba Logical Block Address to start reading from
/// @param sectors Number of sectors to read
/// @param buf Buffer to store read data
/// @note Buffer must be at least 2048 bytes (CD-ROM sector size)
/// @return Non zero on success, zero on failure
U32 CDROM_READ(U32 lba, U32 sectors, U8 *buf);

/// @brief Reads from a hard disk
/// @param lba Lba to read from disk
/// @param sectors Sectors to read (Max 256)
/// @param buf Buffer, must (sectors * 512)
/// @return Non zero on success, zero on failure
U32 HDD_READ(U32 lba, U32 sectors, U8 *buf);

/// @brief Reads from a hard disk
/// @param lba Lba to write to disk
/// @param sectors Sectors to write (Max 256)
/// @param buf Buffer, must (sectors * 512)
/// @return Non zero on success, zero on failure
U32 HDD_WRITE(U32 lba, U32 sectors, U8 *buf);

#undef ISO9660_ONLY_DEFINES
#undef FAT_ONLY_DEFINES
#endif // STD_FS_DISK_H