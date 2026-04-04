# 16-bit kernel

 - KERNEL_ENTRY.asm    = 2. stage bootloader. Max binary file size: 4095 bytes

 - KRNL_NTR.ASM        = 2. stage bootloader. Loads the 32-bit kernel from FAT32 partition. Max binary file size: 4095 bytes. Inside `HOME/SRC/KRNL/KRNL_NTR.ASM`

Include file found at .\16-BIT-BIOS\KERNEL_ENTRY_DATA.inc

# 32-bit kernel(s)

 - KERNEL.asm          = 32-bit kernel entry point
 - RTOSKRNL.asm        = 32-bit kernel

Include files for these can be found at .\32RTOSKRNL


 - KERNEL[.BIN] refers to the 2nd stage bootloader
 - KRNL[.BIN] refers to the 32-bit kernel entry point
 - [32]RTOSKRNL[.BIN] refers to the 32-bit kernel