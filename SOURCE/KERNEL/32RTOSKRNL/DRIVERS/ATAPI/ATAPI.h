#ifndef ATA_ATAPI_DRIVER_H
#define ATA_ATAPI_DRIVER_H

#include "../../../../STD/TYPEDEF.h"
#include "../ATA_SHARED.h"

#define ATAPI_MASTER  0xA0 // Master drive
#define ATAPI_SLAVE   0xB0 // Slave drive


#define ATAPI_SECTOR_SIZE 2048
#define ATAPI_SECTORS(num) (ATAPI_SECTOR_SIZE * (num))

#define ATAPI_PRIMARY_MASTER        'A'//1
#define ATAPI_PRIMARY_SLAVE         'B'//2
#define ATAPI_SECONDARY_MASTER      'C'//3
#define ATAPI_SECONDARY_SLAVE       'D'//4

//133c000 memdisk base
//1D3C000 memdisk end
// size ~10mb

/*+++
U32 ATAPI_CHECK(U0);

DESCRIPTION
    Checks which ATAPI CD-ROM drives are present.
    Save the results as it will be used later if you need to access data from ATAPI
    
RETURN
    ATA_FAILED
    ATAPI_PRIMARY_MASTER
    ATAPI_PRIMARY_SLAVE
    ATAPI_SECONDARY_MASTER
    ATAPI_SECONDARY_SLAVE

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/27 - Antonako1
        Initial version.
---*/
U32 ATAPI_CHECK(U0);

/*+++
U32 READ_CDROM(U32 ATAPICheckRes, U32 lba, U32 sectors, U8 *buf);

DESCRIPTION
    Reads sectors from a CD-ROM drive.

PARAMETERS
    ATAPICheckRes - The result of the ATAPI check to identify the drive.
    lba - The logical block address to start reading from.
    sectors - The number of sectors to read.
    buf - The buffer to store the read data.

RETURN
    ATA_FAILED - Failed to read from the CD-ROM.
    ATA_SUCCESS Successfully read from the CD-ROM.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/28 - Antonako1
        Initial version.

Remarks
    Buffer must be at least ATAPI_SECTOR_SIZE size in bytes
---*/
U32 READ_CDROM(U32 ATAPICheckRes, U32 lba, U32 sectors, U8 *buf);

#ifndef KERNEL_ENTRY

/// @brief Initializes the ATAPI interface.
/// @return Same as ATAPI_CHECK result.
/// @note This is same as calling ATAPI_CHECK, but it only runs once and saves the result for later use.
U32 INITIALIZE_ATAPI();

/// @brief Gets the ATAPI drive information.
/// @return Same as ATAPI_CHECK result.
U32 GET_ATAPI_INFO();

U32 ATAPI_CALC_SECTORS(U32 len);

#endif // KERNEL_ENTRY

#ifdef p4pwr
/*+++
ATAPI_MEMDISK sentinel value

DESCRIPTION
    Returned by INITIALIZE_ATAPI (and accepted by READ_CDROM) when a memdisk
    RAM-backed ISO image is detected. No separate MEMDISK_INIT call is needed;
    INITIALIZE_ATAPI tries the RAM path automatically before falling back to
    a real ATAPI drive.
---*/
#define ATAPI_MEMDISK 'P'
#endif // p4pwr

#endif // ATA_ATAPI_DRIVER_H