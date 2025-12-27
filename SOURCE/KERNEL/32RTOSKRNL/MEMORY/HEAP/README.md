# HEAP Memory Management

HEAP memory management in the 32RTOS kernel is responsible for all dynamic memory allocations requested by the kernel and its subsystems, such as drivers, programs and libraries.

The HEAP module provides functions to allocate, free memory blocks of varying sizes and of various alignment requirements. It manages a large contiguous memory region reserved for dynamic allocations, typically referred to as the "kernel heap pool/kmalloc pool".

The "KERNEL" name is misleading, as this HEAP is used by the entire operating system, including user processes via system calls.

Memory allocated in the HEAP can freely be shared between the whole operating system, including user processes, drivers etc.