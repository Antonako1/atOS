# Paging

This directory contains the implementation of the paging mechanism in atOS-RT. Paging is a memory management scheme that eliminates the need for contiguous allocation of physical memory, thereby reducing fragmentation and allowing for more efficient use of memory.

## Overview

This is a simple 32-bit, single-level paging implementation with basic process isolation.
It provides ring0 paging for atOS-RT.

Kernel is linked to 0x100000 and identity-mapped there. It has the ownership of all memory

"User programs" aka processes are linked to 0x10000000
    but mapped to anywhere in physical memory in the following region: `0x10000000`-`0x20000000`.
    The copying of the pagedirectories goes something like this
```sh
# NOTE: These memory regions are as of writing and may be changed as of reading
# Every bit of memory is mapped for kernel
`0x00000000`-`0x00100000` - Miscellaneous 
`0x00100000`-`0x00550000` - Kernel image
`0x00550000`-`0x08550000` - Kernel heap
`0x08560000`-`0x08570000` - Kernel stack
`0x08580000`-`0x0B000000` - Kernel framebuffer
`0x0B000000`-`0x10000000` - Reserved for expansion as of writing
0x10000000 - Start of user process memory
0x20000000 - Start of user library memory

# Let's say you run a process. A memory region of 0x10040000-0x1004A000 is allocated for your process
# Kernel mappings for memory are first copied into the process, so it looks like the kernel map
0x00000000 - Miscellaneous 
0x00100000 - Kernel image
0x00550000 - Kernel heap
0x08560000 - Kernel stack
0x08580000 - Kernel framebuffer
0x0B000000 - Reserved for expansion as of writing
0x10000000 - Start of user process memory
0x20000000 - Start of user library memory

# We load the binary into memory and initialize stack there
# These are remapped, so that the start of your binary process points to 0x10000000
# This means that your map would look like this in virtual memory
0x00000000 - Miscellaneous 
0x00100000 - Kernel image
0x00550000 - Kernel heap
0x08560000 - Kernel stack
0x08580000 - Kernel framebuffer
0x0B000000 - Reserved for expansion as of writing
0x10000000 - Start of your process binary, mapped 0x10040000(phys) to 0x10000000(virt)
0x20000000 - Start of user library memory

# This is the reason heap memory and library physical memory pointers can be shared throughout the processes
# This is reversed for libraries
```

"User libraries", or dynamic libraries are linked to 0x20000000 and live in region: `0x20000000`-`0x30000000`

User programs are allocated into a predefined memory region.

This region allows user processes to have their own virtual address space,
    isolated from the kernel and other processes, but still having access to kernel and system memory, but not the memory of other programs.
    Allocated memory can be shared freely around the whole system.
    See the block above.