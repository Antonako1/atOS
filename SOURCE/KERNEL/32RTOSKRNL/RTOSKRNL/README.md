# RTOSKRNL

RTOSKRNL directory contains functions for the 32RTOS kernel, as other directories one level down contain modules used by the KERNEL (kernel entry point) aswell.

## Structure

- ./PROC - Process Management, Scheduling, and Communication by the Kernel.
- ./Error - Error handling used by the Kernel.
- ./ACPI - ACPI support used by the Kernel.
- ./RTOSKRNL_INTERNAL.[c|h] - Internal RTOS kernel functions. Some exposed via syscalls

## RTOSKRNL_INTERNAL

RTOSKRNL_INTERNAL files are files containing miscellaneous code for the 32RTOS kernel, such as:

- panic, panic_if and its variants
    - Halts the system in case of an serious error. Comparable to the Windows blue screen
- system_halt, system_halt_if and its variants
    - Halts the CPU. NOP operators are there for debugging
- system_reboot and its variants
    - Reboots the system
- system_shutdown and its variants
    - Shuts down the system. Not working yet
- DUMP_... functions
    - Dumps various data to screen, such as memory, strings, and numbers.

These files also contain the code to initialize/load the filesystem, start the first shell instance and start the inter-process communication loop