# Arghand - An argument handling library

Arghand is a lightweight C library designed to simplify the process of handling command-line arguments in C programs. It provides a straightforward interface for parsing and managing arguments passed to your application, making it easier to implement command-line functionality.

## Features

- Simple API for argument parsing
- Support for flags and options
- Easy integration with existing C projects

## Usage

```c
PU8 help[] = ARGHAND_ARG("-h", "--help");
PU8 name[] = ARGHAND_ARG("-n", "--name");

PPU8 allAliases[] = ARGHAND_ALL_ALIASES(help, name);
U32 aliasCounts[] = { ARGHAND_ALIAS_COUNT(help), ARGHAND_ALIAS_COUNT(name) };

ARGHANDLER ah;
ARGHAND_INIT(&ah, argc, argv, allAliases, aliasCounts, 2);
if(ARGHAND_IS_PRESENT(&ah, "-h")) {
    ARGHAND_FREE(&ah);
    printf("Help: ...\n");
    return 0;
}

PU8 val = ARGHAND_GET_VALUE(&ah, "--name");
if(val) {
    PRINT("Name: %s\n", val);
}
ARGHAND_FREE(&ah);
```

## Installation

### SYS_PROGS

For SYS_PROGS programs, include the Arghand `SOURCES` file in your `project.LIBS` file

### Normal C Programs

To include Arghand, add the following lines to your CMakeLists.txt:

```sh
file(STRINGS "${CUR_DIR}/../../LIBRARIES/ARGHAND/SOURCES" ARGHAND_SOURCES)
list(TRANSFORM ARGHAND_SOURCES PREPEND "${CUR_DIR}/../../LIBRARIES/ARGHAND/")

set(Sources
    # Your sources...
    ${STD_SOURCES}
    ${ARGHAND_SOURCES}
    ${RUNTIME_DIR}/RUNTIME.c
)
```