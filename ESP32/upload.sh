#!/usr/bin/env bash
# upload.sh - Compile and upload an ESP32 sketch
# Usage: ./upload.sh [SKETCH] [PORT]
# Examples:
#   ./upload.sh                          # SERIAL, prompt for port
#   ./upload.sh LED_TEST                 # LED_TEST, prompt for port
#   ./upload.sh LED_TEST /dev/ttyUSB0   # no prompt
#   PORT=/dev/ttyUSB0 ./upload.sh       # port via env

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/config.sh"

esp32_resolve_sketch "$1"
[ -n "$2" ] && PORT="$2"
esp32_select_port

echo "====================================="
echo " Uploading: $SKETCH -> $PORT"
echo "====================================="
echo

arduino-cli compile --fqbn "$BOARD_FQBN" "$SKETCH_DIR"
arduino-cli upload  -p "$PORT" --fqbn "$BOARD_FQBN" "$SKETCH_DIR"

echo
echo "Upload successful: $SKETCH -> $PORT"
