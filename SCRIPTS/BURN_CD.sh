#!/usr/bin/env bash
# BURN_CD.sh — Burn the atOS ISO image to a CD/DVD
# Usage: ./SCRIPTS/BURN_CD.sh [/dev/srX]
#   If no device is given, the script lists available optical drives and prompts.

set -euo pipefail

ISO="./OUTPUT/ISO/atOS.iso"

# --- Verify ISO exists ---
if [ ! -f "$ISO" ]; then
    echo "Error: ISO not found at $ISO"
    echo "Run 'make iso' first to build the image."
    exit 1
fi

ISO_SIZE=$(du -sh "$ISO" | cut -f1)
echo "ISO: $ISO ($ISO_SIZE)"

# --- Determine target device ---
TARGET="${1:-}"

if [ -z "$TARGET" ]; then
    echo ""
    echo "Available optical drives:"
    lsblk -d -o NAME,SIZE,MODEL,TRAN | grep "^sr" || echo "  (none detected)"
    echo ""
    read -rp "Enter target optical drive (e.g. /dev/sr0): " TARGET
fi

# --- Validate device ---
if [ ! -b "$TARGET" ]; then
    echo "Error: '$TARGET' is not a valid block device."
    exit 1
fi

# Ensure it looks like an optical drive
if [[ "$TARGET" != /dev/sr* ]] && [[ "$TARGET" != /dev/cd* ]]; then
    echo "Warning: '$TARGET' does not look like an optical drive (expected /dev/srX)."
    read -rp "Continue anyway? [y/N]: " CONT
    if [[ "$CONT" != "y" && "$CONT" != "Y" ]]; then
        echo "Aborted."
        exit 0
    fi
fi

# --- Check for required tools ---
if ! command -v wodim &>/dev/null && ! command -v cdrecord &>/dev/null; then
    echo "Error: 'wodim' (or 'cdrecord') is required but not installed."
    echo "Install it with:  sudo apt install wodim"
    exit 1
fi

BURNER="wodim"
command -v wodim &>/dev/null || BURNER="cdrecord"

# --- Confirm before burning ---
echo ""
echo "Device : $TARGET"
echo "Image  : $ISO ($ISO_SIZE)"
echo "Tool   : $BURNER"
echo ""
echo "WARNING: This will burn the ISO to the disc in $TARGET."
read -rp "Type 'yes' to confirm: " CONFIRM

if [ "$CONFIRM" != "yes" ]; then
    echo "Aborted."
    exit 0
fi

# --- Burn ISO ---
echo ""
echo "Burning $ISO to $TARGET ..."
sudo "$BURNER" -v dev="$TARGET" -data -dao "$ISO"

echo ""
echo "Burn complete. Ejecting $TARGET ..."
sudo eject "$TARGET" || true

echo "Done."
