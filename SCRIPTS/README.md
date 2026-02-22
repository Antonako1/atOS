various scripts

CLEAN.sh - Cleans all the build files

DISSASEMBLE.sh {input.bin} - Dissasembles the given file

HOT_RELOAD.sh [hot] - Opens qemu with hot reload support. If hot is given as arg, it will reload qemu instance with the new iso

SYNC_UPSTREAM_DEVELOPMENT.sh - Updates local development branch from main repo

SERIAL_LOGGER.sh - Turns all serial port .in files into a .data file with the same name. COM1 not used

SERIAL_SEND.sh {port} {file} - Sends the contents of the given file to the given serial port. COM1 not used 