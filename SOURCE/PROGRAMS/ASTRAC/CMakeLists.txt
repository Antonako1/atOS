cmake_minimum_required(VERSION 3.16)
project(ASTRAC C)

# ═══════════════════════════════════════════════════════════════════════════
#  Paths
# ═══════════════════════════════════════════════════════════════════════════
set(ProgramName ASTRAC)
set(CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCE_KERNEL_DIR ${CUR_DIR}/../../KERNEL)
set(OUTPUT_DIR ${CUR_DIR}/../../../OUTPUT/PROGRAMS/${ProgramName})
set(STD_DIR ${CUR_DIR}/../../STD)
set(RUNTIME_DIR ${CUR_DIR}/../../RUNTIME)

# ═══════════════════════════════════════════════════════════════════════════
#  Sources
# ═══════════════════════════════════════════════════════════════════════════
file(GLOB STD_SOURCES CONFIGURE_DEPENDS "${STD_DIR}/*.c")

set(Sources
    # Runtime
    ${RUNTIME_DIR}/RUNTIME.c

    # Main entry + shared
    ${CUR_DIR}/ASTRAC.c
    ${CUR_DIR}/FULL_BUILD.c
    ${CUR_DIR}/SHARED/PREPROCESS.c

    # Assembler pipeline
    ${CUR_DIR}/ASSEMBLER/AMAIN.c
    ${CUR_DIR}/ASSEMBLER/LEXER.c
    ${CUR_DIR}/ASSEMBLER/AST.c
    ${CUR_DIR}/ASSEMBLER/VERIFY_AST.c
    ${CUR_DIR}/ASSEMBLER/OPTIMIZE.c
    ${CUR_DIR}/ASSEMBLER/GEN.c

    # Disassembler
    ${CUR_DIR}/DISSASEMBLER/DISMAIN.c

    # Compiler front-end
    ${CUR_DIR}/COMPILER/COMPMAIN.c
    ${CUR_DIR}/COMPILER/PARSER.c

    # Standard library (atOS)
    ${STD_SOURCES}
)

# ═══════════════════════════════════════════════════════════════════════════
#  Include directories
# ═══════════════════════════════════════════════════════════════════════════
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

# ═══════════════════════════════════════════════════════════════════════════
#  Compiler flags (freestanding i386 user-mode binary)
# ═══════════════════════════════════════════════════════════════════════════
set(CompArgs
    -DPROCBIN_VADDR=0x10000000
    -D__USER__
    -D__PROCESS__
    -D__UP__
    -D__${ProgramName}__
    
    # 🛑 Disables ALL warnings (including those enabled by -Wall/-Wextra)
    -w
    
    # -Wno-comment (No longer needed if using -w)
    # -Wno-comments (No longer needed if using -w)
    -DDEBUG
    -m32
    -ffreestanding
    -fno-pic
    -fno-pie
    -nostdlib
    -O0
    # -Wall (No longer needed if using -w)
    # -Wextra (No longer needed if using -w)
    -fno-stack-protector
    -fno-builtin
    -fno-inline
)

add_compile_options(${CompArgs})

# ═══════════════════════════════════════════════════════════════════════════
#  Build target
# ═══════════════════════════════════════════════════════════════════════════
add_executable(${ProgramName}.BIN ${Sources})

set_target_properties(${ProgramName}.BIN PROPERTIES
    OUTPUT_NAME "${ProgramName}.BIN"
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)

# ═══════════════════════════════════════════════════════════════════════════
#  Linker options (flat binary output, custom linker script)
# ═══════════════════════════════════════════════════════════════════════════
target_link_options(${ProgramName}.BIN PRIVATE
    -m32
    -nostdlib
    -ffreestanding
    -Wl,-T,${CUR_DIR}/../USER_PROGRAMS.ld
    -Wl,-e,_start
    -Wl,--oformat=binary
)

# ═══════════════════════════════════════════════════════════════════════════
#  Clean target
# ═══════════════════════════════════════════════════════════════════════════
add_custom_target(clean_${ProgramName}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${OUTPUT_DIR}/*.BIN ${OUTPUT_DIR}/*.o
    COMMENT "Cleaning ${ProgramName} build outputs"
)
