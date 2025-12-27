Contains drivers for the 32-bit kernel

Drivers are as follows:

.\AC97 - AC'97 audio driver (ICH-compatible). Requires QEMU `-device ac97,audiodev=...` and is wired via syscalls for simple tone playback and stop.

.\ATA_PIIX3 - PIIX3 ATA controller driver

.\ATA_PIO - PIO-mode ATA driver

.\ATAPI - ATAPI CD-ROM driver

.\BEEPER - PC beeper audio driver. Not used at all... Use AC97

.\CMOS - CMOS and RTC driver

.\PCI - PCI bus driver

.\PS2 - PS2 drivers (keyboard and mouse)

.\RTL8139 - Realtek RTL8139 ethernet driver

.\VIDEO - Video driver utilising VESA/VBE

.\DRIVER_INC.h - Common driver includes

.\ATA_SHARED.h - Common ATA definitions


