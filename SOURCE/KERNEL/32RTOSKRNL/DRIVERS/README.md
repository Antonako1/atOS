Contains drivers for the 32-bit kernel

Drivers are as follows:

.\VIDEO - Video driver utilising VESA/VBE


.\PS2 - PS2 keyboard and mouse driver

.\PIT - Programmable Interval Timer driver

.\CMOS -

.\RTL8139 -

.\ATAPI -
.\ATA_PIO -
.\ATA_PIIX3 -

.\AC97 - AC'97 audio driver (ICH-compatible). Requires QEMU `-device ac97,audiodev=...` and is wired via syscalls for simple tone playback and stop.

.\BEEPER - PC beeper audio driver. Not used at all... Use AC97