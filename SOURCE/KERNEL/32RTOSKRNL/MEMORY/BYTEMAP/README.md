# BYTEMAP - Byte-level Memory Bitmap management

This module implements a byte-level memory bitmap for tracking memory usage in the atOS. Unlike traditional bitmaps that use one bit per page, this byte-level bitmap uses one byte per page, allowing for more granular tracking of memory states.

This is needed for because of how the memory is paged. Please see the PAGING module readme for more details.

Here is the enum definition for the different page states.
```c
typedef enum PageState {
    PS_FREE        = 0, // Free page
    PS_ALLOC       = 1, // Allocated by the allocator

    PS_RESERVED    = 2,  // firmware/MMIO or not for allocator
    PS_LOCKED      = 3,  // temporary pin

    PS_FUSER        = 4,  // Free process page
    PS_LUSER = 5,  // Locked user process page
    PS_AUSER = 6,  // Allocated user process page
    PS_RUSER = 7,  // Reserved user process page

    PS_FLIB = 9, // Free library page
    PS_LLIB = 10, // Locked library page
    PS_ALIB = 11, // Allocated library page
    PS_RLIB = 8, // Reserved library page
} PageState;
```

