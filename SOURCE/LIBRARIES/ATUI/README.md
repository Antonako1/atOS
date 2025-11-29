# ATUI — A static console UI library

## Configuration

## Adding to CMake

```sh
# === ATUI ===
file(STRINGS "${CUR_DIR}/../../LIBRARIES/ATUI/SOURCES" ATUI_SOURCES)
list(TRANSFORM ATUI_SOURCES PREPEND "${CUR_DIR}/../../LIBRARIES/ATUI/")

set(Sources
    # Your sources...
    ${STD_SOURCES}
    ${ATUI_SOURCES}
    ${RUNTIME_DIR}/RUNTIME.c
)
```