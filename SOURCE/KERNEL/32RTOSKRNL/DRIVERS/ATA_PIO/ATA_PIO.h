#ifndef ATA_PIO_DRIVER_H
#define ATA_PIO_DRIVER_H

#include <STD/TYPEDEF.h>
#include <DRIVERS/ATA_SHARED.h>
#include <ATA_PIIX3/ATA_PIIX3.h> // For identifier functions

#define ATA_PRIMARY_MASTER          'E'
#define ATA_PRIMARY_SLAVE           'F'
#define ATA_SECONDARY_MASTER        'G'
#define ATA_SECONDARY_SLAVE         'H'

#define ATA_PIO_SECTOR_SIZE         512
#define ATA_PIO_SECTOR_WORDS        (ATA_PIO_SECTOR_SIZE / 2)
#define ATA_PIO_SECTOR_BYTES        ATA_PIO_SECTOR_SIZE
BOOLEAN ATA_PIO_INIT();

// See defines for return values
U32 ATA_PIO_GET_IDENTIFIER(VOID);
U32 ATA_PIO_IDENTIFY(VOID);

int ata_read_identify_to_buffer(U16 base_port, U8 drive, U16 *ident_out);

BOOLEAN ATA_PIO_READ_SECTORS_EXT(U8 device_id, U32 lba, U8 sector_count, VOIDPTR out_buffer);
BOOLEAN ATA_PIO_WRITE_SECTORS_EXT(U8 device_id, U32 lba, U8 sector_count, VOIDPTR in_buffer);

// Uses saved identifier
BOOLEAN ATA_PIO_READ_SECTORS(U32 lba, U8 sector_count, VOIDPTR out_buffer);
BOOLEAN ATA_PIO_WRITE_SECTORS(U32 lba, U8 sector_count, VOIDPTR in_buffer);

U32 ATA_CALC_SEC_COUNT(U32 bytes);

#define ATA_PIO_READ_CLUSTER(lba, in_buffer)    ATA_PIO_READ_SECTORS(lba, 1, in_buffer)
#define ATA_PIO_WRITE_CLUSTER(lba, out_buffer)  ATA_PIO_WRITE_SECTORS(lba, 1, out_buffer)
#endif // ATA_PIO_DRIVER_H