/**
 * AC_FH.h - AstraC binary file header format definition
 */
#ifndef AC_FH_H
#define AC_FH_H
#include <STD/STRING.h>

#define OFFSET_NON_EXISTENT ((U32)(-1))

#define AC_FILE_MAGIC "ACFH"
#define AC_FILE_MAGIC_LEN 4
#define AC_FILE_RESERVED_SIZE 64
#define AC_FILE_VERSION_MAJOR (U16)1
#define AC_FILE_VERSION_MINOR (U16)0

#define AC_FILE_VERSION ((U32)AC_FILE_VERSION_MAJOR << 16 | AC_FILE_VERSION_MINOR)

typedef struct {
    U8  magic[AC_FILE_MAGIC_LEN];   /* "ACFH" */
    U32 version;    /* file format version */

    U32 entry_point_offset;/* offset to entry point code */

    
    U32 code_offset; /* offset to code section */
    U32 data_offset; /* offset to initialized data section */
    U32 rodata_offset; /* offset to read-only data section */
    U32 bss_offset;  /* offset to uninitialized data section (BSS) */

    U32 code_size;  /* size of code section in bytes */
    U32 data_size;  /* size of data section in bytes */
    U32 rodata_size;/* size of read-only data section in bytes */
    U32 bss_size;   /* size of uninitialized data section in bytes */
    
    U8 reserved[AC_FILE_RESERVED_SIZE]; /* reserved for future use, should be zeroed */
} ATTRIB_PACKED AC_FILE_HEADER;



#endif // AC_FH_H