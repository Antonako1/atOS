/* LIB.ld - Linker script for user dynammic libraries */

OUTPUT_FORMAT("binary")
ENTRY(_start)

SECTIONS
{
    /* Kernel load address in memory */
    . = 0x20000000;

    /* Code section */
    .text : {
        *(.text*)          /* all .text sections from object files */
    }

    /* Read-only data (string literals, constants) */
    .rodata : {
        *(.rodata*)        /* all .rodata sections */
    }

    /* Initialized data */
    .data : {
        *(.data*)          /* all .data sections */
    }

    /* Uninitialized data */
    .bss : {
        *(.bss*)           /* all .bss sections */
        *(COMMON)          /* common symbols (uninitialized globals) */
    }
}
