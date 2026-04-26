# Dynamic or static libraries

## ATRC - Static header-only resource file library

## ARGHAND - Static argument handling library

## ATGL - Static graphics and UI library

## ATUI - Static terminal UI library


## 1 way to implement dynamic libraries in atOS:

Libary is loaded into memory in the 0x2000000 - 0x3000000 range, once loaded the LIB_MAIN() function is called, which should initialize the library and return a pointer to a struct containing function pointers for the library's API. The library can also export global variables if needed.

### Problems with this approach:

Because the library is loaded into a fixed virtual memory address, support for multiple libraries is limited. Only one library can be loaded at a time, and loading a second library would overwrite the first one. This approach also makes it difficult to unload libraries or manage dependencies between them.

### Alternative approach:

A more flexible approach would be to use a dynamic loading mechanism, where libraries are loaded at runtime and assigned unique virtual memory addresses. This would allow multiple libraries to coexist without conflicts and enable features like unloading libraries or managing dependencies. However, this approach would require a more complex loader and additional bookkeeping to track loaded libraries and their addresses.

### Second alternative approach:
Another approach could be to parse the binary format. Libraries are loaded so that PhysAddr = VirtAddr. We parse the binary format, apply relocations so that entry is not at 0x2000000 but at the actual physical address. This allows multiple libraries to be loaded at the same time.