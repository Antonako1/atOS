#!/usr/bin/env bash
# build.sh - Compile an ESP32 sketch
# Usage: ./build.sh [SKETCH]     (default: SERIAL)
# Examples:
#   ./build.sh
#   ./build.sh LED_TEST

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/config.sh"

esp32_resolve_sketch "$1"

echo "====================================="
echo " Compiling: $SKETCH"
echo " Dir:       $SKETCH_DIR"
echo "====================================="
echo

arduino-cli compile --fqbn "$BOARD_FQBN" "$SKETCH_DIR"

echo
echo "Build successful: $SKETCH"
