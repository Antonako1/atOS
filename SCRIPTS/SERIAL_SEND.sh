#!/usr/bin/env bash

if [ $# -lt 2 ]; then
    echo "Usage:"
    echo "  $0 <port> <file>"
    echo "Example:"
    echo "  $0 2 test.bin"
    exit 1
fi

PORT=$1
FILE=$2
PIPE="OUTPUT/SERIAL/SERIAL${PORT}.in"

if [ ! -p "$PIPE" ]; then
    echo "Error: pipe $PIPE not found"
    exit 1
fi

if [ ! -f "$FILE" ]; then
    echo "Error: file $FILE not found"
    exit 1
fi

echo "[SERIAL SEND] Clearing old pending data..."
# Open + immediately close writer to flush old readers
: > "$PIPE" 2>/dev/null || true

echo "[SERIAL SEND] Sending $FILE -> COM${PORT}"

cat "$FILE" > "$PIPE"

echo "[SERIAL SEND] Done. Data sent to COM${PORT}"