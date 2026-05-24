#!/usr/bin/env bash
# send_serial.sh - Send a string over a USB-to-COM serial adapter
#
# Usage:
#   ./send_serial.sh "Hello World"
#   ./send_serial.sh "Hello World" /dev/ttyUSB1
#   ./send_serial.sh "Hello World" /dev/ttyUSB1 115200
#
# Arguments:
#   $1  String to send (required)
#   $2  Serial port     (default: /dev/ttyUSB1)
#   $3  Baud rate       (default: 115200)

set -e

MESSAGE="${1}"
PORT="${2:-/dev/ttyUSB1}"
BAUD="${3:-115200}"

if [ -z "$MESSAGE" ]; then
    echo "Usage: $0 \"message\" [port] [baud]"
    echo "Example: $0 \"Hello\" /dev/ttyUSB1 9600"
    exit 1
fi

if [ ! -e "$PORT" ]; then
    echo "ERROR: Port not found: $PORT"
    echo "Available ports:"
    ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "  (none)"
    exit 1
fi

# Configure port: baud rate, 8N1, no flow control, raw mode
stty -F "$PORT" "$BAUD" cs8 -cstopb -parenb -crtscts raw

# Send the string followed by newline (ESP32 SERIAL.ino splits on '\n')
printf "%s\n" "$MESSAGE" > "$PORT"

echo "Sent to $PORT @ ${BAUD}baud: $MESSAGE"
