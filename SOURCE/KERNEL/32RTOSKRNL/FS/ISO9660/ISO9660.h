#ifndef ISO9660_H
#define ISO9660_H
/*
Define ISO9660_ONLY_DEFINES 
for only definitions
*/

#include "../../../STD/TYPEDEF.h"

#define ISO9660_SECTOR_SIZE 2048
#define ISO9660_VOLUME_DESCRIPTOR_START 16
#define ISO9660_PVD_OFFSET ((ISO9660_VOLUME_DESCRIPTOR_START*ISO9660_SECTOR_SIZE)/ISO9660_SECTOR_SIZE)
#define ISO9660_MAX_PATH 255
#define ISO9660_MAX_DIR_DEPTH 8
#define ISO9660_MAX_EXTENSION_LENGTH 3
#define ISO9660_MAX_FILENAME_LENGTH 8 // excluding extension and ;1
#define ISO9660_MAX_FULLNAME_LENGTH (ISO9660_MAX_FILENAME_LENGTH + 1 + ISO9660_MAX_EXTENSION_LENGTH + 2) // including . and ;1
#define ISO9660_MAX_DIR_RECORDS 64 // Max records in a directory. Practically could be more, but we limit it for simplicity
#define ISO9660_FILE_FLAG_DIRECTORY 0b00000010

typedef U8 strD;
typedef U8 strA;

typedef struct _DateTime {
    strD year[4];
    strD month[2];
    strD day[2];
    strD hour[2];
    strD minute[2];
    strD second[2];
    strD ms[2];
    U8 gmtOffset;
}
__attribute__((packed))
DateTime;

typedef struct _BootRecord {
    U8 type;
    U8 id[5];
    U8 version;
    U8 bootSystemIdentifier[32];
    U8 bootIdentifier[32];
    U8 bootSystemUse[1977];
}
__attribute__((packed))
BootRecord;

typedef struct _IsoDirectoryDateAndTime {
    U8 year;
    U8 month;
    U8 day;
    U8 hour;
    U8 minute;
    U8 second;
    I8 gmtOffset;
}
__attribute__((packed))
IsoDirectoryDateAndTime;

typedef U8 fileFlag;
// Directory Record
typedef struct _IsoDirectoryRecord {
    U8 length;                                         // +0
    U8 extendedAttributeRecordLength;                  // +1
    U32 extentLocationLE_LBA;                          // +2
    U32 extentLocationBE_LBA;                          // +6
    U32 extentLengthLE;                                // +10
    U32 extentLengthBE;                                // +14
    IsoDirectoryDateAndTime recordingDateAndTime;           // +18
    fileFlag fileFlags;                                     // +25
    U8 fileUnitSize;                                   // +26
    U8 interleaveGapSize;                              // +27
    U16 volumeSequenceNumberLE;                        // +28
    U16 volumeSequenceNumberBE;                        // +30
    U8 fileNameLength;                                 // +31
    strD fileIdentifier[1];
} 
__attribute__((packed))
IsoDirectoryRecord;

#define IDR_SIZE sizeof(IsoDirectoryRecord)

typedef struct _PrimaryVolumeDescriptor {
    U8 TypeCode;
    strA standardIdentifier[5];
    U8 version;
    U8 unused1;
    strA systemIdentifier[32];
    strD volumeIdentifier[32];
    U8 unused2[8];
    U32 volumeSpaceSizeLE;
    U32 volumeSpaceSizeBE;
    U8 unused3[32];
    U16 volumeSetSizeLE;
    U16 volumeSetSizeBE;
    U16 volumeSequenceNumberLE;
    U16 volumeSequenceNumberBE;
    U16 logicalBlockSizeLE;
    U16 logicalBlockSizeBE;
    U32 pathTableSizeLE;
    U32 pathTableSizeBE;
    U32 pathTableLocationLE;
    U32 optionalPathTableLocationLE;
    U32 LocationMPathBE;
    U32 optionalPathTableLocationBE;
    IsoDirectoryRecord rootDirectoryRecord;
    strD volumeSetIdentifier[128];
    strA publisherIdentifier[128];
    strA dataPreparerIdentifier[128];
    strA applicationIdentifier[128];
    strD copyrightFileIdentifier[37];
    strD abstractFileIdentifier[37];
    strD bibliographicFileIdentifier[37];
    DateTime creationDateAndTime;
    DateTime modificationDateAndTime;
    DateTime expirationDateAndTime;
    DateTime effectiveDateAndTime;
    U8 fileStructureVersion;
    U8 unused4;
    U8 applicationUsed[512];
    U8 reserved[653];
}
__attribute__((packed))
PrimaryVolumeDescriptor;

typedef struct {
    U8 nameLength;
    U8 extAttrRecord;
    U32 lbaLocation;
    U16 parentDirNum;
    U8 name[1];
} ATTRIB_PACKED PathTableEntry;

#ifndef ISO9660_ONLY_DEFINES

#define ISO9660_TYPE_DIRECTORY 0
#define ISO9660_TYPE_FILE      1
/// @brief Gets record type
/// @param rec Pointer to recod
/// @return ISO9660_TYPE_DIRECTORY or ISO9660_TYPE_FILE
U32 ISO9660_GET_TYPE(IsoDirectoryRecord *rec);

/// @brief Returns a list of all dir records
/// @param dir Directory to look into
/// @param recursive Not supported...
/// @param success TRUE is set on success
/// @return List of all dir records. fileNameLength is included!
IsoDirectoryRecord **ISO9660_GET_DIR_CONTENTS(IsoDirectoryRecord *dir, BOOLEAN recursive, U32 *size_out, U8 *success);

void ISO9660_FREE_LIST(IsoDirectoryRecord **ptr, U32 cnt);

/// @brief Check if the ISO9660 image is valid.
/// @param pvd Pointer to the PrimaryVolumeDescriptor to check.
/// @return TRUE if valid, FALSE otherwise.
BOOLEAN ISO9660_IMAGE_CHECK(PrimaryVolumeDescriptor *pvd);

/// @brief Read the primary volume descriptor from the ISO9660 image.
/// @param descriptor Pointer to the PrimaryVolumeDescriptor structure to fill.
/// @param size Size of the descriptor structure.
/// @return TRUE if successful, FALSE otherwise.
BOOLEAN ISO9660_READ_PVD(PrimaryVolumeDescriptor *descriptor, U32 size);

/// @brief Normalize the given path to conform to ISO9660 standards.
/// @param path Pointer to the path string to normalize.
/// @note By normalize, we mean converting path
///       separators to the ISO9660 standard ('/').
///       Example: "DIR1\DIR2\FILE.TXT" -> "DIR1/DIR2/FILE.TXT"
///       Example2: "DIR1/DIR2/FILE" -> "DIR1/DIR2/FILE;1"
///       The function modifies the path in place.
/// @return Pointer to the normalized path string, or NULLPTR on failure.
/// @note About the return value: If the function is called by kernel, free via KFREE.
///       If called by user program, free via free().
PU8 ISO9660_NORMALIZE_PATH(CHAR *path);

/// @brief Extract the root directory record from the primary volume descriptor.
/// @param pvd Pointer to the PrimaryVolumeDescriptor.
/// @param root Pointer to the IsoDirectoryRecord to fill with the root directory record.
void ISO9660_EXTRACT_ROOT_FROM_PVD(PrimaryVolumeDescriptor *pvd, IsoDirectoryRecord *root);

/// @brief Read a directory record from the ISO9660 filesystem.
/// @param path Path inside the ISO9660 image, formatted inside the function.
/// @param root_record Pointer to the root IsoDirectoryRecord structure.
/// @param out_record Pointer to the IsoDirectoryRecord structure to fill with the found record.
/// @return TRUE if successful, FALSE otherwise.
BOOLEAN ISO9660_READ_DIRECTORY_RECORD(CHAR *path, IsoDirectoryRecord *root_record, IsoDirectoryRecord *out_record);

/// @brief Reads file record from the ISO9660 image into memory.
/// @param path Path inside the ISO9660 image, formatted inside the function
/// @return Pointer to the allocated buffer containing the file data, or NULLPTR on failure.
IsoDirectoryRecord *ISO9660_FILERECORD_TO_MEMORY(CHAR *path);

/// @brief Reads file data from the ISO9660 image into memory.
/// @param fileptr Pointer to the IsoDirectoryRecord of the file to read.
/// @return Pointer to the allocated buffer containing the file data, or NULLPTR on failure.
VOIDPTR ISO9660_READ_FILEDATA_TO_MEMORY(IsoDirectoryRecord *fileptr);

/// @brief Frees the memory allocated by ISO9660 functions.
/// @param ptr Pointer to the memory to free.
void ISO9660_FREE_MEMORY(VOIDPTR ptr);

/// @brief Reads file into memory without having to read IsoDirectoryRecord
/// @param filename File to read
/// @param filesize_out Size of output
/// @return Pointer to buffer. Caller must free
VOIDPTR ISO9660_READ_FILEDATA_TO_MEMORY_QUICKLY(U8 *filename, U32 *filesize_out);

#endif // ISO9660_ONLY_DEFINES

#endif // ISO9660_H