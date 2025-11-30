#!/bin/sh

echo "[1/2] Running make clean..."
make clean

echo "[2/2] Removing all build directories..."
find . -type d -name "build" -exec rm -rf {} +

echo "Done."
