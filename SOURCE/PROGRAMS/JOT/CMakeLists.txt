cmake_minimum_required(VERSION 3.16)
project(JOT C)

# === Configuration ===
set(ProgramName JOT)
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

# === ATUI ===
file(STRINGS "${CUR_DIR}/../../LIBRARIES/ATUI/SOURCES" ATUI_SOURCES)
list(TRANSFORM ATUI_SOURCES PREPEND "${CUR_DIR}/../../LIBRARIES/ATUI/")

set(Sources
    ${RUNTIME_DIR}/RUNTIME.c
    ${CUR_DIR}/JOT.c
    ${STD_SOURCES}
    ${ATUI_SOURCES}
)

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
