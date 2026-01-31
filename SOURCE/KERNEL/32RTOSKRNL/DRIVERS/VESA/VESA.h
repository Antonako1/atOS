/*+++
    Source/KERNEL/32RTOSKRNL/DRIVERS/VESA/VESA.h - VESA Definitions

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    VESA (Video Electronics Standards Association) definitions for atOS.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version. Contains VESA information structure and constants.

REMARKS
    None

DESCRIPTION
    VESA (Video Electronics Standards Association) definitions for atOS.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/05/26 - Antonako1
        Created VESA information structure and constants.    

REMARKS
    This file should not be included alone, as VESA does not work without VBE.
    Use VBE.h to include VESA definitions in your application.

    Note: If building outside KERNEL, ensure that the external definitions are defined,
    if not, define them in your application's source file.
        See below for external definitions.
    
    Access VESA struct at memory address VESA_LOAD_ADDRESS_PHYS.

    When compiling, include VESA.c
---*/
#ifndef VESA_H
#define VESA_H
#include "../../../../STD/TYPEDEF.h" // TYPE DEFINITIONS

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define SCREEN_CENTER_X (SCREEN_WIDTH/2)
#define SCREEN_CENTER_Y (SCREEN_HEIGHT/2)

#if defined(__RTOS__) || defined(KERNEL_ENTRY)
#include "../../MEMORY/MEMORY.h"

#define RM2LA(seg, off)  (((U32)(seg) << 4) + (U32)(off))
#define FAR_PTR_TO_LINEAR(ptr)  RM2LA(((ptr) >> 16) & 0xFFFF, (ptr) & 0xFFFF)

/*+++
external definitions
---*/
#define VIDEO_MODE_VESA 0x00000001 // VESA mode flag
#define VIDEO_MODE_VBE  0x00000002 // VBE mode flag

// #define VIDEO_MEMORY ((volatile U16*)0x00F00000)
#define VIDEO_MEMORY ((volatile U16*)0xB8000) // Text mode video memory address


/* Foreground/Background colors */
#define COLOR_GREEN_ON_BLACK 0x0A

typedef struct {
    CHAR Signature[4];       // "VESA" signature (should be "VESA")
    U16 Version;        // VBE version (e.g., 0x0300 = 3.0)
    U32 OEMStringPtr;   // Pointer to an OEM string (e.g., manufacturer)
    U32 Capabilities;   // Flags: e.g., DAC width, graphics mode support
    U32 VideoModePtr;   // Pointer to a list of supported video modes (16-bit mode numbers)
    U16 TotalMemory;    // Number of 64KB blocks of video memory
    U16 Reserved[5];    // Reserved, must be ignored

    U32 OemSoftwareRev;      // OEM software revision
    U32 OemVendorNamePtr;    // Far pointer to vendor name string
    U32 OemProductNamePtr;   // Far pointer to product name string
    U32 OemProductRevPtr;    // Far pointer to product revision string
    U32 VBE_AF_VERSION;
    DWORD VideoModes;      // Current video mode (16-bit mode number)
    U8 VBE_IMPLEMENTATION[216];
    U8 OEM_SCRATCHPAD[256]; // OEM scratchpad for additional data 
} __attribute__((packed)) VESA_INFO;

#define VESA_LOAD_ADDRESS_SEGMENT 0x6400 / 16
#define VESA_LOAD_ADDRESS_OFFSET 0x0000u

// #ifdef __PROCESS__
// #define VESA_LOAD_ADDRESS_PHYS (PROCBIN_VADDR - (VESA_LOAD_ADDRESS_SEGMENT * 16 + VESA_LOAD_ADDRESS_OFFSET))
// #else
// #endif

#define VESA_LOAD_ADDRESS_PHYS (VESA_LOAD_ADDRESS_SEGMENT * 16 + VESA_LOAD_ADDRESS_OFFSET)
#define VESA_CTRL_SIZE 512
#define VESA_TARGET_MODE 0x116 // 1024x768x16

#define GET_VESA_INFO() ((VESA_INFO*)VESA_LOAD_ADDRESS_PHYS)

BOOL vesa_check(void);
#endif // __RTOS || KERNEL_ENTRY

#endif // VESA_H