#!/bin/sh

if [ "$1" != "from_make" ]; then
    echo "[1/2] Running make clean..."
    make clean
fi

echo "[2/2] Removing all build directories..."
find . -type d -name "build" -exec rm -rf {} +

echo "Done."
