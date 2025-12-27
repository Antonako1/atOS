# Library Developing

As of now, there are two ways to develop a library, either a static, include only library or a static compiled one.

## Static header include

This is as simple as including the header into your source files and you are done

## Static compiled library

For this, your library needs a `SOURCES` file.
Source files are separated by a new line and path is a relative path, starting from the library's root.

To include it use this snippet of code:

```sh
# Example from ATUI

# Reads SOURCES file into ATUI_SOURCES
file(STRINGS "${CUR_DIR}/../../LIBRARIES/ATUI/SOURCES" ATUI_SOURCES)

# Transforms into list, and prepends each source file with absolute path
list(TRANSFORM ATUI_SOURCES PREPEND "${CUR_DIR}/../../LIBRARIES/ATUI/")

set(Sources
    ${CUR_DIR}/JOT.c
    ${STD_SOURCES}

    # Include the library sources in your SOURCES
    ${ATUI_SOURCES}
    ${RUNTIME_DIR}/RUNTIME.c
)
```

## Dynamic libraries

Not implemented yet