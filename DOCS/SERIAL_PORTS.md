# Serial ports

Serial port 1 is used for debugging and logging, see DEBUG/KDEBUG.h for kernel debug and STD/DEBUG.h for process debug.

Serial ports 2-4 are free to use. They can send and receive data, see SCRIPTS/SERIAL_SEND.sh for sending data from the host machine, SCRIPTS/SERIAL_LOGGER.sh for logging data on the host machine and STD/COMPORT.h for usage in programs.

The SHUTTLE program can be used to transfer data between in and out the OS and between serial ports. It can be used to send data from the host machine to the OS, from the OS to the host machine and between serial ports. See SYS_PROGS/SHUTTLE/README.md for usage.