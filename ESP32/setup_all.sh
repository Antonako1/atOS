#!/usr/bin/env bash
# setup_all.sh - First-time setup: install deps, build all sketches, upload SERIAL, open monitor
# Usage: ./setup_all.sh [PORT]
# Examples:
#   ./setup_all.sh
#   ./setup_all.sh /dev/ttyUSB0

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/config.sh"

[ -n "$1" ] && PORT="$1"
esp32_select_port
export PORT

echo "====================================="
echo " ESP32 Full Setup"
echo "====================================="
echo

echo "--- Installing dependencies ---"
bash "$SCRIPT_DIR/install_deps.sh"
echo

echo "--- Building LCD_TEST ---"
bash "$SCRIPT_DIR/build.sh" LCD_TEST
echo

echo "--- Building SERIAL ---"
bash "$SCRIPT_DIR/build.sh" SERIAL
echo

echo "--- Uploading SERIAL -> $PORT ---"
bash "$SCRIPT_DIR/upload.sh" SERIAL "$PORT"
echo

echo "--- Opening monitor (CTRL+C to exit) ---"
bash "$SCRIPT_DIR/monitor.sh" "$PORT"
