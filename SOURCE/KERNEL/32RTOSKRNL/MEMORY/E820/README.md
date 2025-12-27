# E820 Memory Map Handling in RTOS Kernel

This directory contains the implementation for parsing and handling the E820 memory map provided by the BIOS during system boot inside the 2nd. stage bootloader. The E820 memory map is crucial for understanding the physical memory layout of the system, including available RAM regions and reserved areas.

In QEMU, the E820 map is typically hardcoded to simulate a standard memory layout, but on real hardware, it is provided by the BIOS and can vary between systems.

Since QEMU provides a fixed E820 map, this is implemented as such, but the code should be adaptable for real hardware where the E820 map is dynamic.