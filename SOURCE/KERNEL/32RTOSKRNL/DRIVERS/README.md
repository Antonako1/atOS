Contains drivers for the 32-bit kernel

All of the drivers, EXCEPT PIT, can be compiled outside kernel into your own application, allowing you to use the same drivers in both kernel and user-space.

Drivers are as follows:

.\VIDEO - Video driver utilising VESA/VBE

.\DISK - Disk driver, ATA/ATAPI driver

.\PS2 - PS2 keyboard and mouse driver

.\PIT - Programmable Interval Timer driver

