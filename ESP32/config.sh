#!/usr/bin/env bash
# config.sh - Shared configuration for ESP32 scripts
# Source this file from other scripts: source "$SCRIPT_DIR/config.sh"

BOARD_FQBN="esp32:esp32:esp32"

# Resolve sketch directory.
# Usage: esp32_resolve_sketch [SKETCH_NAME]
# Sets: SKETCH (name) and SKETCH_DIR (absolute path to sketch folder).
# Available sketches: SERIAL, LCD_TEST
esp32_resolve_sketch() {
    SKETCH="${1:-SERIAL}"
    SKETCH_DIR="$SCRIPT_DIR/$SKETCH"

    if [ ! -f "$SKETCH_DIR/$SKETCH.ino" ]; then
        echo "ERROR: Sketch not found: $SKETCH_DIR/$SKETCH.ino"
        echo "Available sketches:"
        for d in "$SCRIPT_DIR"/*/; do
            name="$(basename "$d")"
            [ -f "$d/$name.ino" ] && echo "  $name"
        done
        exit 1
    fi
}

# Prompt for PORT if not already set.
# Accepts PORT via env or as a positional arg passed before calling.
esp32_select_port() {
    if [ -n "$PORT" ] && [ -e "$PORT" ]; then
        return 0
    fi

    echo "Available serial devices:"
    ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "  (none found)"
    echo

    read -rp "Enter ESP32 USB port (e.g. /dev/ttyUSB0): " PORT

    if [ ! -e "$PORT" ]; then
        echo "ERROR: Device not found: $PORT"
        exit 1
    fi
}
