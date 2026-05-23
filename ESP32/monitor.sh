#!/usr/bin/env bash
# monitor.sh - Open serial monitor on the ESP32
# Usage: ./monitor.sh [PORT]
# Examples:
#   ./monitor.sh
#   ./monitor.sh /dev/ttyUSB0
#   PORT=/dev/ttyUSB0 ./monitor.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/config.sh"

[ -n "$1" ] && PORT="$1"
esp32_select_port

echo "====================================="
echo " Serial monitor: $PORT @ 115200 baud"
echo " Press CTRL+C to exit."
echo "====================================="
echo

arduino-cli monitor -p "$PORT" -c baudrate=115200
