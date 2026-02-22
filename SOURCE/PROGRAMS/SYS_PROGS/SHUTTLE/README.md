# SHUTTLE

Shuttle is utility program that allows users to send/receive data via serial ports.

Allows usage of COM2-4, all for I/O. COM1 is reserved for debugging purposes.

See SCRIPTS/SERIAL_LOGGER.sh and SCRIPTS/SERIAL_SENDER.sh for retrieving data and sending data.

Usage is as follows:

```
shuttle - Serial COM port data transfer tool

USAGE:
  shuttle -p <port> -s (-S <string> | -f <file>)
  shuttle -p <port> -r -f <file>
  shuttle -p <port> -c

DESCRIPTION:
  Sends or receives data through COM2-COM4 UART ports.
  COM1 is reserved for debug logging.

OPTIONS:
  -h, --help.                  Show this help message

  -p, --port <n>.              Select COM port number (2, 3, or 4)

  -s, --send.                  Send data to the selected COM port

  -r, --receive.               Receive data from the selected COM port

  -f, --file <path>.           File to send or destination file when receiving

  -S, --string <text>.         String data to send

  -c, --console.               Sends any data logged from keyboard

EXAMPLES:
  # This will send the string "Hello, COM2!" to COM2.
  shuttle -p 2 -s -S "Hello, COM2!"

  # This will send the contents of 'data.bin' to COM3.
  shuttle -p 3 -s -f data.bin

  # This will receive data from COM4 and save it to 'received_data.bin'.
  shuttle -p 4 -r -f received_data.bin
  
  # This will log any keystrokes to COM2. The process will run indefinitely until killed with 'kill'.
  shuttle -p 2 -c
```
