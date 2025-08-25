; SOURCE/KERNEL/KERNEL_ENTRY_DATA.inc - Kernel entry data
;      
; Licensed under the MIT License. See LICENSE file in the project root for full license information.
;
; DESCRIPTION
;     Contains constants and global variables used by KERNEL_ENTRY.asm
;
; AUTHORS
;     Antonako1
; 
; REVISION HISTORY
;     2025/05/10 - Antonako1
;         - Created the file.
; 
; REMARKS
;     None
%ifndef KERNEL_ENTRY_DATA
%define KERNEL_ENTRY_DATA
%define BOOLEAN db

%define _MAX_PATH 255   ; Maximum path length
%define _MAX_DRIVE 3   ; Maximum drive length
%define _MAX_DIR  255   ; Maximum directory length
%define _MAX_FNAME 255 ; Maximum file name length
%define _MAX_EXT  255   ; Maximum file extension length

%define BIT_TO_BYTE(x) (x/2)
%define BYTE_TO_BIT(x) (x*8)

_32_bit_on: BOOLEAN 0


;%%%%%%%%%%%%%%%%%%%%%%%
; INT 15h, AX E820h, AX E801h
;%%%%%%%%%%%%%%%%%%%%%%%
SMAP equ 0x534D4150
SMAP_STR: db "SMAP", 4
E820_present: BOOLEAN 0
E801_present: BOOLEAN 0

num_of_e820_entries: dw 0   ; Number of entries in the E820 table. 
e820_entries_ptr: dd 0      ; Pointer to the E820 table in memory.

; E820 table entry structure
;
; Offset in Bytes		Name		Description
; 	 0	    BaseAddrLow		Low 32 Bits of Base Address
; 	 4	    BaseAddrHigh	High 32 Bits of Base Address
; 	 8	    LengthLow		Low 32 Bits of Length in Bytes
; 	 12	    LengthHigh		High 32 Bits of Length in Bytes
; 	 16	    Type		Address type of  this range.
;    20     ACPI        ACPI information. Only present if E820_ACPI is defined.
E820_start:
E820:
    BaseAddressLow: dd 0        ; Low 32 Bits of Base Address
    BaseAddressHigh: dd 0       ; High 32 Bits of Base Address 
    LengthLow: dd 0             ; Low 32 Bits of Length in Bytes
    LengthHigh: dd 0            ; High 32 Bits of Length in Bytes
    Type: dd 0                  ; Address type of this range.
%ifdef E820_ACPI
    ACPI: dd 0
%endif
E820_end:

%ifdef E820_ACPI
MEM_STRUCT_SIZE equ 24
%else
MEM_STRUCT_SIZE equ 20
%endif
MEM_STRUCT_SIZE_MAX equ 24

E820R: times MEM_STRUCT_SIZE_MAX db 0



; Type values. Any other value is undefined.
E820_TYPE_ADDRESS_RANGE_MEMORY equ 1        ; ARM-memory
E820_TYPE_ARM equ E820_TYPE_ADDRESS_RANGE_MEMORY

E820_TYPE_ADDRESS_RANGE_RESERVED equ 2      ; ARR-memory
E820_TYPE_ARR equ E820_TYPE_ADDRESS_RANGE_RESERVED

E820_TYPE_ADDRESS_RANGE_ACPI equ 3          ; ACPI
E820_TYPE_ACPI equ E820_TYPE_ADDRESS_RANGE_ACPI

E820_TYPE_ADDRESS_RANGE_NVS equ 4           ; NVS
E820_TYPE_NVS equ E820_TYPE_ADDRESS_RANGE_NVS

E820_TYPE_ADDRESS_RANGE_UNUSABLE equ 5
E820_TYPE_ARU equ E820_TYPE_ADDRESS_RANGE_UNUSABLE
    
; E820 entry structure in memory
E820_ENTRY_SEGMENT equ 0x0000
E820_ENTRY_OFFSET equ 0x8000
E820_MAX_SIZE equ 0x1000 ; 4096 bytes

DEBUG_PRINT_VALUE equ 0
; Address Range Descriptor Structure
; 
; Offset in Bytes		Name		Description
; 	 0	    BaseAddrLow		Low 32 Bits of Base Address
; 	 4	    BaseAddrHigh	High 32 Bits of Base Address
; 	 8	    LengthLow		Low 32 Bits of Length in Bytes
; 	 12	    LengthHigh		High 32 Bits of Length in Bytes
; 	 16	    Type		Address type of  this range.
; 
;
;   {
;       uint32_t BaseAddrLow;	    Low 32 Bits of Base Address
;       uint32_t BaseAddrHigh;	    High 32 Bits of Base Address
;       uint32_t LengthLow;	        Low 32 Bits of Length in Bytes
;       uint32_t LengthHigh;	    High 32 Bits of Length in Bytes
;       uint32_t Type;		        Address type of  this range.
;   }
; The BaseAddrLow and BaseAddrHigh together are the 64 bit BaseAddress of this range. The BaseAddress is the physical address of the start of the range being specified.
; 
; The LengthLow and LengthHigh together are the 64 bit Length of this range. The Length is the physical contiguous length in bytes of a range being specified.
; 
; The Type field describes the usage of the described address range as defined in the table below.





;%%%%%%%%%%%%%%%%%%%%%%%
; KRNL reading
;%%%%%%%%%%%%%%%%%%%%%%%
krnl_status: BOOLEAN 0
KRNL_STR_LEN equ 10
krnl_str: db "KRNL.BIN;1", KRNL_STR_LEN


; record is at (buffer+offset)
;  uint8_t length;                (buffer+offset+0). Record length
;  uint32_t extentLocationLE_LBA  (buffer+offset+2). LBA of data
;  uint32_t extentLengthLE;       (buffer+offset+10) Length of data
;  fileFlag fileFlags;            (buffer+offset+25). uint8_t. Fileflags. 0b00000010 if directory, 0b00000000 for file
;  uint8_t fileNameLength;        (buffer+offset+32). Length of fileIdentifier
;  strD fileIdentifier[1];        (buffer+offset+33). uint8_t. File name
;
KERNEL_LOAD_ADDRESS EQU 0x2000
KERNEL_LOAD_SEGMENT EQU 0x2000
KERNEL_LOAD_OFFSET EQU  0x0000

BUFFER_SEGMENT equ 0x0000
BUFFER_OFFSET equ 0x4000

%define SECTOR_SIZE             2048        ; 2KB per sector
%define VOL_DESC_START          16          ; Volume descriptor starts at sector 16
PVD_OFFSET equ              VOL_DESC_START*SECTOR_SIZE/SECTOR_SIZE

extentLocationLE_LBA: dd 0
extentLengthLE: dd 0
extenLocationLE_LBA_KRNL: dd 0
extentLengthLE_KRNL: dd 0
offset_var: dd 0
name_buf: db _MAX_PATH
buffer_var: dd 0
RETRY_COUNT_MAX equ 3 ; Maximum number of retries for reading files
retry_count db 0 ; retry count for reading files


; %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
; VESA/VBE information
; %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
VESA_LOAD_SEGMENT EQU 0x9000
VESA_LOAD_OFFSET  EQU 0x0000

; VESA controller info buffer (512 bytes)
VESA_CTRL_SIZE    EQU 512

; VBE mode info buffer immediately after VESA controller info
VBE_MODE_OFFSET   EQU VESA_LOAD_OFFSET + VESA_CTRL_SIZE
VBE_MODE_SIZE     EQU 256          ; typical size for mode info structure

VESA_TARGET_MODE  EQU 0x116   


; %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
; GDT/IDT
; %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GDT_SEGMENT EQU 0x9300
GDT_OFFSET  EQU 0x0000
GDT_SIZE    EQU 40             ; 5 entries × 8 bytes each

IDT_SEGMENT EQU 0x9400
IDT_OFFSET  EQU 0x0000
IDT_SIZE    EQU 2048           ; 256 entries × 8 bytes each


IDT_PTR:
    dw IDT_SIZE-1
    dd IDT_SEGMENT
IDT_PTR_END:

IDT:
    times 256 dq 0                 ; Fill with 0s for now (256 entries x 8 bytes)

IDT_end:

IDTR:
    dw IDT_end - IDT - 1           ; Limit
    dd IDT                         ; Base

gdt_start:
    dd 0x0
    dd 0x0
    dd 0x0000FFFF
    dd 0x00CF9A00
    dd 0x0000FFFF
    dd 0x00CF9200
gdt_end:

gdtr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

%endif ;KERNEL_ENTRY_DATA