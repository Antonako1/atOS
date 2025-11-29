# Program developing

## External developing via GCC and CMake

There are two folders that should interest you:

 1. `SOURCES/PROGRAMS`
 2. `SOURCES/PROGRAMS/SYS_PROGS`

The first option uses CMake to compile your program into its own folder inside `/PROGRAMS/<PROGNAME>/`

This is as simple as:
 - Create a new folder inside `SOURCES/PROGRAMS/`
 - Create a CMake file, paste the below example there and modify it
 - Add the program into `SOURCES/PROGRAMS/USER_PROGRAMS.txt`
  
Example C file:

```c
#include <STD/TYPEDEF.h>
#include <STD/STRING.h>

U0 EXIT() {
    printf("Exiting!\n");
}

U32 main(U32 argc, PPU8 argv) {
    // Register exit functions if needed, see RUNTIME.c for maximum amount
    ON_EXIT(EXIT);

    for(U32 i = 0; i < argc; i++) {
        if(STRCMP(arg, "exit") == 0) {
            printf("Exiting via 'exit' flag!\n");
            return 1;
        } else {
            printf("%s\n", arg);
        }
    }
    
    return 0;
}
```

Example CMake file:

```sh
cmake_minimum_required(VERSION 3.16)

# Name your project
set(ProgramName JOT)
project(${ProgramName} C)

# === Configuration ===
set(CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCE_KERNEL_DIR ${CUR_DIR}/../../KERNEL)
set(OUTPUT_DIR ${CUR_DIR}/../../../OUTPUT/PROGRAMS/${ProgramName})
set(STD_DIR ${CUR_DIR}/../../STD)
set(RUNTIME_DIR ${CUR_DIR}/../../RUNTIME)

# === Sources ===
file(GLOB STD_SOURCES
    CONFIGURE_DEPENDS
    "${STD_DIR}/*.c"
)

# Includes ATUI library
file(STRINGS "${CUR_DIR}/../../LIBRARIES/ATUI/SOURCES" ATUI_SOURCES)
list(TRANSFORM ATUI_SOURCES PREPEND "${CUR_DIR}/../../LIBRARIES/ATUI/")

# Add sources here
set(Sources
    ${CUR_DIR}/JOT.c
    ${STD_SOURCES}
    ${ATUI_SOURCES}
    ${RUNTIME_DIR}/RUNTIME.c
)

# If you have other resources that need to be copied into the binary output folder,
# add them here
set(OTHER_RESOURCES
    ${CUR_DIR}/JOT.CNF
)

# === Include directories ===
include_directories(
    ${CUR_DIR}
    ${CUR_DIR}/../../
    ${SOURCE_KERNEL_DIR}/32RTOSKRNL/DRIVERS/
    ${SOURCE_KERNEL_DIR}/32RTOSKRNL/
    ${SOURCE_KERNEL_DIR}/32RTOSKRNL/CPU/
    ${SOURCE_KERNEL_DIR}/32RTOSKRNL/MEMORY/
    ${SOURCE_KERNEL_DIR}/32RTOSKRNL/FS/
    ${SOURCE_KERNEL_DIR}/32RTOSKRNL/RTOSKRNL/
)


# === Compiler flags ===
set(CompArgs
    -DPROCBIN_VADDR=0x10000000
    -D__USER__
    -D__PROCESS__
    -D__UP__
    -D__${ProgramName}__
    
    -w
    
    -m32
    -ffreestanding
    -fno-pic
    -fno-pie
    -nostdlib
    -O0
    -fno-stack-protector
    -fno-builtin
    -fno-inline
)

add_compile_options(${CompArgs})

add_compile_options(-include ${STD_DIR}/TYPEDEF.h)

# === Build target ===
add_executable(${ProgramName}.BIN ${Sources})

add_custom_command(TARGET ${ProgramName}.BIN POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${OTHER_RESOURCES}
        ${OUTPUT_DIR}
    COMMENT "Copying resource files..."
)

# Tell CMake to use GCC and treat this as a freestanding binary
set_target_properties(${ProgramName}.BIN PROPERTIES
    OUTPUT_NAME "${ProgramName}.BIN"
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)

# === Custom linker options ===
target_link_options(${ProgramName}.BIN PRIVATE
    -m32
    -nostdlib
    -ffreestanding
    -Wl,-T,${CUR_DIR}/../USER_PROGRAMS.ld
    -Wl,-e,_start
    -Wl,--oformat=binary
)

# === Clean target ===
add_custom_target(clean_${ProgramName}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${OUTPUT_DIR}/*.BIN ${OUTPUT_DIR}/*.o
    COMMENT "Cleaning ${ProgramName} build outputs"
)
```

The second option compiles executables into /ATOS folder. See `SOURCE/PROGRAMS/SYS_PROGS/README.md` for more information