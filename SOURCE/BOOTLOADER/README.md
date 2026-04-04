"BOOTLOADER.asm" is 1. stage bootloader. It loads 2nd stage bootloader into memory and jumps into it.

2nd stage bootloader is "..\KERNEL\KERNEL_ENTRY.asm". It reads memory, sets up 32-bit mode and jumps to 32-bit kernel entry. 

"DISK_VBR.asm" Is the virtual boot record for the disk image. It loads 1st stage bootloader into memory and jumps into it.

It can be found in `HOME/SRC/BOOTLOADER/VBR.ASM`. It is built with AstraC Assembler, exported out via shuttle and exported.