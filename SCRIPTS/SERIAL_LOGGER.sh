#!/usr/bin/env bash

BASE="OUTPUT/SERIAL"

echo "[SERIAL LOGGER] Starting..."

mkdir -p "$BASE"

log_port () {
    PORT=$1
    PIPE="$BASE/SERIAL${PORT}.out"
    OUTFILE="$BASE/SERIAL${PORT}.data"

    echo "  Logging COM${PORT} -> $OUTFILE"

    # ✅ clear previous log
    : > "$OUTFILE"

    # unbuffered continuous logging
    stdbuf -o0 cat "$PIPE" >> "$OUTFILE" &
}

log_port 2
log_port 3
log_port 4

echo "[SERIAL LOGGER] Running. Translating data in real time. See OUTPUT/SERIAL data files (Ctrl+C to stop)"
wait