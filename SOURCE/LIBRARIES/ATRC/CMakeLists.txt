cmake_minimum_required(VERSION 3.16)
project(ATRC C)

# === Configuration ===
set(CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ProgramName ATRC)
set(SOURCE_KERNEL_DIR ${CUR_DIR}/../../KERNEL)
set(OUTPUT_DIR ${CUR_DIR}/../../../OUTPUT/LIBS/${ProgramName})
set(STD_DIR ${CUR_DIR}/../../STD)

# === Sources ===
file(GLOB STD_SOURCES CONFIGURE_DEPENDS "${STD_DIR}/*.c")
set(Sources
    ${CUR_DIR}/ATRC.c
    ${STD_SOURCES}
)

# === Include directories ===
include_directories(
    ${CUR_DIR}
    ${CUR_DIR}/../../
    ${SOURCE_KERNEL_DIR}/32RTOSKRNL/
    ${SOURCE_KERNEL_DIR}/32RTOSKRNL/DRIVERS/
    ${SOURCE_KERNEL_DIR}/32RTOSKRNL/CPU/
    ${SOURCE_KERNEL_DIR}/32RTOSKRNL/MEMORY/
    ${SOURCE_KERNEL_DIR}/32RTOSKRNL/FS/
    ${SOURCE_KERNEL_DIR}/32RTOSKRNL/RTOSKRNL/
)

# === Compiler flags ===
set(CompArgs
    -DPROCBIN_VADDR=0x20000000
    -D__USER__
    -D__LIB__
    -m32
    -ffreestanding
    -fno-pic
    -fno-pie
    -nostdlib
    -O2
    -Wall
    -Wextra
    -fno-stack-protector
    -fno-builtin
    -fno-inline
    -Wno-comments
)

add_compile_options(${CompArgs})

# === Build target ===
add_executable(${ProgramName}.LIB ${Sources})

# Output to IronClad library directory
set_target_properties(${ProgramName}.LIB PROPERTIES
    OUTPUT_NAME "${ProgramName}.LIB"
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)

# === Linker options ===
target_link_options(${ProgramName}.LIB PRIVATE
    -m32
    -nostdlib
    -ffreestanding
    -Wl,-T,${CUR_DIR}/../LIB.ld
    -Wl,-e,_start
    -Wl,--oformat=binary
)

# === Clean target ===
add_custom_target(clean_${ProgramName}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${OUTPUT_DIR}/*.LIB ${OUTPUT_DIR}/*.o
    COMMENT "Cleaning ${ProgramName} build outputs"
)
