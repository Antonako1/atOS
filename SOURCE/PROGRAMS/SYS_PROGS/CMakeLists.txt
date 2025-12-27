cmake_minimum_required(VERSION 3.16)

project(SystemPrograms C)

set(CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCE_KERNEL_DIR ${CUR_DIR}/../../KERNEL)
set(STD_DIR ${CUR_DIR}/../../STD)
set(RUNTIME_DIR ${CUR_DIR}/../../RUNTIME)

# ========================================
# Helper function: Load rows from a file
# ========================================
function(load_rows FILE_OUT FILE_PATH)
    if(EXISTS "${FILE_PATH}")
        file(STRINGS "${FILE_PATH}" CONTENTS)
        set(${FILE_OUT} ${CONTENTS} PARENT_SCOPE)
    else()
        set(${FILE_OUT} "" PARENT_SCOPE)
    endif()
endfunction()

# ===================================================
# Helper: Determine if file is a code source file
# ===================================================
function(is_code_file OUT_VAR FILEPATH)
    get_filename_component(EXT "${FILEPATH}" EXT)
    string(TOLOWER "${EXT}" EXT)

    set(ALLOWED_EXTS
        ".c" ".cc" ".cpp" ".cxx"
        ".h" ".hh" ".hpp" ".hxx"
        ".s" ".asm" ".S"
    )

    if(EXT IN_LIST ALLOWED_EXTS)
        set(${OUT_VAR} TRUE PARENT_SCOPE)
    else()
        set(${OUT_VAR} FALSE PARENT_SCOPE)
    endif()
endfunction()

# ====================================================
# Helper: Add files to ALL_SOURCES or ATOS_FILES
# ====================================================
function(collect_files INPUT_LIST BASE_DIR)
    foreach(ITEM ${INPUT_LIST})
        set(FULLPATH "${BASE_DIR}/${ITEM}")
        is_code_file(IS_CODE "${FULLPATH}")

        if(IS_CODE)
            list(APPEND ALL_SOURCES "${FULLPATH}")
        else()
            list(APPEND ATOS_FILES "${FULLPATH}")
        endif()
    endforeach()

    set(ALL_SOURCES "${ALL_SOURCES}" PARENT_SCOPE)
    set(ATOS_FILES "${ATOS_FILES}" PARENT_SCOPE)
endfunction()

# ==========================================
# Function that creates a program automatically
# ==========================================
function(add_system_program PROGNAME PROGDIR)

    message(STATUS "Configuring program: ${PROGNAME}")

    set(SOURCES_FILE "${PROGDIR}/${PROGNAME}.SOURCES")
    set(LIBS_FILE    "${PROGDIR}/${PROGNAME}.LIBS")
    set(BDEF_FILE    "${PROGDIR}/${PROGNAME}.BDEF")

    # --- storage for collected files ---
    set(ALL_SOURCES "")
    set(ATOS_FILES "")
    list(APPEND ALL_SOURCES "${RUNTIME_DIR}/RUNTIME.c")

    # 1. Load program source files
    load_rows(PROG_SOURCES "${SOURCES_FILE}")
    collect_files("${PROG_SOURCES}" "${PROGDIR}")

    # 2. Load libraries if .LIBS exists
    if(EXISTS "${LIBS_FILE}")
        load_rows(LIBLIST "${LIBS_FILE}")

        foreach(LIBSRC_FILE ${LIBLIST})
            set(FULL_LIBSRC "${PROGDIR}/${LIBSRC_FILE}")
            load_rows(LIBSRC_ROWS "${FULL_LIBSRC}")

            get_filename_component(LIBSRC_DIR "${FULL_LIBSRC}" DIRECTORY)
            collect_files("${LIBSRC_ROWS}" "${LIBSRC_DIR}")
        endforeach()
    endif()

    # Add STD sources
    file(GLOB STD_SOURCES CONFIGURE_DEPENDS "${STD_DIR}/*.c")
    list(APPEND ALL_SOURCES ${STD_SOURCES})

    # Output folder
    set(OUTPUT_DIR ${CUR_DIR}/../../../ISO_DIR/ATOS)
    set(ATOS_OUT_DIR "${OUTPUT_DIR}")

    # Include directories
    include_directories(
        ${PROGDIR}
        ${CUR_DIR}
        ${CUR_DIR}/../../
        ${SOURCE_KERNEL_DIR}/32RTOSKRNL/DRIVERS/
        ${SOURCE_KERNEL_DIR}/32RTOSKRNL/
        ${SOURCE_KERNEL_DIR}/32RTOSKRNL/CPU/
        ${SOURCE_KERNEL_DIR}/32RTOSKRNL/MEMORY/
        ${SOURCE_KERNEL_DIR}/32RTOSKRNL/FS/
        ${SOURCE_KERNEL_DIR}/32RTOSKRNL/RTOSKRNL/
    )

    # Global base compiler flags (common to all programs)
    set(BASE_COMP_ARGS
        -DPROCBIN_VADDR=0x10000000
        -D__USER__
        -D__PROCESS__
        -D__UP__
        -D__${PROGNAME}__

        -w
        -m32
        -ffreestanding
        -fno-pic
        -fno-pie
        -nostdlib
        -fno-stack-protector
        -fno-builtin
        -fno-inline
    )

    # Executable
    add_executable(${PROGNAME}.BIN ${ALL_SOURCES})

    target_compile_options(${PROGNAME}.BIN PRIVATE ${BASE_COMP_ARGS})

    # 3. Load build definitions (.BDEF) if present
    if(EXISTS "${BDEF_FILE}")
        message(STATUS "  Using build definitions: ${BDEF_FILE}")
        load_rows(BDEF_FLAGS "${BDEF_FILE}")
        if(BDEF_FLAGS)
            target_compile_options(${PROGNAME}.BIN PRIVATE ${BDEF_FLAGS})
        endif()
    endif()

    # Copy non-code files and/or directories into ATOS/
    if(ATOS_FILES)
        add_custom_command(TARGET ${PROGNAME}.BIN POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${ATOS_OUT_DIR}"
        )

        foreach(RES ${ATOS_FILES})
            get_filename_component(FNAME2 "${RES}" NAME_WE)
            if(IS_DIRECTORY "${RES}")
                add_custom_command(TARGET ${PROGNAME}.BIN POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                            "${RES}" "${ATOS_OUT_DIR}/${FNAME2}"
                    COMMENT "Copying directory ${RES} to ATOS/"
                )
            else()
                get_filename_component(FILENAME "${RES}" NAME)
                add_custom_command(TARGET ${PROGNAME}.BIN POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${RES}" "${ATOS_OUT_DIR}/${FILENAME}"
                    COMMENT "Copying file ${FILENAME} to ATOS/"
                )
            endif()
        endforeach()
    endif()

    set_target_properties(${PROGNAME}.BIN PROPERTIES
        OUTPUT_NAME "${PROGNAME}.BIN"
        RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    )

    target_link_options(${PROGNAME}.BIN PRIVATE
        -m32
        -nostdlib
        -ffreestanding
        -Wl,-T,${CUR_DIR}/../USER_PROGRAMS.ld
        -Wl,-e,_start
        -Wl,--oformat=binary
    )
endfunction()

# ===============================================
# Scan current folder and 1-level subfolders
# for all *.SOURCES files
# ===============================================
file(GLOB PROG_SOURCE_FILES
    "${CUR_DIR}/*.SOURCES"
    "${CUR_DIR}/*/*.SOURCES"
)

foreach(FILEPATH ${PROG_SOURCE_FILES})
    get_filename_component(PROGNAME "${FILEPATH}" NAME_WE)
    get_filename_component(PROGDIR  "${FILEPATH}" DIRECTORY)

    add_system_program(${PROGNAME} "${PROGDIR}")
endforeach()
