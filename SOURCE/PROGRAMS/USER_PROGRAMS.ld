/* PROGRAMS.ld - Linker script for user programs */

OUTPUT_FORMAT("binary")
ENTRY(_start)

SECTIONS
{
    /* Program load address in virtual memory */
    . = 0x10000000;

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
