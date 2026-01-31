# `SOURCE/` Directory Overview

This directory contains all core components and user-defined extensions that make up the atOS operating system. Below is a breakdown of each subdirectory and its purpose:

```
SOURCE/
├── BASE.txt            # Test file
├── INSIDE_1.txt        # Test file
├── BOOTLOADER/         # 1st stage bootloader
├── KERNEL/             # 32-bit kernel source code (protected mode) and 2nd stage bootloader
├── PROGRAMS/           # User programs to be included in the final ISO
├── LIBRARIES/          # User libraries to be included in the final ISO
├── README.md           # This file - provides an overview of the source layout
├── STD/                # Standard library functions or reusable code
├── RUNTIME/            # Runtime environment source code
├── SYS_SRC/            # Source code files that are part of the internal system. Used by the AstraC compiler.
├── FILES/              # Points to /HOME/DOCS/ directory
├── ATOSH.PATH/         # atOShell default path values 
├── ATOSHELL.SH/        # atOShell default startup script
├── ATOSH.ENV_VARS/           # Default environment variables for atOShell
└── USER_PROGRAMS       # File list of programs to be compiled and included. See DOCS\DEVELOPING.md
```

* For more details on each component, see the README files located within each respective folder.

